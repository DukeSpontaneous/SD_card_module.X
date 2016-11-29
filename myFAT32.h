#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define BIT_PER_BYTE 8
#define BYTE_PER_SECTOR 512 // �������� ��������������� ������ ������� ������
#define BYTE_PER_SHORT_DIR_ENTRY 32 // ����� �������� ������ Directory Entry
#define FILE_10MB_IN_512_SECTORS_SIZE (10 * 0x800) // ��������� ������ ����� (10 �����)

#define FAT32_MAX_512_SECTORS_IN_CLUSTER 64 // �������� ������������� ������� �������� (� ��������)
#define FAT32_SHORT_FILE_ENTRY_BYTES 32 // ������ �������� ������ � ����� � ���������� (� ������)

typedef enum
{
	FAT32_MARK_BAD_CLUSTER = 0x0FFFFFF7, // ����� ������� ��������
	FAT32_MARK_BPB_MEDIA = 0x0FFFFFF8, //  ���� FAT32 FAT[0] (����� ������ ��������� ����� ��������)
	FAT32_MARK_EOC = 0x0FFFFFFF, // ����� "End Of Clusterchain"
	FAT32_MARK_FFFFFFFF = 0xFFFFFFFF
} FAT32_MARK;
//------------------------------------------------------------------------------

typedef enum
{
	MY_FAT32_ERROR_OK,
	MY_FAT32_ERROR_IO,
	MY_FAT32_ERROR_FORMAT,
	MY_FAT32_ERROR_CALCULATION
} MY_FAT32_ERROR;
//------------------------------------------------------------------------------

// ������� ���������� ����������� ����� �� ���������� ���������� (Partition Tables)

#pragma pack(push,1)	

typedef struct
{
	// https://ru.wikipedia.org/wiki/�������_�����������_������#.D0.A1.D1.82.D1.80.D1.83.D0.BA.D1.82.D1.83.D1.80.D0.B0_.D0.BE.D0.BF.D0.B8.D1.81.D0.B0.D0.BD.D0.B8.D1.8F_.D1.80.D0.B0.D0.B7.D0.B4.D0.B5.D0.BB.D0.B0
	uint8_t status; // ���� �������� (0x80 - �������� / 0x00 - ������� ������) - 1 ����

	uint8_t headStart; // ��������� ���������� ������ ������� (�������) - 1 ����
	uint16_t cylSectStart; // ��������� ���������� ������ ������� (������ � �������) - 2 �����

	uint8_t type; // ��� ���� ������� - 1 ����

	uint8_t headEnd; // �������� ���������� ������ ������� (�������) - 1 ����
	uint16_t cylSectEnd; // �������� ���������� ������ ������� (������ � �������) - 2 �����

	uint32_t firstSector; // ����� �������� �������������� ������� - 4 �����
	uint32_t sectorsTotal; // ����� ���������� �������� � ������ ������� - 4 �����
} PARTITION_INFO_16_BYTE_LINE;
#pragma pack(pop)
//------------------------------------------------------------------------------

// ��������� ������� ����������� ������ MBR (Master Boot Record),
// ��������������� � ������� 512-������� ������������ ������.

#pragma pack(push,1)

typedef struct
{
	// ��������� IPL 1 (��������� ���������� �������� ���� �� ������ 00h �� 1BEh)
	uint8_t nothing[446];

	// ������� ���������� ����������� ����� �� ���������� ���������� (Partition Tables)
	// (4 ������ �� 16 ���� = 64 �����) �������� ���� � ������ 1BEh �� 1FDh	
	uint8_t partitionData[64];

	// 2 ��������� ����� � ����� ������ ������� � ������ 1FE �� 1FF - �������� ��������� (Ending Signature)
	uint16_t signature; //0xAA55
	// 55�� - �������� ����� MBR. ����������� ���������� ���������� ����������
} MBR_INFO_STRUCTURE;
#pragma pack(pop)
//------------------------------------------------------------------------------

