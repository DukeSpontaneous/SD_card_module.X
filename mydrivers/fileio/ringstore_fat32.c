#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ringstore_fat32_config.h"
#include "fileio_config.h"

#include "ringstore_fat32.h"

#include "fileio_private.h"
#include "sd_spi.h"

extern FILEIO_TimestampGet timestampGet;

extern struct
{
	FILEIO_DIRECTORY currentWorkingDirectory;
} globalParameters;

static FILEIO_DRIVE *drive = NULL;

static uint32_t MATH_DivisionCeiling(uint32_t divides, uint32_t divisor)
{
	return divides % divisor ?
			divides / divisor + 1 : divides / divisor;
}

// ******************************************************************
//�������: ��������� ������������ ����� ������ MY_FILE (FAT32),
//������� ����� ���������� � ����� ���������� �� ������������ ����������.
//���������: unsigned long totalSectorsCount - ����� ��������� ��������
//unsigned char sectorPerCluster - ������ �������� � ��������
//return: unsigned long (������������ ����� ������,
//������� ����� ���������� �� ��; 0 ��� ������������ ����������)
// ******************************************************************

static uint32_t RINGSTORE_CalcMaxFilesCount(const uint32_t totalDataSectorsCount, const uint8_t sectorPerCluster)
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
	uint32_t clusterPerFile = RINGSTORE_CONFIG_SECTORS_PER_FILE / sectorPerCluster;

	uint32_t maxFilesCount = totalClustersCount / clusterPerFile;

	// �����, ������� ������ ����� ��� ���������� ��������� �������� ����������
	uint32_t rootDirBytesCount = maxFilesCount * sizeof (FILEIO_DIRECTORY_ENTRY);
	// �����, ������� �������� ����� ��� ���������� ��������� �������� ����������
	uint32_t rootDirSectorsCount = MATH_DivisionCeiling(rootDirBytesCount, FILEIO_CONFIG_MEDIA_SECTOR_SIZE);
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

static uint16_t RINGSTORE_CalcMinRootDirClusterChainLength(const uint32_t maxFilesCount, const uint8_t sectorPerCluster)
{
	// ������ �� ������� �� 0
	if (sectorPerCluster == 0)
		return 0;

	uint32_t bytesForRootDir = sizeof (FILEIO_DIRECTORY_ENTRY) * (1 + maxFilesCount);

	uint16_t sectorsForRootDir = MATH_DivisionCeiling(bytesForRootDir, FILEIO_CONFIG_MEDIA_SECTOR_SIZE);

	return MATH_DivisionCeiling(sectorsForRootDir, sectorPerCluster);
}
//------------------------------------------------------------------------------

static void RINGSTORE_SetCrtTime(FILEIO_DIRECTORY_ENTRY *entry, const FILEIO_TIMESTAMP *time)
{
	entry->createDate = time->date.value;
	entry->writeDate = time->date.value;
	entry->accessDate = time->date.value;

	entry->createTimeMs = time->timeMs;

	entry->createTime = time->time.value;
	entry->writeTime = time->time.value;
}
//------------------------------------------------------------------------------

