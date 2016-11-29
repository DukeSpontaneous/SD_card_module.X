#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "myFAT32.h"

#include "SD_protocol.h"
#include "MathHelper.h"
#include "TimeHelper.h"

LOGICAL_MY_FAT32_DRIVE_INFO LogacalDriveInfo;

// TODO: �������� �� �����-�� ���������� �������� ��������� �������	
static struct tm currentTime;


// ******************************************************************
//�������: ��������� BIOS Parameter Block ������������� ����������
//(SD �����) �� ������������� � myFAT32
//(��� ���� ��������������� �������� ��������������� ���������
//FAT32, ��� ������� � �������� ������� �������� ��� FAT,
//�������� �������� ���������� �� ������ �������� � �.�.).
//���������: BIOS_PARAMETER_BLOCK_STRUCTURE *bpb (��������� �� �������)
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************

static bool FS_validationMyFAT32(const BIOS_PARAMETER_BLOCK_STRUCTURE *bpb)
{
	// ���� ������� ������ �� �������� �����������,
	if (bpb->BS_jmpBoot[0] != 0xE9 && bpb->BS_jmpBoot[0] != 0xEB)
		return false; // �� ��� ��������� ��������.

	// ���� ��������� ��� ��� ����� �� ������������� ��������� 55AAh,
	// �������� ��������� �� ������: Missing operating system � ������� ���������������)
	if (bpb->bootEndSignature != 0xAA55)
		return false;

	if (bpb->BPB_BytsPerSec != BYTE_PER_SECTOR)
		return false;

	// ���� ����� �������� � �������� �� ����� ������� ������,
	// ��� ������� �� �������������� ��������� [1, 64]
	bool validation = false;
	uint8_t i;
	for (i = 1; i < 0b10000000; i <<= 1)
		if (bpb->BPB_SecPerClus == i)
		{
			validation = true;
			break;
		}
	if (!validation)
		return false;

	if (bpb->BPB_NumFATs != 2)
		return false;

	if (bpb->BPB_RootEntCnt != 0 || bpb->BPB_TotSec16 != 0 || bpb->BPB_FATSz16 != 0)
		return false;

	if (bpb->BPB_Media != 0xF8)
		return false;

	if (bpb->BPB_RootClus != 2)
		return false;

	if (bpb->bootEndSignature != 0xAA55)
		return false;

	return true;
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: �������� BIOS Parameter Block ������������� ���������� (SD �����)
//���������: BIOS_PARAMETER_BLOCK_STRUCTURE *bpb (��������� �� �������)
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************

static bool FS_getBIOSParameterBlock(BIOS_PARAMETER_BLOCK_STRUCTURE *bpb)
{
	if (!SD_readSingle512Block((uint8_t*) bpb, 0))
		return false;

	// ���� ��������� ��� ��� ����� �� ������������� ��������� 55AAh,
	// �������� ��������� �� ������: Missing operating system � ������� ���������������)
	if (bpb->bootEndSignature != 0xAA55)
		return false;

	// ���� ������ ���� ����� ������� SD ����� ����� 0xEB ��� 0xE9,
	// �� ��� ����������� ������.
	// � ��������� ������ ��� MBR, � ������� ������� � ���������� ��������� ��������.
	if (bpb->BS_jmpBoot[0] != 0xE9 && bpb->BS_jmpBoot[0] != 0xEB)
	{
		PARTITION_INFO_16_BYTE_LINE *partition = (PARTITION_INFO_16_BYTE_LINE*) ((MBR_INFO_STRUCTURE*) bpb)->partitionData; //first partition

		// ��� �������� � ������� �������� ���������������� ��� "����� �������� �������������� �������"
		long firstLogicalSector = partition->firstSector; //the unused sectors, hidden to the FAT

		if (!SD_readSingle512Block((uint8_t*) bpb, firstLogicalSector))
			return false;
	}

	if (!FS_validationMyFAT32(bpb))
		return false; // �� ��� ��������� ��������.		

	return true;
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ��������� ������������ ����� ������ MY_FILE (FAT32),
//������� ����� ���������� � ����� ���������� �� ������������ ����������.
//���������: unsigned long totalSectorsCount - ����� ��������� ��������
//unsigned char sectorPerCluster - ������ �������� � ��������
//return: unsigned long (������������ ����� ������,
//������� ����� ���������� �� ��; 0 ��� ������������ ����������)
// ******************************************************************

static uint32_t FS_calcMax10MBFilesCount(const uint32_t totalDataSectorsCount, const uint8_t sectorPerCluster)
{
	// ������������ ������, ������� �� ������� ��������,
	// ������ ���� ������������ ��� ���������� ������ ������
	// � ���������� ��������������� ��� ����� ������.
	// ���������� ����� ������ � ����� � ���������� ���������� 32 �����.
	//
	// �������� = ��������_��������_����������(������_������) + ��������_������	

	// ������ �� ������� �� 0
	if (sectorPerCluster == 0)
		return 0;

	uint32_t totalClustersCount = totalDataSectorsCount / sectorPerCluster;
	uint32_t clusterPerFile = FILE_10MB_IN_512_SECTORS_SIZE / sectorPerCluster;

	uint32_t maxFilesCount = totalClustersCount / clusterPerFile;

	// �����, ������� ������ ����� ��� ���������� ��������� �������� ����������
	uint32_t rootDirBytesCount = maxFilesCount * FAT32_SHORT_FILE_ENTRY_BYTES;
	// �����, ������� �������� ����� ��� ���������� ��������� �������� ����������
	uint32_t rootDirSectorsCount = MATH_DivisionCeiling(rootDirBytesCount, BYTE_PER_SECTOR);
	// �����, ������� ��������� ����� ��� ���������� ��������� �������� ����������
	uint32_t rootDirClustersCount = MATH_DivisionCeiling(rootDirSectorsCount, sectorPerCluster);

	// ���� �������� ������ ������ � �������� ���������� ��������� ������ ���������� ������������
	if (totalClustersCount < (rootDirClustersCount + maxFilesCount * clusterPerFile))
		--maxFilesCount; // �� ����� ���������� ������ �� ���� ����.

	return maxFilesCount;
}
//------------------------------------------------------------------------------

// ******************************************************************
// �������: ��������� ����������� ����������� ����� ������� ���������
// �������� ���������� (Root Directory), ����������� ��� ���������� �
// ��� ������ �������� (FAT 32 Byte Directory Entry) ������� ������
// ����������� totalSectorsCount ��� ������� �������� sectorPerCluster.
// ******************************************************************
// ���������� unsigned long ����������� ����� ������� ���������
// �������� ���������� (Root Directory).
// ******************************************************************

static uint16_t FS_calcMinMyFilesRootDirClusterChainLength(const uint32_t maxFilesCount, const uint8_t sectorPerCluster)
{
	// ������ �� ������� �� 0
	if (sectorPerCluster == 0)
		return 0;

	uint32_t bytesForRootDir = BYTE_PER_SHORT_DIR_ENTRY * (1 + maxFilesCount);

	uint16_t sectorsForRootDir = MATH_DivisionCeiling(bytesForRootDir, BYTE_PER_SECTOR);

	return MATH_DivisionCeiling(sectorsForRootDir, sectorPerCluster);
}
//------------------------------------------------------------------------------

static void FS_setFileEntryCrtTime(DIRECTORY_ENTRY_STRUCTURE *entry, const struct tm *time)
{
	// http://www.cplusplus.com/reference/ctime/

	/*
	 * Date Format. A FAT directory entry date stamp is a 16-bit field that is basically a date relative to the MS-DOS epoch of 01/01/1980. Here is the format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the 16-bit word):
	 * 
	 * Bits 0�4: Day of month, valid value range 1-31 inclusive.
	 * Bits 5�8: Month of year, 1 = January, valid value range 1�12 inclusive.
	 * Bits 9�15: Count of years from 1980, valid value range 0�127 inclusive (1980�2107).
	 */

	uint16_t crtDate = 0;

	crtDate += time->tm_mday; // int tm_mday; ���� ������ - [1,31]
	crtDate += (time->tm_mon + 1) << 5; // int tm_mon; ������ ����� ������ - [0,11]
	crtDate += (time->tm_year - 80) << 9; // int tm_year; ���� � 1900

	entry->DIR_CrtDate = crtDate;
	entry->DIR_WrtDate = crtDate;
	entry->DIR_LstAccDate = crtDate;

	/*
	 * DIR_CrtTimeTenth (1 byte)
	 * 
	 * Millisecond stamp at file creation time.
	 * This field actually contains a count of tenths of a second.
	 * The granularity of the seconds part of DIR_CrtTime is 2 seconds
	 * so this field is a count of tenths of a second and
	 * its valid value range is 0-199 inclusive.
	 */

	entry->DIR_CrtTimeTenth = (time->tm_sec % 2) * 100; // int tm_sec; ������� �� ������ ������ - [0,60]

	/*
	 * Time Format. A FAT directory entry time stamp is a 16-bit field that has a granularity of 2 seconds. Here is the format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the 16-bit word).
	 * 
	 * Bits 0�4: 2-second count, valid value range 0�29 inclusive (0 � 58 seconds).
	 * Bits 5�10: Minutes, valid value range 0�59 inclusive.
	 * Bits 11�15: Hours, valid value range 0�23 inclusive.
	 * 
	 * The valid time range is from Midnight 00:00:00 to 23:59:58. 
	 */

	uint16_t crtTime = 0;

	crtTime += time->tm_sec > 58 ? (58 / 2) : (time->tm_sec / 2); // int tm_sec; ������� �� ������ ������ - [0,60]
	crtTime += time->tm_min << 5; // int tm_min;	������ �� ������ ���� - [0,59]
	crtTime += time->tm_hour << 11; // int tm_hour;	���� �� �������� - [0,23]

	entry->DIR_CrtTime = crtTime;
	entry->DIR_WrtTime = crtTime;
}
//------------------------------------------------------------------------------

static void FS_setFileEntryWrtTime(DIRECTORY_ENTRY_STRUCTURE *entry, const struct tm *time)
{
	/*
	 * Date Format. A FAT directory entry date stamp is a 16-bit field that is basically a date relative to the MS-DOS epoch of 01/01/1980. Here is the format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the 16-bit word):
	 * 
	 * Bits 0�4: Day of month, valid value range 1-31 inclusive.
	 * Bits 5�8: Month of year, 1 = January, valid value range 1�12 inclusive.
	 * Bits 9�15: Count of years from 1980, valid value range 0�127 inclusive (1980�2107).
	 */

	uint16_t wrtDate = 0;

	wrtDate += time->tm_mday; // int tm_mday; ���� ������ - [1,31]
	wrtDate += (time->tm_mon + 1) << 5; // int tm_mon; ������ ����� ������ - [0,11]
	wrtDate += (time->tm_year - 80) << 9; // int tm_year; ���� � 1900

	entry->DIR_WrtDate = wrtDate;
	entry->DIR_LstAccDate = wrtDate;

	/*
	 * Time Format. A FAT directory entry time stamp is a 16-bit field that has a granularity of 2 seconds. Here is the format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the 16-bit word).
	 * 
	 * Bits 0�4: 2-second count, valid value range 0�29 inclusive (0 � 58 seconds).
	 * Bits 5�10: Minutes, valid value range 0�59 inclusive.
	 * Bits 11�15: Hours, valid value range 0�23 inclusive.
	 * 
	 * The valid time range is from Midnight 00:00:00 to 23:59:58. 
	 */

	uint16_t wrtTime = 0;

	wrtTime += time->tm_sec > 58 ? (58 / 2) : (time->tm_sec / 2); // int tm_sec; ������� �� ������ ������ - [0,60]
	wrtTime += time->tm_min << 5; // int tm_min;	������ �� ������ ���� - [0,59]
	wrtTime += time->tm_hour << 11; // int tm_hour;	���� �� �������� - [0,23]

	entry->DIR_WrtTime = wrtTime;
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: �������� LOGICAL_FAT32_DRIVE_INFO ��������� �����������
//������� MyFAT32.
//���������: LOGICAL_FAT32_DRIVE_INFO *fsInfo -
//���������� ��������� ���������� ���������
//return: bool (���� ������;
//true/false - ������������/�������������� ������� myFAT32)
// ******************************************************************

static void FS_createFileEntryUpdate(DIRECTORY_ENTRY_STRUCTURE *dirEntry, const LOGICAL_MY_FAT32_DRIVE_INFO *myInfo, const struct tm * time)
{
	sprintf(dirEntry->DIR_Name, "%08luBIN", myInfo->CUR_FileNameIndex % 100000000);

	dirEntry->DIR_Attr = ATTR_ARCHIVE; // | ATTR_READ_ONLY
	dirEntry->DIR_NTRes = 0;

	FS_setFileEntryCrtTime(dirEntry, time);

	uint32_t fileCluster =
			myInfo->CALC_FirstFileClusterNum +
			myInfo->CUR_FileRootDirIndex * myInfo->clusterPer10MbFile;
	dirEntry->DIR_FstClusLO = fileCluster;
	dirEntry->DIR_FstClusHI = fileCluster >> sizeof (uint16_t) * BIT_PER_BYTE;

	dirEntry->DIR_FileSize = 0; // FILE_10MB_IN_512_SECTORS_SIZE * BYTE_PER_SECTOR;	
}
//------------------------------------------------------------------------------

static void FS_writeFileEntryUpdate(DIRECTORY_ENTRY_STRUCTURE *dirEntry, const LOGICAL_MY_FAT32_DRIVE_INFO *myInfo, const struct tm *time)
{
	FS_setFileEntryWrtTime(dirEntry, time);

	dirEntry->DIR_FileSize =
			myInfo->CUR_SectorByteNum
			+ (myInfo->CUR_FilePhysicalSectorNum - myInfo->PH_ADDR_SectorFirstFile)
			% (myInfo->sectorPer10MbFile)
			* BYTE_PER_SECTOR;
}
//------------------------------------------------------------------------------

static void FS_driveFileEntryUpdate(DIRECTORY_ENTRY_STRUCTURE *dirEntry, const LOGICAL_MY_FAT32_DRIVE_INFO *myInfo, const struct tm *time)
{
	memset(dirEntry, 0, sizeof (DIRECTORY_ENTRY_STRUCTURE));
	sprintf(dirEntry->DIR_Name, "LOG_STORAGE");
	FS_setFileEntryWrtTime(dirEntry, time);
	dirEntry->DIR_LstAccDate = 0;

	dirEntry->DIR_Attr = ATTR_VOLUME_ID;
}
//------------------------------------------------------------------------------

static bool FS_stepToNextRootDirEntry(LOGICAL_MY_FAT32_DRIVE_INFO *myInfo)
{
	TIME_GetCompileTime(&currentTime);

	uint32_t indexOfOldEntryInRootDir = 1 + myInfo->CUR_FileRootDirIndex;

	uint8_t indexOfOldEntryInSector =
			indexOfOldEntryInRootDir % (BYTE_PER_SECTOR / sizeof (DIRECTORY_ENTRY_STRUCTURE));

	uint32_t indexOfOldSectorInRootDir =
			myInfo->PH_ADDR_SectorSecondCluster
			+ (indexOfOldEntryInRootDir * sizeof (DIRECTORY_ENTRY_STRUCTURE)) / BYTE_PER_SECTOR;

	DIRECTORY_ENTRY_STRUCTURE dirEntries[BYTE_PER_SECTOR / sizeof (DIRECTORY_ENTRY_STRUCTURE)];

	if (!SD_readSingle512Block((uint8_t*) dirEntries, indexOfOldSectorInRootDir))
		return false;
	
	FS_writeFileEntryUpdate(&dirEntries[indexOfOldEntryInSector], myInfo, &currentTime);

	++(myInfo->CUR_FileNameIndex);
	myInfo->CUR_FileNameIndex %= 100000000;

	++(myInfo->CUR_FilePhysicalSectorNum);

	uint32_t indexOfNewSectorInRootDir;
	if (++(myInfo->CUR_FileRootDirIndex) == myInfo->CALC_max10MBFilesCount)
	{
		// ����� ���������� ����� ��������� ����������� �������� ������:

		// ��� ������� ������� � ������ ������ ������
		// (� ������ ������ ������� �������� ����������)		
		if (!SD_writeSingle512Block((uint8_t*) dirEntries, indexOfOldSectorInRootDir))
			return false;

		myInfo->CUR_FilePhysicalSectorNum = myInfo->PH_ADDR_SectorFirstFile;
		myInfo->CUR_FileRootDirIndex = 0;

		indexOfNewSectorInRootDir = myInfo->PH_ADDR_SectorSecondCluster;

		if (!SD_readSingle512Block((uint8_t*) dirEntries, indexOfNewSectorInRootDir))
			return false;
		FS_createFileEntryUpdate(&dirEntries[1], myInfo, &currentTime);

	} else if ((indexOfOldEntryInSector + 1) == BYTE_PER_SECTOR / sizeof (DIRECTORY_ENTRY_STRUCTURE))
	{
		// �� �� ����� �������� ��������� ������ � ��� �� ������ RootDir:

		// ������ ��������� ������ ��������� �� � ��� �� ������� ����������,
		// ��� � ���������� (���������� �������� ��� ������ ����� ���������)
		if (!SD_writeSingle512Block((uint8_t*) dirEntries, indexOfOldSectorInRootDir))
			return false;

		// INFO: ������ ������ � ���������� ����� ���� ����������������
		// �������� ���������� ��� ����� ������ ������.

		indexOfNewSectorInRootDir = indexOfOldSectorInRootDir + 1;

		if (!SD_readSingle512Block((uint8_t*) dirEntries, indexOfNewSectorInRootDir))
			return false;
		FS_createFileEntryUpdate(&dirEntries[0], myInfo, &currentTime);

	} else
	{
		// �� ����� �������� ��������� ������ � ��� �� ������ RootDir:

		// ������ ��������� ������ ��������� � ��� �� ������� ����������,
		// ��� � ���������� (�������� �������� ��� ������ ����� ���������)
		FS_createFileEntryUpdate(&dirEntries[indexOfOldEntryInSector + 1], myInfo, &currentTime);

		indexOfNewSectorInRootDir = indexOfOldSectorInRootDir;
	}
	if (!SD_writeSingle512Block((uint8_t*) dirEntries, indexOfNewSectorInRootDir))
		return false;

	return true;
}
//------------------------------------------------------------------------------

// �������� ����� ����������� �� ��� � �������� ��������� �������

static bool FS_pushLoggingBuffer(LOGICAL_MY_FAT32_DRIVE_INFO *myInfo)
{
	uint16_t freeBytesIndex = myInfo->CUR_SectorByteNum;

	// �������� ��������� ����� ������
	memset(
			myInfo->CUR_LoggingBuffer + freeBytesIndex,
			0,
			BYTE_PER_SECTOR - freeBytesIndex);

	if (!SD_writeSingle512Block(myInfo->CUR_LoggingBuffer, myInfo->CUR_FilePhysicalSectorNum))
		return false;

	/* ������� ������� �� ��������� ������ ����������� ������ ��� */
	if (
			((myInfo->CUR_FilePhysicalSectorNum + 1
			- myInfo->PH_ADDR_SectorFirstFile)

			% myInfo->sectorPer10MbFile) == 0)
	{
		// ���� ����� ���������� �������
		// ������������ ������ ������� ������� �����
		// ������� �� ����� ����� � �������� ��� �������,
		// �� ��� ������, ��� �� ����� � ������� ������ ���������� �����.

		FS_stepToNextRootDirEntry(myInfo);
	} else
	{
		// ���� ��������� ������ ����������� �������� �����
		++(myInfo->CUR_FilePhysicalSectorNum);
	}

	/* ������� ������� �� �������� ���� ������ ������� */
	myInfo->CUR_SectorByteNum = 0;

	return true;
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: �������� ��������� ����� ��� ������ �� SD-����������
//(������������ �� ������ ����� ������������� �����, ������� ������������
//�� ������ ������ �����, ����� � ���� ������ �� ���������� ����� �����).
//���������: const uint8_t packet[] - ��������� �� ����� ��� ��������
//uint8_t pSize - ������ ������ ��� ��������
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************

bool FS_loggingPacket(const uint8_t packet[], uint8_t pSize)
{
	if (LogacalDriveInfo.CUR_SectorByteNum + pSize > BYTE_PER_SECTOR)
		if (!FS_pushLoggingBuffer(&LogacalDriveInfo))
			return false;

	memcpy(
			LogacalDriveInfo.CUR_LoggingBuffer + LogacalDriveInfo.CUR_SectorByteNum,
			packet,
			pSize);
	LogacalDriveInfo.CUR_SectorByteNum += pSize;

	if (LogacalDriveInfo.CUR_SectorByteNum == BYTE_PER_SECTOR)
		if (!FS_pushLoggingBuffer(&LogacalDriveInfo))
			return false;

	return true;
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ������������� ��������� 512-����� ������� �� �����������
//������, �� ������� ��������� ������ (� ���������� ��������� �����),
//� �������� ������ � LOGICAL_MY_FAT32_DRIVE_INFO.
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************

bool FS_saveBuffer()
{
	return FS_pushLoggingBuffer(&LogacalDriveInfo);
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ����������/���������� ��������� ������������� SD-����������,
//�� ������� ��������� ���������� FAT32-����������� �������� �������.
//���������: LOGICAL_MY_FAT32_DRIVE_INFO *myInfo - ������� ���������� 
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************

bool FS_getMyFAT32Info(LOGICAL_MY_FAT32_DRIVE_INFO *myInfo)
{
	BIOS_PARAMETER_BLOCK_STRUCTURE bpb;
	if (!FS_getBIOSParameterBlock(&bpb))
		return false;

	myInfo->PH_ADDR_SectorLogicalNull = bpb.BPB_HiddSec;
	myInfo->PH_ADDR_SectorFSInfo = bpb.BPB_HiddSec + bpb.BPB_FSInfo;

	myInfo->PH_ADDR_SectorFAT1 = bpb.BPB_HiddSec + bpb.BPB_RsvdSecCnt;
	myInfo->sectorPerFAT = bpb.BPB_FATSz32;
	myInfo->FAT_CountOfTables = bpb.BPB_NumFATs;

	if (myInfo->FAT_CountOfTables == 0 || myInfo->sectorPerFAT == 0)
		return false;

	// ������������ ������ ���������� ����� �� ������ �������� FAT (���� �� ���).
	// �� ��������� (� �������� ������ �������) ������ ��� ������ ����������
	// �������� ���������� ����������� �������.
	myInfo->PH_ADDR_SectorSecondCluster =
			myInfo->PH_ADDR_SectorFAT1 + myInfo->sectorPerFAT * myInfo->FAT_CountOfTables;

	myInfo->TOTAL_LogicalSectors = bpb.BPB_TotSec32;

	myInfo->sectorPerCluster = bpb.BPB_SecPerClus;
	myInfo->sectorPer10MbFile = FILE_10MB_IN_512_SECTORS_SIZE;

	// ������������ �������� ������, ��������� ��� ���������� ����� � ������
	// (� ��������� �� ����� ������������ ������ ����� ������ ���������)
	uint32_t logicalSectorsDataCount =
			myInfo->TOTAL_LogicalSectors -
			(myInfo->PH_ADDR_SectorSecondCluster - bpb.BPB_HiddSec);

	// ������������ ����� ������ FAT32 ������� CURRENT_FILE_512_SECTORS_SIZE,
	// ������� ����� ���������� �� ������� ������������ ����������.
	myInfo->CALC_max10MBFilesCount = FS_calcMax10MBFilesCount(
			logicalSectorsDataCount, bpb.BPB_SecPerClus);
	myInfo->CALC_totalRootDirClustersCount = FS_calcMinMyFilesRootDirClusterChainLength(
			myInfo->CALC_max10MBFilesCount, bpb.BPB_SecPerClus);

	if (myInfo->CALC_max10MBFilesCount == 0 || myInfo->CALC_totalRootDirClustersCount == 0)
		return false;

	// ������� ������� ����� ������ ������� (��� ��������������� � �������� ����������)
	uint32_t fatEntrysForFirstChain = (2 + myInfo->CALC_totalRootDirClustersCount);
	// ������� �������� ������� FAT32 ����� ������ �������
	uint32_t fatSectorsForFirstChain = MATH_DivisionCeiling(fatEntrysForFirstChain, 128);

	// ����� ������� ������������ �� �������� FAT32 �������� (128-�������),
	// ������� ����� ������ ������� �����
	myInfo->CALC_FirstFileClusterNum = fatSectorsForFirstChain * 128;
	myInfo->PH_ADDR_SectorFirstFile =
			myInfo->PH_ADDR_SectorSecondCluster +
			(myInfo->CALC_FirstFileClusterNum - 2) * myInfo->sectorPerCluster;

	myInfo->clusterPer10MbFile =
			FILE_10MB_IN_512_SECTORS_SIZE / myInfo->sectorPerCluster;

	return true;
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ��������� �������� ������� �� ������������
//FAT32-������������ ������� myFAT (�����������������),
//���������������� ������������� ���������� ������������ ������
//������������� ��� �������� ������������� ����� ������ MY_FILE
//(����� c �������� ������, ���������� �� 10 ��, ������� ���������
//������� ���������� � ���������� �� �������� ������ FAT).
//������ ����� ������������������ ������������� ������ ������.
//���������: LOGICAL_MY_FAT32_DRIVE_INFO *myInfo -
//���������� � ���������� �������
//return: MY_FAT32_ERROR (��� ���������� �������� ��
//������������/�������������� ������� myFAT)
// ******************************************************************

MY_FAT32_ERROR FS_checkMyFAT32(LOGICAL_MY_FAT32_DRIVE_INFO *fsInfo)
{
	// ���� ���������� ��������� ���������� �������� ����� �������������� ������,
	// �� ��� ������� ��������� ������ ���������� �������� �� ������� ����,
	// ������������� �� ������� FAT32 � ����������� ��� �������,
	// ��������������� � ��������� �� �������� ������� � ����� �� 10 ��.
	//
	// ���������� �� ����� ������� ����� ��������� ������� ��� ��������
	// � ����� ������� (������ ������, ���� �� ���, ����� ���������).	

	// ���������� ��������� ������ ����� ������� FAT,
	// �������� ������� �������� ����������,
	// �� ������� ������������ ������ �������
	uint32_t fatSectorN, fatSectorEntryN, fatAbsoluteEntryN;
	uint32_t fat[(BYTE_PER_SECTOR / sizeof (uint32_t))]; // ����� ��� ������ 512 �������

	uint8_t fatN;

	/*���� �1 (������): ������ ��������� � ������ ������� ��������� � �������� FAT32*/

	// ����������� ����� ������� long ��������� ������ ������� FAT � ��������� ��������,
	// ����������� ��� ���������� ����������� ���������� ������ MY_FILE
	// (� ������ ������ 10 �� ������).
	uint16_t firstChainOfClustersLength =
			2 + fsInfo->CALC_totalRootDirClustersCount;

	// ��� ������ ����� ������� FAT32
	for (fatN = 0; fatN < fsInfo->FAT_CountOfTables; ++fatN)
	{
		// ��� ������� ������� ������ FAT32
		for (fatSectorN = 0; /*����� ����� break*/; ++fatSectorN)
		{
			if (!SD_readSingle512Block(
					(uint8_t*) fat,
					fsInfo->PH_ADDR_SectorFAT1 + fatN * fsInfo->sectorPerFAT + fatSectorN))
				return MY_FAT32_ERROR_IO;

			// ������ ������ (FAT[0]) � ������� FAT32 ������� �� ��������� BPB_Media:
			//
			// "Microsoft Extensible Firmware Initiative FAT32 File System Specification":
			// The first reserved cluster, FAT[0], contains the BPB_Media byte value
			// in its low 8 bits, and all other bits are set to 1.
			// For example, if the BPB_Media value is 0xF8, for FAT12 FAT[0] = 0x0FF8,
			// for FAT16 FAT[0] = 0xFFF8, and for FAT32 FAT[0] = 0x0FFFFFF8.
			//
			// ������ ������ (FAT[1]) ������ ��� ����� ��������������:
			//
			// The second reserved cluster, FAT[1], is set by FORMAT to the EOC mark.
			// On FAT12 volumes, it is not used and is simply always contains an EOC mark.
			// For FAT16 and FAT32, the file system driver may use the high two bits
			// of the FAT[1] entry for dirty volume flags (all other bits, are always left set to 1).
			// Note that the bit location is different for FAT16 and FAT32,
			// because they are the high 2 bits of the entry.
			// For FAT32:
			// ClnShutBitMask  = 0x08000000; (1 - volume is �clean� / 0 - volume is �dirty�)
			// HrdErrBitMask   = 0x04000000; (1 - no disk read/write errors were encountered / 0 - the file system driver encountered a disk I/O error)

			// ��� ������ long ������ ������ FAT
			for (fatSectorEntryN = 0;
					fatSectorEntryN < (BYTE_PER_SECTOR / sizeof (uint32_t));
					++fatSectorEntryN, ++fatAbsoluteEntryN)
			{
				// ������ ��� ������ � ������� FAT32 �������� ������� ����������
				if (fatSectorN == 0 && fatSectorEntryN < 2)
				{
					if (fat[0] != FAT32_MARK_BPB_MEDIA || (fat[1] & 0x3FFFFFFF) != 0x3FFFFFFF)
						return MY_FAT32_ERROR_FORMAT;

					fatSectorEntryN = 2;
					fatAbsoluteEntryN = 2;
				}

				if (fatAbsoluteEntryN == (firstChainOfClustersLength - 1))
				{
					if (fat[fatSectorEntryN] != FAT32_MARK_EOC)
						return MY_FAT32_ERROR_FORMAT;

					// �������� ����� �� ����� ������ �����
					break;

				} else if (fat[fatSectorEntryN] != fatAbsoluteEntryN + 1)
				{
					return MY_FAT32_ERROR_FORMAT;
				}
			}

			if (fatAbsoluteEntryN == (firstChainOfClustersLength - 1))
				break;
		}
	}
	/*���� �1 (�����): ������ ��������� � ������ ������� ��������� � �������� FAT32*/

	// ���� �������� � ������ ������� (������� �������� ����������)
	// ������������� ������ �������, �� ������� ���������������� ������ �������,
	// ��������������� ��� ������ MY_FILE
	// (����������� ������������������ ������ �� 10 ��,
	// ������� ������� ��������� �� 512 �������� ������ FAT32)

	/*���� �2 (������): ������ ������� ��������� ������ MY_FILE � �������� FAT32*/
	uint32_t clusterPerFile = FILE_10MB_IN_512_SECTORS_SIZE / fsInfo->sectorPerCluster;
	uint32_t currentFileClusterEntry, myFilesCount;
	uint32_t fatSectorAfterRootDir = fatSectorN + 1;

	// ��� ������ ����� ������� FAT32
	for (fatN = 0; fatN < fsInfo->FAT_CountOfTables; ++fatN)
	{
		currentFileClusterEntry = 0, myFilesCount = 0;
		fatAbsoluteEntryN = fatSectorAfterRootDir * (BYTE_PER_SECTOR / sizeof (uint32_t));

		// ��� ������� ������� ������ FAT32,
		// ������� �� ���������� �� ���, �� ������� �� ������������ �����,
		// ��, �������� ������ �������, ������� ���������� �������� ���������� ������� �� 10 �� �����.
		for (fatSectorN = fatSectorAfterRootDir; /*����� ����� break*/; ++fatSectorN)
		{
			if (!SD_readSingle512Block(
					(uint8_t*) fat,
					fsInfo->PH_ADDR_SectorFAT1 + fatN * fsInfo->sectorPerFAT + fatSectorN))
				return MY_FAT32_ERROR_IO;

			for (fatSectorEntryN = 0; fatSectorEntryN < (BYTE_PER_SECTOR / sizeof (uint32_t)); ++fatSectorEntryN, ++fatAbsoluteEntryN)
			{
				++currentFileClusterEntry;
				if (currentFileClusterEntry == clusterPerFile)
				{
					if (fat[fatSectorEntryN] != FAT32_MARK_EOC)
					{
						return MY_FAT32_ERROR_FORMAT;
					} else
					{
						currentFileClusterEntry = 0;
						++myFilesCount;
						if (myFilesCount == fsInfo->CALC_max10MBFilesCount)
							break;

						// ��� ����� ������������ ������������� ���������� �� EOC ����� ������� ��������,
						// �������, �����, �� ������ ���������� ��� �������� ������ �� 512-����������� �������.
					}
				} else
				{
					if (fat[fatSectorEntryN] != (fatAbsoluteEntryN + 1))
						return MY_FAT32_ERROR_FORMAT;
				}
			}

			if (myFilesCount == fsInfo->CALC_max10MBFilesCount)
				break;
		}

		if (currentFileClusterEntry != 0 || myFilesCount != fsInfo->CALC_max10MBFilesCount)
			return MY_FAT32_ERROR_CALCULATION;
	}
	/*���� �2 (�����): ������ ������� ��������� ������ MY_FILE � �������� FAT32*/

	/*���� �3 (������): ������ ������ ������� �������� ����������*/
	// �������� ������ ������������� �������, ������ ������ �������� ����������
	// ������ ���� ����������� (������ ����� ���� ������ ��������� �� ���������
	// ��������������� ������� ���������).
	//
	// � ����� �������� �������������� �����c�� � ���������� ������� �������������,
	// ����� ������� ��� ������������ ����������� ��������� ��� ������������� ����,
	// ��� ��������� �������� ���������� ������������� ������ �������,
	// � ����� ���� ������� ��� ���� ��� ��������������.
	DIRECTORY_ENTRY_STRUCTURE *dirEntrys = (DIRECTORY_ENTRY_STRUCTURE*) fat;

	uint32_t chainRootDirSectorsCount =
			fsInfo->CALC_totalRootDirClustersCount * fsInfo->sectorPerCluster;

	uint32_t dataSectorN;
	uint8_t dataEntryN;

	uint32_t curClusterN, curMyFilesCount = 0;
	uint32_t fNameNum, fSize;

	// MY: �������� �����, ����������� �����
	for (dataSectorN = 0; dataSectorN < chainRootDirSectorsCount; ++dataSectorN)
	{
		// ��������� ������ ������ ��������� ��������...
		if (!SD_readSingle512Block(
				(uint8_t*) dirEntrys,
				fsInfo->PH_ADDR_SectorSecondCluster + dataSectorN))
			return MY_FAT32_ERROR_IO;

		uint32_t firstClusterOfFile;
		for (dataEntryN = 0;
				dataEntryN < (BYTE_PER_SECTOR / sizeof (DIRECTORY_ENTRY_STRUCTURE));
				++dataEntryN, ++curMyFilesCount, curClusterN += clusterPerFile)
		{

			if (dataSectorN == 0 && dataEntryN == 0)
				continue; // ���������� ������ � ������ ����������� �������

			// ���� ���� ����� (����� ����, ����� �������) �� ���������� ���...
			// TODO: �� ������ ���������� ��� ��������� ���������, ��������
			if (dataSectorN == 0 && dataEntryN == 1)
				curClusterN = fsInfo->CALC_FirstFileClusterNum;
			else if (dirEntrys[dataEntryN].DIR_CrtDate == 0)
				continue;

			firstClusterOfFile = (uint32_t) dirEntrys[dataEntryN].DIR_FstClusHI;
			firstClusterOfFile <<= sizeof (uint16_t) * BIT_PER_BYTE;
			firstClusterOfFile += (uint32_t) dirEntrys[dataEntryN].DIR_FstClusLO;

			if (curMyFilesCount > fsInfo->CALC_max10MBFilesCount)
				if (firstClusterOfFile != 0) // ���� ���� ������ ������ �� ����������� ���������
					return MY_FAT32_ERROR_FORMAT;

			if (firstClusterOfFile != curClusterN) // ���� ������ ������� ����� �� ����� ����������
				return MY_FAT32_ERROR_FORMAT;

			fNameNum = atoi(dirEntrys[dataEntryN].DIR_Name);
			if (fNameNum >= fsInfo->CUR_FileNameIndex) // ��������� ������, ���� ��� ����������
			{
				fsInfo->CUR_FileNameIndex = fNameNum;
				fsInfo->CUR_FileRootDirIndex = curMyFilesCount - 1;
				fSize = dirEntrys[dataEntryN].DIR_FileSize;
			}
		}
	}

	// ��������� ���������� ���������� �������
	fsInfo->CUR_FilePhysicalSectorNum =
			fsInfo->PH_ADDR_SectorFirstFile
			+ fsInfo->CUR_FileRootDirIndex * fsInfo->sectorPer10MbFile
			+ fSize / BYTE_PER_SECTOR;

	fsInfo->CUR_SectorByteNum = fSize % BYTE_PER_SECTOR;

	if (fsInfo->CUR_SectorByteNum != 0)
		if (!SD_readSingle512Block(fsInfo->CUR_LoggingBuffer, fsInfo->CUR_FilePhysicalSectorNum))
			return MY_FAT32_ERROR_IO;

	/*���� �3 (�����): ������ ������ ������� �������� ����������*/

	return MY_FAT32_ERROR_OK;
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ��������������� �������� ������� FAT32 � ������������ �
//FAT32-����������� �������� myFAT (����������������),
//��������������� ������������� ���������� ������������ ������
//������������� ��� �������� ������������� ����� ������ MY_FILE
//(����� c �������� ������, ���������� �� 10 ��, ������� ���������
//������� ���������� � ���������� �� �������� ������ FAT).
//������������� ������ ������ �� �������� �������.
//���������: LOGICAL_MY_FAT32_DRIVE_INFO *lDriveInfo -
//���������� � ���������� �������
//return: MY_FAT32_ERROR (��� ���������� �������� ��
//������������/�������������� ������� myFAT)
// ******************************************************************

MY_FAT32_ERROR FS_formattingFAT32ToMyFAT32(LOGICAL_MY_FAT32_DRIVE_INFO *myInfo)
{
	// ���������� ��������� ������ ����� ������� FAT,
	// �������� ������� �������� ����������,
	// �� ������� ������������ ������ �������
	uint32_t fatSectorN, fatSectorEntryN, fatAbsoluteEntryN;
	uint32_t fat[BYTE_PER_SECTOR / sizeof (uint32_t)]; // ����� ��� ������ 512 �������

	uint8_t fatN;

	/*���� �1 (������): �������������� ��������� � ������ ������� ��������� � �������� FAT32*/

	// ����������� ����� ������� long ��������� ������ ������� FAT � ��������� ��������,
	// ����������� ��� ���������� ����������� ���������� ������ MY_FILE
	// (� ������ ������ 10 �� ������).
	uint16_t firstChainOfClustersLength =
			2 + myInfo->CALC_totalRootDirClustersCount;

	for (fatSectorN = 0; /*����� ����� break*/; ++fatSectorN)
	{
		for (fatSectorEntryN = 0; fatSectorEntryN < (BYTE_PER_SECTOR / sizeof (uint32_t)); ++fatSectorEntryN, ++fatAbsoluteEntryN)
		{
			if (fatSectorN == 0 && fatSectorEntryN < 2)
			{
				fat[0] = FAT32_MARK_BPB_MEDIA;
				fat[1] = FAT32_MARK_FFFFFFFF;
				fatSectorEntryN = 2;
				fatAbsoluteEntryN = 2;
			}

			// �������� ����� �� ����� ������������� ������ �������

			// ���� ��� ��������� ������� �������, �� �������� ��� EOC,
			// � ���������� ����� ��������.
			if (fatAbsoluteEntryN == (firstChainOfClustersLength - 1))
			{
				// ������� firstChainOfClusters == 2 (������� ������� RootDir)
				// �� ������ ���� �������!
				// ������ �� �������� ������������ FS_getLogicalInfoMyFAT32().
				fat[fatSectorEntryN] = FAT32_MARK_EOC;

				// �� ����� �������� ���������� ����� �������,
				// � ��������� ��� �� ������, ��������� �� �����.
				memset((fat + (fatSectorEntryN + 1)), 0x00, 4 * (128 - fatSectorEntryN - 1));

				break;
			}

			fat[fatSectorEntryN] = fatAbsoluteEntryN + 1;
		}

		// ������ ������� �� ��� ����� ������� FAT
		for (fatN = 0; fatN < myInfo->FAT_CountOfTables; ++fatN)
			if (!SD_writeSingle512Block(
					(uint8_t*) fat,
					myInfo->PH_ADDR_SectorFAT1 + fatN * myInfo->sectorPerFAT + fatSectorN))
				return MY_FAT32_ERROR_IO;

		// �������� ����� �� ����� ������������� ������ �������

		// � ������� FAT ���������� ������� �� ��������� ������,		
		if (fatAbsoluteEntryN == (firstChainOfClustersLength - 1))
			break;
	}
	// �� ������ �� ��������������� �����,
	// fatSectorN � fatAbsoluteEntryN ������ ��������� �� ������ ������ ������� ������� FAT,
	// � �������� ������ �������� ������� ��������� ����� MY_FILE ������.

	/*���� �1 (�����): �������������� ��������� � ������ ������� ��������� � �������� FAT32*/

	/*���� �2 (������): �������������� ������� ��������� ������ MY_FILE � �������� FAT32*/
	uint32_t clusterPerFile = FILE_10MB_IN_512_SECTORS_SIZE / myInfo->sectorPerCluster;
	uint32_t currentFileClusterEntry = 0, myFilesCount = 0;


	// ������ fatAbsoluteEntryN  ����� ������ ���� ����������
	// �� ������ ���������� �������
	fatAbsoluteEntryN = fatAbsoluteEntryN % 128 ?
			MATH_DivisionCeiling(fatAbsoluteEntryN, 128) * 128 : fatAbsoluteEntryN;


	for (++fatSectorN; /*����� ����� break*/; ++fatSectorN)
	{
		for (fatSectorEntryN = 0; fatSectorEntryN < (512 / 4); ++fatSectorEntryN, ++fatAbsoluteEntryN)
		{
			++currentFileClusterEntry;
			if (currentFileClusterEntry == clusterPerFile) // ���� ������� �� ������� ���������� �����
			{
				fat[fatSectorEntryN] = FAT32_MARK_EOC;
				currentFileClusterEntry = 0;
				++myFilesCount;

				if (fatSectorEntryN != 127 && fatSectorEntryN != 63)
					return MY_FAT32_ERROR_CALCULATION;
			} else if (myFilesCount != myInfo->CALC_max10MBFilesCount) // ���� ���������� ������� �������� �����
			{
				fat[fatSectorEntryN] = fatAbsoluteEntryN + 1;
			} else // ���� ������� ������������� ����� ����� ������ (10Mb) ������
			{
				memset((fat + (fatSectorEntryN)), 0x00, 4 * (128 - fatSectorEntryN));
				break;
			}
		}

		// ������ ������� �� ��� ����� ������� FAT
		for (fatN = 0; fatN < myInfo->FAT_CountOfTables; ++fatN)
			if (!SD_writeSingle512Block(
					(uint8_t*) fat,
					myInfo->PH_ADDR_SectorFAT1 + fatN * myInfo->sectorPerFAT + fatSectorN))
				return MY_FAT32_ERROR_IO;

		// �������� �����...
		if (myFilesCount == myInfo->CALC_max10MBFilesCount)
			break;
	}
	/*���� �2 (�����): �������������� ������� ��������� ������ MY_FILE � �������� FAT32*/

	/*���� �3 (������): ������� ������� ������, ���������� ��� �������� ����������*/

	// �������� ������ ������� �����
	myInfo->CUR_FileNameIndex = 0;
	myInfo->CUR_FileRootDirIndex = 0;
	myInfo->CUR_FilePhysicalSectorNum = myInfo->PH_ADDR_SectorFirstFile;
	myInfo->CUR_SectorByteNum = 0;

	DIRECTORY_ENTRY_STRUCTURE *dEntry = (DIRECTORY_ENTRY_STRUCTURE*) fat;
	memset(dEntry, 0, BYTE_PER_SECTOR);

	TIME_GetCompileTime(&currentTime);
	FS_driveFileEntryUpdate(dEntry + 0, myInfo, &currentTime);
	FS_createFileEntryUpdate(dEntry + 1, myInfo, &currentTime);
	if (!SD_writeSingle512Block(
			(uint8_t*) dEntry,
			myInfo->PH_ADDR_SectorSecondCluster + 0))
		return MY_FAT32_ERROR_IO;

	// ������� ���������� ����� �������� ����������	
	uint8_t* nullBuf = (uint8_t*) fat;
	memset(nullBuf, 0, 512);
	uint32_t minRootDirSectorsCount = myInfo->CALC_totalRootDirClustersCount * myInfo->sectorPerCluster;

	uint32_t dataSectorN;
	for (dataSectorN = 1; dataSectorN < minRootDirSectorsCount; ++dataSectorN)
	{
		if (!SD_writeSingle512Block(
				nullBuf,
				myInfo->PH_ADDR_SectorSecondCluster + dataSectorN))
			return MY_FAT32_ERROR_IO;
	}
	/*���� �3 (�����): ������� ������� ������, ���������� ��� �������� ����������*/

	FAT32_FS_INFO *fsInfo = (FAT32_FS_INFO *) fat;

	if (!SD_readSingle512Block((uint8_t*) fsInfo, myInfo->PH_ADDR_SectorFSInfo))
		return MY_FAT32_ERROR_IO;
	fsInfo->FSI_Free_Count = 0xFFFFFFFF;
	fsInfo->FSI_Nxt_Free = 0xFFFFFFFF;
	if (!SD_writeSingle512Block((uint8_t*) fsInfo, myInfo->PH_ADDR_SectorFSInfo))
		return MY_FAT32_ERROR_IO;

	return MY_FAT32_ERROR_OK;
}
//------------------------------------------------------------------------------