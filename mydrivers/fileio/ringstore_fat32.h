#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fileio.h"

// ����������� ������ ����� (� �����)
#define RINGSTORE_BIT_PER_BYTE 8
// �������� ������������� ������� �������� FAT32 (� ��������)
//#define RINGSTORE_MAX_512_SECTORS_IN_FAT32_CLUSTER 64

typedef enum
{
	FAT32_MARK_BAD_CLUSTER = 0x0FFFFFF7, // ����� ������� ��������
	FAT32_MARK_BPB_MEDIA = 0x0FFFFFF8, //  ���� FAT32 FAT[0] (����� ������ ��������� ����� ��������)
	FAT32_MARK_EOC = 0x0FFFFFFF, // ����� "End Of Clusterchain"
	FAT32_MARK_FFFFFFFF = 0xFFFFFFFF
} FAT32_MARK;

typedef struct
{
	// ��� ����� ����� ����� ����������� ��������� ���� �� ���������� �������,
	// ������� ������� ����� ���� ��� ���������� �������� ����������� ���������.

	uint32_t CALC_FirstFileClusterNum;
	uint32_t PH_ADDR_SectorFirstFile;

	uint32_t clusterPerFile;

	uint32_t CALC_maxFilesCount;
	// ����������� ����� ������� ��������� �������� ���������� (RootDir), �����������
	// ��� ��������� MyFile ������, ����������� �� ��������� ������������ ������.
	uint16_t CALC_totalRootDirClustersCount;

	uint32_t CUR_FileNameIndex; // ���������� � ����?
	uint16_t CUR_FileRootDirIndex; // ���������� � ����?
	uint32_t CUR_FilePhysicalSectorNum;
	uint16_t CUR_SectorByteNum;

	uint8_t CUR_LoggingBuffer[FILEIO_CONFIG_MEDIA_SECTOR_SIZE];
} RINGSTORE_OBJECT;
//------------------------------------------------------------------------------

// ******************************************************************
//�������: �������� ��������� ����� ��� ������ �� SD-����������
//(������������ �� ������ ����� ������������� �����, ������� ������������
//�� ������ ������ �����, ����� � ���� ������ �� ���������� ����� �����).
//���������: const uint8_t packet[] - ��������� �� ����� ��� ��������
//uint8_t pSize - ������ ������ ��� ��������
//return: bool (���� ������; ���������� true ��� ������� ����������)
// ******************************************************************
FILEIO_ERROR_TYPE RINGSTORE_StorePacket(RINGSTORE_OBJECT *rStore, const uint8_t packet[], uint8_t pSize);
//------------------------------------------------------------------------------

FILEIO_ERROR_TYPE RINGSTORE_Open(RINGSTORE_OBJECT *rStore, const FILEIO_DRIVE_CONFIG *driveConfig, void *mediaParameters);

FILEIO_ERROR_TYPE RINGSTORE_TryClose(RINGSTORE_OBJECT *rStore);