static void RINGSTORE_SetWrtTime(FILEIO_DIRECTORY_ENTRY *entry, const FILEIO_TIMESTAMP *time)
{
	entry->writeDate = time->date.value;
	entry->accessDate = time->date.value;

	entry->writeTime = time->time.value;
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

static void RINGSTORE_UpdateCreateFileEntry(FILEIO_DIRECTORY_ENTRY *dirEntry, const RINGSTORE_OBJECT *rStore, const FILEIO_TIMESTAMP *time)
{
	sprintf(dirEntry->name, "%08luBIN", rStore->CUR_FileNameIndex % 100000000);

	dirEntry->attributes = FILEIO_ATTRIBUTE_READ_ONLY | FILEIO_ATTRIBUTE_ARCHIVE;
	dirEntry->reserved0 = 0;

	RINGSTORE_SetCrtTime(dirEntry, time);

	uint32_t fileCluster =
			rStore->CALC_FirstFileClusterNum +
			rStore->CUR_FileRootDirIndex * rStore->clusterPerFile;
	dirEntry->firstClusterLow = fileCluster;
	dirEntry->firstClusterHigh = fileCluster >> sizeof (uint16_t) * RINGSTORE_BIT_PER_BYTE;

	dirEntry->fileSize = 0; // RINGSTORE_CONFIG_SECTORS_PER_FILE * FILEIO_CONFIG_MEDIA_SECTOR_SIZE;	
}
//------------------------------------------------------------------------------

static void RINGSTORE_UpdateWriteFileEntry(FILEIO_DIRECTORY_ENTRY *dirEntry, const RINGSTORE_OBJECT *rStore, const FILEIO_TIMESTAMP *time)
{
	RINGSTORE_SetWrtTime(dirEntry, time);

	dirEntry->fileSize =
			rStore->CUR_SectorByteNum
			+ (rStore->CUR_FilePhysicalSectorNum - rStore->PH_ADDR_SectorFirstFile)
			% RINGSTORE_CONFIG_SECTORS_PER_FILE
			* FILEIO_CONFIG_MEDIA_SECTOR_SIZE;
}
//------------------------------------------------------------------------------

static void RINGSTORE_UpdateDriveFileEntry(FILEIO_DIRECTORY_ENTRY *dirEntry, const RINGSTORE_OBJECT *rStore, const FILEIO_TIMESTAMP *time)
{
	memset(dirEntry, 0, sizeof (FILEIO_DIRECTORY_ENTRY));
	sprintf(dirEntry->name, "LOG_STORAGE");
	RINGSTORE_SetWrtTime(dirEntry, time);
	dirEntry->accessDate = 0;

	dirEntry->attributes = FILEIO_ATTRIBUTE_VOLUME;
}
//------------------------------------------------------------------------------

static FILEIO_ERROR_TYPE RINGSTORE_StepToNextFile(RINGSTORE_OBJECT *rStore)
{
	uint32_t indexOfOldEntryInRootDir = 1 + rStore->CUR_FileRootDirIndex;

	uint8_t indexOfOldEntryInSector =
			indexOfOldEntryInRootDir % (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (FILEIO_DIRECTORY_ENTRY));

	uint32_t indexOfOldSectorInRootDir =
			drive->firstDataSector
			+ (indexOfOldEntryInRootDir * sizeof (FILEIO_DIRECTORY_ENTRY)) / FILEIO_CONFIG_MEDIA_SECTOR_SIZE;

	FILEIO_DIRECTORY_ENTRY dirEntries[FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (FILEIO_DIRECTORY_ENTRY)];

	if (!FILEIO_SD_SectorRead(drive->mediaParameters,
			indexOfOldSectorInRootDir, (uint8_t*) dirEntries))
		return FILEIO_ERROR_BAD_SECTOR_READ;

	FILEIO_TIMESTAMP timeStamp;
	if (timestampGet != NULL)
		(*timestampGet)(&timeStamp);
	RINGSTORE_UpdateWriteFileEntry(&dirEntries[indexOfOldEntryInSector], rStore, &timeStamp);

	++(rStore->CUR_FileNameIndex);
	rStore->CUR_FileNameIndex %= 100000000;

	++(rStore->CUR_FilePhysicalSectorNum);

	uint32_t indexOfNewSectorInRootDir;
	if (++(rStore->CUR_FileRootDirIndex) == rStore->CALC_maxFilesCount)
	{
		// ����� ���������� ����� ��������� ����������� �������� ������:

		// ��� ������� ������� � ������ ������ ������
		// (� ������ ������ ������� �������� ����������)
		if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
				indexOfOldSectorInRootDir, (uint8_t*) dirEntries, false))
			return FILEIO_ERROR_WRITE;

		rStore->CUR_FilePhysicalSectorNum = rStore->PH_ADDR_SectorFirstFile;
		rStore->CUR_FileRootDirIndex = 0;

		indexOfNewSectorInRootDir = drive->firstDataSector;

		if (!FILEIO_SD_SectorRead(drive->mediaParameters,
				indexOfNewSectorInRootDir, (uint8_t*) dirEntries))
			return FILEIO_ERROR_BAD_SECTOR_READ;

		RINGSTORE_UpdateCreateFileEntry(&dirEntries[1], rStore, &timeStamp);

	} else if ((indexOfOldEntryInSector + 1) == FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (FILEIO_DIRECTORY_ENTRY))
	{
		// �� �� ����� �������� ��������� ������ � ��� �� ������ RootDir:

		// ������ ��������� ������ ��������� �� � ��� �� ������� ����������,
		// ��� � ���������� (���������� �������� ��� ������ ����� ���������)
		if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
				indexOfOldSectorInRootDir, (uint8_t*) dirEntries, false))
			return FILEIO_ERROR_WRITE;

		// INFO: ������ ������ � ���������� ����� ���� ����������������
		// �������� ���������� ��� ����� ������ ������.

		indexOfNewSectorInRootDir = indexOfOldSectorInRootDir + 1;

		if (!FILEIO_SD_SectorRead(drive->mediaParameters,
				indexOfNewSectorInRootDir, (uint8_t*) dirEntries))
			return FILEIO_ERROR_BAD_SECTOR_READ;

		RINGSTORE_UpdateCreateFileEntry(&dirEntries[0], rStore, &timeStamp);

	} else
	{
		// �� ����� �������� ��������� ������ � ��� �� ������ RootDir:

		// ������ ��������� ������ ��������� � ��� �� ������� ����������,
		// ��� � ���������� (�������� �������� ��� ������ ����� ���������)
		RINGSTORE_UpdateCreateFileEntry(&dirEntries[indexOfOldEntryInSector + 1], rStore, &timeStamp);

		indexOfNewSectorInRootDir = indexOfOldSectorInRootDir;
	}
	if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
			indexOfNewSectorInRootDir, (uint8_t*) dirEntries, false))
		return FILEIO_ERROR_WRITE;

	return FILEIO_ERROR_NONE;
}
//------------------------------------------------------------------------------

