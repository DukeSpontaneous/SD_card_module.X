#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fileio.h"

// Стандартный размер байта (в битах)
#define RINGSTORE_BIT_PER_BYTE 8
// Стандарт максимального размера кластера FAT32 (в секторах)
//#define RINGSTORE_MAX_512_SECTORS_IN_FAT32_CLUSTER 64

typedef enum
{
	FAT32_MARK_BAD_CLUSTER = 0x0FFFFFF7, // Метка плохого кластера
	FAT32_MARK_BPB_MEDIA = 0x0FFFFFF8, //  Байт FAT32 FAT[0] (почти всегда равняется этому значению)
	FAT32_MARK_EOC = 0x0FFFFFFF, // Метка "End Of Clusterchain"
	FAT32_MARK_FFFFFFFF = 0xFFFFFFFF
} FAT32_MARK;

typedef struct
{
	// При такой схеме можно практически полностью уйти от кластерной системы,
	// которая остаётся нужна лишь для реализации браковки неисправных кластеров.

	uint32_t CALC_FirstFileClusterNum;
	uint32_t PH_ADDR_SectorFirstFile;

	uint32_t clusterPerFile;

	uint32_t CALC_maxFilesCount;
	// Минимальная длина цепочки кластеров корневой директории (RootDir), необходимая
	// для адресации MyFile файлов, заполняющих всё доступное пространство памяти.
	uint16_t CALC_totalRootDirClustersCount;

	uint32_t CUR_FileNameIndex; // Индексация с нуля?
	uint16_t CUR_FileRootDirIndex; // Индексация с нуля?
	uint32_t CUR_FilePhysicalSectorNum;
	uint16_t CUR_SectorByteNum;

	uint8_t CUR_LoggingBuffer[FILEIO_CONFIG_MEDIA_SECTOR_SIZE];
} RINGSTORE_OBJECT;
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: передать очередной пакет для записи на SD-накопитель
//(отправляется на запись через промежуточный буфер, который отправляется
//на запись только тогда, когда в него больше не помещается новый пакет).
//Аргументы: const uint8_t packet[] - указатель на пакет для передачи
//uint8_t pSize - размер пакета для передачи
//return: bool (флаг успеха; возвращает true при штатном завершении)
// ******************************************************************
FILEIO_ERROR_TYPE RINGSTORE_StorePacket(RINGSTORE_OBJECT *rStore, const uint8_t packet[], uint8_t pSize);
//------------------------------------------------------------------------------

FILEIO_ERROR_TYPE RINGSTORE_Open(RINGSTORE_OBJECT *rStore, const FILEIO_DRIVE_CONFIG *driveConfig, void *mediaParameters);

FILEIO_ERROR_TYPE RINGSTORE_TryClose(RINGSTORE_OBJECT *rStore);