// ��������� ������������ �������; ������� � ������������ FAT32:
// "Microsoft Extensible Firmware Initiative // FAT32 File System Specification"
// P.S. ������������ ���������� ��������� (� ������ ����).

#pragma pack(push,1)

typedef struct
{
	/* MS Spec: Boot Sector and BPB Structure */

	uint8_t BS_jmpBoot[3]; // ������� JMP � NOP												// default: 0x009000EB	//	[0, 2]		*0x4D9058EB
	char BS_OEMName[8]; // �������� � ������ ��											// "MSDOS5.0"			//	[3, 10]

	// ��� ����� ������������ ������� �������� ��� BIOS Parameter Block (BPB) (���� ���������� BIOS).
	// ��� �������� ���������� �������������� �����, ������� MS-DOS � Windows ���������� ��� ������ ������������� �������.
	// ��������� ��� ���������� �������� ���� ����������, ������������ ������� ������,
	// ��� ��������� ������� FAT, �������� �������, ��� ���������� � ��������� ������� ������.

	// ������ ����� ���������� BIOS: DOS 7.1 EBPB (FAT32):

	// DOS 2.0 BPB
	uint16_t BPB_BytsPerSec; // ���������� ������ �� ������										// deafault: 512		//	[11, 12]	*0x0200		= 512
	uint8_t BPB_SecPerClus; // ���������� �������� �� ������� (������ ������ ���� � ������� �)						//	[13]		*0x08
	uint16_t BPB_RsvdSecCnt; // ���������� ����������������� �������� ����� ������ FAT									//	[14, 15]	*0x186E		= 6254 (��������� � ����, ��������� ��������������� ��� 6253: � ���������� ������� 6254 ���������� FAT1)
	// All Microsoft file system drivers will support a value other than 2,
	// but it is still highly recommended that no value other than 2 be used in this field
	uint8_t BPB_NumFATs; // ���������� ������ FAT																		//	[16]		*0x02
	uint16_t BPB_RootEntCnt; // ���������� ��������� � �������� �������� (������������ ������)	// must be 0 for FAT32	//	[17, 18]
	uint16_t BPB_TotSec16; // ����� ����� �������� FAT16										// must be 0 for FAT32	//	[19, 20]
	uint8_t BPB_Media; // ���������� �����					// � ������ ������ F8 (������ ���� � ����� ��������)		//	[21]
	uint16_t BPB_FATSz16; // ���������� �������� �� ������� ������� FAT16						// must be 0 for FAT32	//	[22, 23]

	// DOS 3.31 ���
	uint16_t BPB_SecPerTrk; // ���������� �������� �� �������													//	[24, 25]
	uint16_t BPB_NumHeads; // ����� �������																		//	[26, 27]
	uint32_t BPB_HiddSec; // ���������� ������� ��������														//	[28, 31]	*0x000000E9	= 233
	uint32_t BPB_TotSec32; // ����� ����� ��������, ���� ������ ����� ������ 32 ��								//	[32, 35]	*0x000F4317	= 1000215

	/* MS Spec: FAT32 Structure Starting at Offset 36 */

	// DOS 7.1 EBPB
	uint32_t BPB_FATSz32; // ����� ��������, ������� ����� FAT														//	[36, 39]	*0x000003C9	= 969
	uint16_t BPB_ExtFlags; // ������� ���� ������ (��� ���������� �������� ����)									//	[40, 41]
	uint16_t BPB_FSVer; // ������ // 0x0000 (defines version 0.0)													//	[42, 43]
	uint32_t BPB_RootClus; // ����� ������� ��������, ����������� �������� ��������� �� F��32-�����	// (=2)`	//	[44, 47]	*0x00000002

	// BPB_FSInfo:
	// ��������� �� ������ ����������� ������. � �� ���������� ���������� � ���,
	// ������� �� ����� ����� ���������, ������� �� ��� �������� � ����� ������� ��� ������� ����� ���������.
	// ����� �������, ����� �������� ��� ����� ������������ ����������, ������ �� ����� ��������� ��� ������� FAT.
	uint16_t BPB_FSInfo; // ��������� �� ������ FSinfo structure										// (=1)		//	[48, 49]
	uint16_t BPB_BkBootSec; // ����� ����������� �������, �������� ����� ������������					// 0x0006	//	[50, 51]
	uint8_t BPB_Reserved[12]; // ��������������� (��� ����� ��������)												//	[52, 63]
	uint8_t BS_DrvNum; // ����� ����������� �����							// 0x80 �������������� �������� ������	//	[64]
	uint8_t BS_Reserved1; // �����																				//	[65]
	uint8_t BS_BootSig; // ����������� ����������� ������						// ������ 0x29						//	[66]
	uint32_t BS_VolID; // �������� ����� ����																		//	[67, 70]
	char BS_VolLab[11]; // ����� ����										// "NO NAME    "					//	[71, 81]
	char BS_FilSysType[8]; // ��� �������� ������� (12- ��� 16-���������)	// "FAT32   "						//	[82, 89]

	uint8_t bootData[420]; //					//	[90, 509]
	uint16_t bootEndSignature; //0xAA55			//	[510, 511]
} BIOS_PARAMETER_BLOCK_STRUCTURE;
#pragma pack(pop)
//------------------------------------------------------------------------------