// �������� ����� ����������� �� ��� � �������� ��������� �������

static FILEIO_ERROR_TYPE RINGSTORE_StepToNextSector(RINGSTORE_OBJECT *rStore)
{
	uint16_t freeBytesIndex = rStore->CUR_SectorByteNum;

	// �������� ��������� ����� ������
	memset(
			rStore->CUR_LoggingBuffer + freeBytesIndex,
			0,
			FILEIO_CONFIG_MEDIA_SECTOR_SIZE - freeBytesIndex);

	if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
			rStore->CUR_FilePhysicalSectorNum, rStore->CUR_LoggingBuffer, false))
		return FILEIO_ERROR_WRITE;

	/* ������� ������� �� ��������� ������ ����������� ������ ��� */
	if (
			((rStore->CUR_FilePhysicalSectorNum + 1
			- rStore->PH_ADDR_SectorFirstFile)

			% RINGSTORE_CONFIG_SECTORS_PER_FILE) == 0)
	{
		// ���� ����� ���������� �������
		// ������������ ������ ������� ������� �����
		// ������� �� ����� ����� � �������� ��� �������,
		// �� ��� ������, ��� �� ����� � ������� ������ ���������� �����.

		FILEIO_ERROR_TYPE error = RINGSTORE_StepToNextFile(rStore);
		if (error != FILEIO_ERROR_NONE)
			return error;
	} else
	{
		// ���� ��������� ������ ����������� �������� �����
		++(rStore->CUR_FilePhysicalSectorNum);
	}

	/* ������� ������� �� �������� ���� ������ ������� */
	rStore->CUR_SectorByteNum = 0;

	return FILEIO_ERROR_NONE;
}
//------------------------------------------------------------------------------

static FILEIO_ERROR_TYPE RINGSTORE_SaveAndCloseFile(RINGSTORE_OBJECT *rStore)
{
	/*1 ������: ����� ���������� �������*/
	uint16_t freeBytesIndex = rStore->CUR_SectorByteNum;

	// �� ������ ������ �������� ��������� ����� ������������� ������
	memset(
			rStore->CUR_LoggingBuffer + freeBytesIndex,
			0,
			FILEIO_CONFIG_MEDIA_SECTOR_SIZE - freeBytesIndex);

	if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
			rStore->CUR_FilePhysicalSectorNum, rStore->CUR_LoggingBuffer, false))
		return FILEIO_ERROR_WRITE;
	/*1 �����: ����� ���������� �������*/
	
	/*2 ������: ����� ���������� ������*/
	uint32_t indexOfOldEntryInRootDir = 1 + rStore->CUR_FileRootDirIndex;

	uint8_t indexOfOldEntryInSector =
			indexOfOldEntryInRootDir % (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (FILEIO_DIRECTORY_ENTRY));

	uint32_t indexOfOldSectorInRootDir =
			drive->firstDataSector
			+ (indexOfOldEntryInRootDir * sizeof (FILEIO_DIRECTORY_ENTRY)) / FILEIO_CONFIG_MEDIA_SECTOR_SIZE;

	FILEIO_DIRECTORY_ENTRY dirEntries[FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (FILEIO_DIRECTORY_ENTRY)];

	if (!FILEIO_SD_SectorRead(drive->mediaParameters,
			indexOfOldSectorInRootDir, (uint8_t*) dirEntries))
		return FILEIO_ERROR_BAD_SECTOR_READ;

	FILEIO_TIMESTAMP timeStamp;
	if (timestampGet != NULL)
		(*timestampGet)(&timeStamp);
	RINGSTORE_UpdateWriteFileEntry(&dirEntries[indexOfOldEntryInSector], rStore, &timeStamp);
	
	if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
				indexOfOldSectorInRootDir, (uint8_t*) dirEntries, false))
			return FILEIO_ERROR_WRITE;
	/*2 �����: ����� ���������� ������*/
	
	return FILEIO_ERROR_NONE;
}

// ******************************************************************
//�������: ����������/���������� ��������� ������������� SD-����������,
//�� ������� ��������� ���������� FAT32-����������� �������� �������.
//���������: LOGICAL_MY_FAT32_DRIVE_INFO *myInfo - ������� ���������� 
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************