typedef enum
{
	FSI_SIG_LEAD = 0x41615252,
	FSI_SIG_STRUC = 0x61417272,
	FSI_SIG_TRAIL = 0xAA550000
} FSI_SIG;

// FAT32 FSInfo Sector Structure and Backup Boot Sector
#pragma pack(push,1)

typedef struct
{
	uint32_t FSI_LeadSig;
	uint8_t FSI_Reserved1[480];
	uint32_t FSI_StrucSig;
	uint32_t FSI_Free_Count;
	uint32_t FSI_Nxt_Free;
	uint8_t FSI_Reserved2[12];
	uint32_t FSI_TrailSig;
} FAT32_FS_INFO;
#pragma pack(pop)

typedef enum
{
	//	File attributes:
	ATTR_READ_ONLY = 0x01,
	ATTR_HIDDEN = 0x02,
	ATTR_SYSTEM = 0x04,
	ATTR_VOLUME_ID = 0x08,
	ATTR_DIRECTORY = 0x10,
	ATTR_ARCHIVE = 0x20, // Windows �����-�� �������� ��� ��� ������� �����
	ATTR_LONG_NAME = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID

	/*
	 * The upper two bits of the attribute byte are reserved
	 * and should always be set to 0 when a file is created
	 * and never modified or looked at after that.
	 */
} DIR_ATTR;

// Structure to access Directory Entry in the FAT; ������� � ������������ FAT32:
// "Microsoft Extensible Firmware Initiative // FAT32 File System Specification"

#pragma pack(push,1)

typedef struct
{
	char DIR_Name[11];

	uint8_t DIR_Attr; // DIR_ATTR file attributes

	// MY: �� ����� ���� ������ ������ � ������� �� ����... 
	uint8_t DIR_NTRes; //Reserved for use by Windows NT. Set value to 0 when a file is created and never modify or look at it after that.

	uint8_t DIR_CrtTimeTenth; // count of tenths of a second and its valid value range is 0-199 inclusive.

	// Bits 0�4: 2-second count, valid value range 0�29 inclusive (0 � 58 seconds).
	// Bits 5�10: Minutes, valid value range 0�59 inclusive.
	// Bits 11�15: Hours, valid value range 0�23 inclusive.
	// The valid time range is from Midnight 00:00:00 to 23:59:58.
	uint16_t DIR_CrtTime; //Time file was created.

	// Bits 0�4: Day of month, valid value range 1-31 inclusive.
	// Bits 5�8: Month of year, 1 = January, valid value range 1�12 inclusive.
	// Bits 9�15: Count of years from 1980, valid value range 0�127 inclusive (1980�2107).
	uint16_t DIR_CrtDate; //Date file was created.

	uint16_t DIR_LstAccDate; // Last access date. Note that there is no last access time, only a date. This is the date of last read or write. In the case of a write, this should be set to the same date as DIR_WrtDate.
	uint16_t DIR_FstClusHI; //higher word of the first cluster number
	uint16_t DIR_WrtTime; //Time of last write. Note that file creation is considered a write.
	uint16_t DIR_WrtDate; //Date of last write. Note that file creation is considered a write.
	uint16_t DIR_FstClusLO; //lower word of the first cluster number
	uint32_t DIR_FileSize; //32-bit DWORD holding this file�s size in bytes.
} DIRECTORY_ENTRY_STRUCTURE;
#pragma pack(pop)
//------------------------------------------------------------------------------