static FILEIO_ERROR_TYPE RINGSTORE_GetInfo(RINGSTORE_OBJECT *rStore)
{
	// ������������ �������� ������, ��������� ��� ���������� ����� � ������
	// (� ��������� �� ����� ������������ ������ ����� ������ ���������)
	uint32_t logicalSectorsDataCount = drive->partitionClusterCount * drive->sectorsPerCluster;

	// ������������ ����� ������ FAT32 ������� CURRENT_FILE_512_SECTORS_SIZE,
	// ������� ����� ���������� �� ������� ������������ ����������.
	rStore->CALC_maxFilesCount = RINGSTORE_CalcMaxFilesCount(
			logicalSectorsDataCount, drive->sectorsPerCluster);
	rStore->CALC_totalRootDirClustersCount = RINGSTORE_CalcMinRootDirClusterChainLength(
			rStore->CALC_maxFilesCount, drive->sectorsPerCluster);

	if (rStore->CALC_maxFilesCount == 0 || rStore->CALC_totalRootDirClustersCount == 0)
		return FILEIO_ERROR_INVALID_ARGUMENT;

	// ������� ������� ����� ������ ������� (��� ��������������� � �������� ����������)
	uint32_t fatEntrysForFirstChain = (2 + rStore->CALC_totalRootDirClustersCount);
	// ������� �������� ������� FAT32 ����� ������ �������
	uint32_t fatSectorsForFirstChain = MATH_DivisionCeiling(fatEntrysForFirstChain, 128);

	// ����� ������� ������������ �� �������� FAT32 �������� (128-�������),
	// ������� ����� ������ ������� �����
	rStore->CALC_FirstFileClusterNum = fatSectorsForFirstChain * 128;

	// TODO: ��� ������ ����� ���� �����-�� ������ (����������� �����������)
	rStore->PH_ADDR_SectorFirstFile =
			drive->firstDataSector +
			(rStore->CALC_FirstFileClusterNum - 2) * drive->sectorsPerCluster;

	rStore->clusterPerFile =
			RINGSTORE_CONFIG_SECTORS_PER_FILE / drive->sectorsPerCluster;

	return FILEIO_ERROR_NONE;
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

static FILEIO_ERROR_TYPE RINGSTORE_Check(RINGSTORE_OBJECT *rStore)
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
	uint32_t fat[FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t)]; // ����� ��� ������ 512 �������

	uint8_t fatN;

	/*���� �1 (������): ������ ��������� � ������ ������� ��������� � �������� FAT32*/

	// ����������� ����� ������� long ��������� ������ ������� FAT � ��������� ��������,
	// ����������� ��� ���������� ����������� ���������� ������ MY_FILE
	// (� ������ ������ 10 �� ������).
	uint16_t firstChainOfClustersLength =
			2 + rStore->CALC_totalRootDirClustersCount;

	// ��� ������ ����� ������� FAT32
	for (fatN = 0; fatN < drive->fatCopyCount; ++fatN)
	{
		// ��� ������� ������� ������ FAT32
		for (fatSectorN = 0; /*����� ����� break*/; ++fatSectorN)
		{
			if (!FILEIO_SD_SectorRead(drive->mediaParameters,
					drive->firstFatSector + fatN * drive->fatSectorCount + fatSectorN, (uint8_t*) fat))
				return FILEIO_ERROR_BAD_SECTOR_READ;

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
					fatSectorEntryN < (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t));
					++fatSectorEntryN, ++fatAbsoluteEntryN)
			{
				// ������ ��� ������ � ������� FAT32 �������� ������� ����������
				if (fatSectorN == 0 && fatSectorEntryN < 2)
				{
					if (fat[0] != FAT32_MARK_BPB_MEDIA || (fat[1] & 0x3FFFFFFF) != 0x3FFFFFFF)
						return FILEIO_ERROR_NOT_FORMATTED;

					fatSectorEntryN = 2;
					fatAbsoluteEntryN = 2;
				}

				if (fatAbsoluteEntryN == (firstChainOfClustersLength - 1))
				{
					if (fat[fatSectorEntryN] != FAT32_MARK_EOC)
						return FILEIO_ERROR_NOT_FORMATTED;

					// �������� ����� �� ����� ������ �����
					break;

				} else if (fat[fatSectorEntryN] != fatAbsoluteEntryN + 1)
				{
					return FILEIO_ERROR_NOT_FORMATTED;
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
	uint32_t clusterPerFile = RINGSTORE_CONFIG_SECTORS_PER_FILE / drive->sectorsPerCluster;
	uint32_t currentFileClusterEntry, myFilesCount;
	uint32_t fatSectorAfterRootDir = fatSectorN + 1;

	// ��� ������ ����� ������� FAT32
	for (fatN = 0; fatN < drive->fatCopyCount; ++fatN)
	{
		currentFileClusterEntry = 0, myFilesCount = 0;
		fatAbsoluteEntryN = fatSectorAfterRootDir * (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t));

		// ��� ������� ������� ������ FAT32,
		// ������� �� ���������� �� ���, �� ������� �� ������������ �����,
		// ��, �������� ������ �������, ������� ���������� �������� ���������� ������� �� 10 �� �����.
		for (fatSectorN = fatSectorAfterRootDir; /*����� ����� break*/; ++fatSectorN)
		{
			if (!FILEIO_SD_SectorRead(drive->mediaParameters,
					drive->firstFatSector + fatN * drive->fatSectorCount + fatSectorN, (uint8_t*) fat))
				return FILEIO_ERROR_BAD_SECTOR_READ;

			for (fatSectorEntryN = 0; fatSectorEntryN < (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t)); ++fatSectorEntryN, ++fatAbsoluteEntryN)
			{
				++currentFileClusterEntry;
				if (currentFileClusterEntry == clusterPerFile)
				{
					if (fat[fatSectorEntryN] != FAT32_MARK_EOC)
					{
						return FILEIO_ERROR_NOT_FORMATTED;
					} else
					{
						currentFileClusterEntry = 0;
						++myFilesCount;
						if (myFilesCount == rStore->CALC_maxFilesCount)
							break;

						// ��� ����� ������������ ������������� ���������� �� EOC ����� ������� ��������,
						// �������, �����, �� ������ ���������� ��� �������� ������ �� 512-����������� �������.
					}
				} else
				{
					if (fat[fatSectorEntryN] != (fatAbsoluteEntryN + 1))
						return FILEIO_ERROR_NOT_FORMATTED;
				}
			}

			if (myFilesCount == rStore->CALC_maxFilesCount)
				break;
		}
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
	FILEIO_DIRECTORY_ENTRY *dirEntrys = (FILEIO_DIRECTORY_ENTRY*) fat;

	uint32_t chainRootDirSectorsCount =
			rStore->CALC_totalRootDirClustersCount * drive->sectorsPerCluster;

	uint32_t dataSectorN;
	uint8_t dataEntryN;

	uint32_t curClusterN, curMyFilesCount = 0;
	uint32_t fNameNum, fSize;

	rStore->CUR_FileNameIndex = 0x00000000;
	bool nullEntryFlag = false;
	// MY: �������� �����, ����������� �����
	for (dataSectorN = 0; dataSectorN < chainRootDirSectorsCount; ++dataSectorN)
	{
		// ��������� ������ ������ ��������� ��������...
		if (!FILEIO_SD_SectorRead(drive->mediaParameters,
				drive->firstDataSector + dataSectorN, (uint8_t*) dirEntrys))
			return FILEIO_ERROR_BAD_SECTOR_READ;

		uint32_t firstClusterOfFile;
		for (dataEntryN = 0;
				dataEntryN < (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / FILEIO_DIRECTORY_ENTRY_SIZE);
				++dataEntryN, ++curMyFilesCount, curClusterN += clusterPerFile)
		{

			if (dataSectorN == 0 && dataEntryN == 0)
				continue; // ���������� ������ � ������ ����������� �������

			if (dataSectorN == 0 && dataEntryN == 1)
			{
				curClusterN = rStore->CALC_FirstFileClusterNum;
			} else if (dirEntrys[dataEntryN].name[0] == 0)
			{
				// ����������� ������ ������
				nullEntryFlag = true;
				continue;
			} else if (nullEntryFlag == true)
			{
				// ���� ���� � ����������� ����������� ����� �������...				
				return FILEIO_ERROR_NOT_FORMATTED;
			}

			firstClusterOfFile = (uint32_t) dirEntrys[dataEntryN].firstClusterHigh;
			firstClusterOfFile <<= sizeof (uint16_t) * RINGSTORE_BIT_PER_BYTE;
			firstClusterOfFile += (uint32_t) dirEntrys[dataEntryN].firstClusterLow;

			if (curMyFilesCount > rStore->CALC_maxFilesCount)
				if (firstClusterOfFile != 0) // ���� ���� ������ ������ �� ����������� ���������
					return FILEIO_ERROR_NOT_FORMATTED;

			if (firstClusterOfFile != curClusterN) // ���� ������ ������� ����� �� ����� ����������
				return FILEIO_ERROR_NOT_FORMATTED;

			fNameNum = atoi(dirEntrys[dataEntryN].name);
			if (fNameNum >= rStore->CUR_FileNameIndex) // ��������� ������, ���� ��� ����������
			{
				rStore->CUR_FileNameIndex = fNameNum;
				rStore->CUR_FileRootDirIndex = curMyFilesCount - 1;
				fSize = dirEntrys[dataEntryN].fileSize;
			}
		}
	}

	// ��������� ���������� ���������� �������
	rStore->CUR_FilePhysicalSectorNum =
			rStore->PH_ADDR_SectorFirstFile
			+ 1l * rStore->CUR_FileRootDirIndex * RINGSTORE_CONFIG_SECTORS_PER_FILE
			+ fSize / FILEIO_CONFIG_MEDIA_SECTOR_SIZE;

	rStore->CUR_SectorByteNum = fSize % FILEIO_CONFIG_MEDIA_SECTOR_SIZE;

	if (rStore->CUR_SectorByteNum != 0)
		if (!FILEIO_SD_SectorRead(drive->mediaParameters,
				rStore->CUR_FilePhysicalSectorNum, rStore->CUR_LoggingBuffer))
			return FILEIO_ERROR_BAD_SECTOR_READ;

	/*���� �3 (�����): ������ ������ ������� �������� ����������*/

	return FILEIO_ERROR_NONE;
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

static FILEIO_ERROR_TYPE RINGSTORE_FormattingTables(RINGSTORE_OBJECT *rStore)
{
	// ���������� ��������� ������ ����� ������� FAT,
	// �������� ������� �������� ����������,
	// �� ������� ������������ ������ �������
	uint32_t fatSectorN, fatSectorEntryN, fatAbsoluteEntryN;
	uint32_t fat[FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t)]; // ����� ��� ������ 512 �������

	uint8_t fatN;

	/*���� �1 (������): �������������� ��������� � ������ ������� ��������� � �������� FAT32*/

	// ����������� ����� ������� long ��������� ������ ������� FAT � ��������� ��������,
	// ����������� ��� ���������� ����������� ���������� ������ MY_FILE
	// (� ������ ������ 10 �� ������).
	uint16_t firstChainOfClustersLength =
			2 + rStore->CALC_totalRootDirClustersCount;

	for (fatSectorN = 0; /*����� ����� break*/; ++fatSectorN)
	{
		for (
				fatSectorEntryN = 0;
				fatSectorEntryN < (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t));
				++fatSectorEntryN, ++fatAbsoluteEntryN)
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
				memset(
						fat + (fatSectorEntryN + 1),
						0x00,
						FILEIO_CONFIG_MEDIA_SECTOR_SIZE - 4 * (fatSectorEntryN + 1));

				break;
			}

			fat[fatSectorEntryN] = fatAbsoluteEntryN + 1;
		}

		// ������ ������� �� ��� ����� ������� FAT
		for (fatN = 0; fatN < drive->fatSectorCount; ++fatN)
			if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
					drive->firstFatSector + fatN * drive->fatSectorCount + fatSectorN, (uint8_t*) fat, false))
				return FILEIO_ERROR_WRITE;

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
	uint32_t clusterPerFile = RINGSTORE_CONFIG_SECTORS_PER_FILE / drive->sectorsPerCluster;
	uint32_t currentFileClusterEntry = 0, myFilesCount = 0;


	// ������ fatAbsoluteEntryN  ����� ������ ���� ����������
	// �� ������ ���������� �������
	fatAbsoluteEntryN = fatAbsoluteEntryN % 128 ?
			MATH_DivisionCeiling(fatAbsoluteEntryN, 128) * 128 : fatAbsoluteEntryN;


	for (++fatSectorN; /*����� ����� break*/; ++fatSectorN)
	{
		for (
				fatSectorEntryN = 0;
				fatSectorEntryN < (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t));
				++fatSectorEntryN, ++fatAbsoluteEntryN)
		{
			++currentFileClusterEntry;
			if (currentFileClusterEntry == clusterPerFile) // ���� ������� �� ������� ���������� �����
			{
				fat[fatSectorEntryN] = FAT32_MARK_EOC;
				currentFileClusterEntry = 0;
				++myFilesCount;
			} else if (myFilesCount != rStore->CALC_maxFilesCount) // ���� ���������� ������� �������� �����
			{
				fat[fatSectorEntryN] = fatAbsoluteEntryN + 1;
			} else // ���� ������� ������������� ����� ����� ������ (10Mb) ������
			{
				memset(
						fat + fatSectorEntryN,
						0x00,
						FILEIO_CONFIG_MEDIA_SECTOR_SIZE - 4 * fatSectorEntryN);
				break;
			}
		}

		// ������ ������� �� ��� ����� ������� FAT
		for (fatN = 0; fatN < drive->fatSectorCount; ++fatN)
			if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
					drive->firstFatSector + fatN * drive->fatSectorCount + fatSectorN, (uint8_t*) fat, false))
				return FILEIO_ERROR_WRITE;

		// �������� �����...
		if (myFilesCount == rStore->CALC_maxFilesCount)
			break;
	}
	/*���� �2 (�����): �������������� ������� ��������� ������ MY_FILE � �������� FAT32*/

	/*���� �3 (������): ������� ������� ������, ���������� ��� �������� ����������*/

	// �������� ������ ������� �����
	rStore->CUR_FileNameIndex = 0;
	rStore->CUR_FileRootDirIndex = 0;
	rStore->CUR_FilePhysicalSectorNum = rStore->PH_ADDR_SectorFirstFile;
	rStore->CUR_SectorByteNum = 0;

	FILEIO_DIRECTORY_ENTRY *dEntry = (FILEIO_DIRECTORY_ENTRY*) fat;
	memset(dEntry, 0, FILEIO_CONFIG_MEDIA_SECTOR_SIZE);

	FILEIO_TIMESTAMP timeStamp;
	if (timestampGet != NULL)
		(*timestampGet)(&timeStamp);
	RINGSTORE_UpdateDriveFileEntry(dEntry + 0, rStore, &timeStamp);
	RINGSTORE_UpdateCreateFileEntry(dEntry + 1, rStore, &timeStamp);

	if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
			drive->firstDataSector + 0, (uint8_t*) dEntry, false))
		return FILEIO_ERROR_WRITE;

	// ������� ���������� ����� �������� ����������	
	uint8_t* nullBuf = (uint8_t*) fat;
	memset(nullBuf, 0, 512);
	uint32_t minRootDirSectorsCount = rStore->CALC_totalRootDirClustersCount * drive->sectorsPerCluster;

	uint32_t dataSectorN;
	for (dataSectorN = 1; dataSectorN < minRootDirSectorsCount; ++dataSectorN)
		if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
				drive->firstDataSector + dataSectorN, nullBuf, false))
			return FILEIO_ERROR_WRITE;
	/*���� �3 (�����): ������� ������� ������, ���������� ��� �������� ����������*/

	return FILEIO_ERROR_NONE;
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

FILEIO_ERROR_TYPE RINGSTORE_StorePacket(RINGSTORE_OBJECT *rStore, const uint8_t packet[], uint8_t pSize)
{
	FILEIO_ERROR_TYPE error;

	if (rStore->CUR_SectorByteNum + pSize > FILEIO_CONFIG_MEDIA_SECTOR_SIZE)
	{
		error = RINGSTORE_StepToNextSector(rStore);
		if (error != FILEIO_ERROR_NONE)
			return error;
	}

	memcpy(
			rStore->CUR_LoggingBuffer + rStore->CUR_SectorByteNum,
			packet,
			pSize);
	rStore->CUR_SectorByteNum += pSize;

	if (rStore->CUR_SectorByteNum == FILEIO_CONFIG_MEDIA_SECTOR_SIZE)
	{
		error = RINGSTORE_StepToNextSector(rStore);
		if (error != FILEIO_ERROR_NONE)
			return error;
	}

	return FILEIO_ERROR_NONE;
}
//------------------------------------------------------------------------------

FILEIO_ERROR_TYPE RINGSTORE_Open(RINGSTORE_OBJECT *rStore, const FILEIO_DRIVE_CONFIG *driveConfig, void *mediaParameters)
{
	FILEIO_ERROR_TYPE error;

	if (drive != NULL)
		if (FILEIO_DriveUnmount('A') != FILEIO_RESULT_SUCCESS)
			return FILEIO_ERROR_INIT_ERROR;

	error = FILEIO_DriveMount('A', driveConfig, mediaParameters);
	if (error != FILEIO_ERROR_NONE)
		return error;
	drive = globalParameters.currentWorkingDirectory.drive;

	if (drive->type != FILEIO_FILE_SYSTEM_TYPE_FAT32)
		return FILEIO_ERROR_UNSUPPORTED_FS;

	error = RINGSTORE_GetInfo(rStore);
	if (error != FILEIO_ERROR_NONE)
		return error;

	error = RINGSTORE_Check(rStore);
	switch (error)
	{
		case FILEIO_ERROR_NOT_FORMATTED:
			return RINGSTORE_FormattingTables(rStore);
		default:
			return error;
	}
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: 
//return: 
// ******************************************************************

FILEIO_ERROR_TYPE RINGSTORE_TryClose(RINGSTORE_OBJECT *rStore)
{
	FILEIO_ERROR_TYPE error = FILEIO_ERROR_NONE;
	
	if (drive != NULL)
	{		
		error = RINGSTORE_SaveAndCloseFile(rStore);
		
		drive = NULL;

		if (FILEIO_DriveUnmount('A') != FILEIO_RESULT_SUCCESS)
			return FILEIO_ERROR_UNINITIALIZED;
	} else
	{
		return FILEIO_ERROR_UNINITIALIZED;
	}

	return error;
}
//------------------------------------------------------------------------------