typedef struct
{
	// ��� ����� ����� ����� ����������� ��������� ���� �� ���������� �������,
	// ������� ������� ����� ���� ��� ���������� �������� ����������� ���������.

	uint32_t PH_ADDR_SectorLogicalNull;
	uint16_t PH_ADDR_SectorFSInfo;

	uint32_t PH_ADDR_SectorFAT1;
	uint16_t sectorPerFAT;
	uint8_t FAT_CountOfTables;

	uint32_t PH_ADDR_SectorSecondCluster;
	uint32_t CALC_FirstFileClusterNum;
	uint32_t PH_ADDR_SectorFirstFile;
	uint32_t TOTAL_LogicalSectors;

	uint8_t sectorPerCluster;
	uint32_t sectorPer10MbFile;
	uint32_t clusterPer10MbFile;

	uint32_t CALC_max10MBFilesCount;
	// ����������� ����� ������� ��������� �������� ���������� (RootDir), �����������
	// ��� ��������� MyFile ������, ����������� �� ��������� ������������ ������.
	uint16_t CALC_totalRootDirClustersCount;

	uint32_t CUR_FileNameIndex;	// ���������� � ����?
	uint16_t CUR_FileRootDirIndex;		// ���������� � ����?
	uint32_t CUR_FilePhysicalSectorNum;
	uint16_t CUR_SectorByteNum;
	
	uint8_t CUR_LoggingBuffer[BYTE_PER_SECTOR];
} LOGICAL_MY_FAT32_DRIVE_INFO;

extern LOGICAL_MY_FAT32_DRIVE_INFO LogacalDriveInfo;
//------------------------------------------------------------------------------

// ******************************************************************
//�������: �������� ��������� ����� ��� ������ �� SD-����������
//(������������ �� ������ ����� ������������� �����, ������� ������������
//�� ������ ������ �����, ����� � ���� ������ �� ���������� ����� �����).
//���������: const uint8_t packet[] - ��������� �� ����� ��� ��������
//uint8_t pSize - ������ ������ ��� ��������
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************
bool FS_loggingPacket(const uint8_t packet[], uint8_t pSize);
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ������������� ��������� 512-����� ������� �� �����������
//������, �� ������� ��������� ������ (� ���������� ��������� �����),
//� �������� ������ � LOGICAL_MY_FAT32_DRIVE_INFO.
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************
bool FS_saveBuffer();
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ����������/���������� ��������� ������������� SD-����������,
//�� ������� ��������� ���������� FAT32-����������� �������� �������.
//���������: LOGICAL_MY_FAT32_DRIVE_INFO *myInfo - ������� ���������� 
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************
bool FS_getMyFAT32Info(LOGICAL_MY_FAT32_DRIVE_INFO *lDriveInfo);
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
MY_FAT32_ERROR FS_checkMyFAT32(LOGICAL_MY_FAT32_DRIVE_INFO *lDriveInfo);
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
MY_FAT32_ERROR FS_formattingFAT32ToMyFAT32(LOGICAL_MY_FAT32_DRIVE_INFO *lDriveInfo);
//------------------------------------------------------------------------------