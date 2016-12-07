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
//Функция: вычислить максимальное число файлов MY_FILE (FAT32),
//которые можно разместить в одной директории на запоминающем устройстве.
//Аргументы: unsigned long totalSectorsCount - число доступных секторов
//unsigned char sectorPerCluster - размер кластера в секторах
//return: unsigned long (максимальное число файлов,
//которое можно разместить на ЗУ; 0 при недопустимых аргументах)
// ******************************************************************

static uint32_t RINGSTORE_CalcMaxFilesCount(const uint32_t totalDataSectorsCount, const uint8_t sectorPerCluster)
{
	// Пространство памяти, начиная со второго кластера,
	// должно быть использовано для размещения списка файлов
	// и размещения непосредственно тел самих файлов.
	// Кратчайшая форма записи о файле в директории составляет 32 байта.
	//
	// КЛАСТЕРЫ = КЛАСТЕРЫ_КОРНЕВОЙ_ДИРЕКТОРИИ(СПИСОК_ФАЙЛОВ) + КЛАСТЕРЫ_ДАННЫХ	

	// Защита от деления на 0
	if (sectorPerCluster == 0)
		return 0;

	uint32_t totalClustersCount = totalDataSectorsCount / sectorPerCluster;
	uint32_t clusterPerFile = RINGSTORE_CONFIG_SECTORS_PER_FILE / sectorPerCluster;

	uint32_t maxFilesCount = totalClustersCount / clusterPerFile;

	// Узнаём, сколько байтов нужно для размещения требуемой корневой директории
	uint32_t rootDirBytesCount = maxFilesCount * sizeof (FILEIO_DIRECTORY_ENTRY);
	// Узнаём, сколько секторов нужно для размещения требуемой корневой директории
	uint32_t rootDirSectorsCount = MATH_DivisionCeiling(rootDirBytesCount, FILEIO_CONFIG_MEDIA_SECTOR_SIZE);
	// Узнаём, сколько кластеров нужно для размещения требуемой корневой директории
	uint32_t rootDirClustersCount = MATH_DivisionCeiling(rootDirSectorsCount, sectorPerCluster);

	// Если итоговый размер файлов и корневой директории превышает размер доступного пространства
	if (totalClustersCount < (rootDirClustersCount + maxFilesCount * clusterPerFile))
		--maxFilesCount; // то можно разместить меньше на один файл.

	return maxFilesCount;
}
//------------------------------------------------------------------------------

// ******************************************************************
// Функция: вычислить минимальную необходимую длину цепочки кластеров
// корневой директории (Root Directory), необходимую для размещения в
// ней списка коротких (FAT 32 Byte Directory Entry) записей файлов
// количеством totalSectorsCount при размере кластера sectorPerCluster.
// ******************************************************************
// Возвращает unsigned long необходимую длину цепочки кластеров
// корневой директории (Root Directory).
// ******************************************************************

static uint16_t RINGSTORE_CalcMinRootDirClusterChainLength(const uint32_t maxFilesCount, const uint8_t sectorPerCluster)
{
	// Защита от деления на 0
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
//Функция: получить LOGICAL_FAT32_DRIVE_INFO структуру логического
//разрела MyFAT32.
//Аргументы: LOGICAL_FAT32_DRIVE_INFO *fsInfo -
//физическая адресация логических элементов
//return: bool (флаг успеха;
//true/false - соответствие/несоответствие формату myFAT32)
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
		// Номер очередного файла превышает вычисленный максимум файлов:

		// Нам следует перейти в начало кольца файлов
		// (в начало кольца записей корневой директории)
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
		// Мы не можем добавить сделующую запись в тот же сектор RootDir:

		// Значит очередная запись находится НЕ В ТОМ ЖЕ секторе директории,
		// что и предыдущая (невозможно обновить обе записи одним действием)
		if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
				indexOfOldSectorInRootDir, (uint8_t*) dirEntries, false))
			return FILEIO_ERROR_WRITE;

		// INFO: пустые записи в директории могут быть интерпретированы
		// файловым менеджером как конец списка файлов.

		indexOfNewSectorInRootDir = indexOfOldSectorInRootDir + 1;

		if (!FILEIO_SD_SectorRead(drive->mediaParameters,
				indexOfNewSectorInRootDir, (uint8_t*) dirEntries))
			return FILEIO_ERROR_BAD_SECTOR_READ;

		RINGSTORE_UpdateCreateFileEntry(&dirEntries[0], rStore, &timeStamp);

	} else
	{
		// Мы можем добавить сделующую запись в тот же сектор RootDir:

		// Значит очередная запись находится В ТОМ ЖЕ секторе директории,
		// что и предыдущая (возможно обновить обе записи одним действием)
		RINGSTORE_UpdateCreateFileEntry(&dirEntries[indexOfOldEntryInSector + 1], rStore, &timeStamp);

		indexOfNewSectorInRootDir = indexOfOldSectorInRootDir;
	}
	if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
			indexOfNewSectorInRootDir, (uint8_t*) dirEntries, false))
		return FILEIO_ERROR_WRITE;

	return FILEIO_ERROR_NONE;
}
//------------------------------------------------------------------------------

// Записать буфер логирования на ПЗУ и обновить положение курсора

static FILEIO_ERROR_TYPE RINGSTORE_StepToNextSector(RINGSTORE_OBJECT *rStore)
{
	uint16_t freeBytesIndex = rStore->CUR_SectorByteNum;

	// Обнулить свободную часть буфера
	memset(
			rStore->CUR_LoggingBuffer + freeBytesIndex,
			0,
			FILEIO_CONFIG_MEDIA_SECTOR_SIZE - freeBytesIndex);

	if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
			rStore->CUR_FilePhysicalSectorNum, rStore->CUR_LoggingBuffer, false))
		return FILEIO_ERROR_WRITE;

	/* Перевод курсора на следующий сектор логического кольца ПЗУ */
	if (
			((rStore->CUR_FilePhysicalSectorNum + 1
			- rStore->PH_ADDR_SectorFirstFile)

			% RINGSTORE_CONFIG_SECTORS_PER_FILE) == 0)
	{
		// Если номер очередного сектора
		// относительно номера сектора первого файла
		// делится на длину файла в секторах без остатка,
		// то это значит, что мы вошли в нулевой сектор следующего файла.

		FILEIO_ERROR_TYPE error = RINGSTORE_StepToNextFile(rStore);
		if (error != FILEIO_ERROR_NONE)
			return error;
	} else
	{
		// Если следующий сектор принадлежит текущему файлу
		++(rStore->CUR_FilePhysicalSectorNum);
	}

	/* Перевод курсора на исходный байт нового сектора */
	rStore->CUR_SectorByteNum = 0;

	return FILEIO_ERROR_NONE;
}
//------------------------------------------------------------------------------

static FILEIO_ERROR_TYPE RINGSTORE_SaveAndCloseFile(RINGSTORE_OBJECT *rStore)
{
	/*1 НАЧАЛО: Часть сохранения сектора*/
	uint16_t freeBytesIndex = rStore->CUR_SectorByteNum;

	// На всякий случай обнулить свободную часть записываемого буфера
	memset(
			rStore->CUR_LoggingBuffer + freeBytesIndex,
			0,
			FILEIO_CONFIG_MEDIA_SECTOR_SIZE - freeBytesIndex);

	if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
			rStore->CUR_FilePhysicalSectorNum, rStore->CUR_LoggingBuffer, false))
		return FILEIO_ERROR_WRITE;
	/*1 КОНЕЦ: Часть сохранения сектора*/
	
	/*2 НАЧАЛО: Часть обновления записи*/
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
	/*2 КОНЕЦ: Часть обновления записи*/
	
	return FILEIO_ERROR_NONE;
}

// ******************************************************************
//Функция: рассчитать/установить параметры подключенного SD-накопителя,
//на котором ожидается обнаружить FAT32-совместимую файловую систему.
//Аргументы: LOGICAL_MY_FAT32_DRIVE_INFO *myInfo - приёмник параметров 
//return: bool (флаг успеха; возвращает true при штатном завершении)
// ******************************************************************

static FILEIO_ERROR_TYPE RINGSTORE_GetInfo(RINGSTORE_OBJECT *rStore)
{
	// Пространство секторов памяти, доступное для размещения папок и файлов
	// (с поправкой на вычет пространства памяти перед вторым кластером)
	uint32_t logicalSectorsDataCount = drive->partitionClusterCount * drive->sectorsPerCluster;

	// Максимальное число файлов FAT32 размера CURRENT_FILE_512_SECTORS_SIZE,
	// которое можно разместить на целевом записывающем устройстве.
	rStore->CALC_maxFilesCount = RINGSTORE_CalcMaxFilesCount(
			logicalSectorsDataCount, drive->sectorsPerCluster);
	rStore->CALC_totalRootDirClustersCount = RINGSTORE_CalcMinRootDirClusterChainLength(
			rStore->CALC_maxFilesCount, drive->sectorsPerCluster);

	if (rStore->CALC_maxFilesCount == 0 || rStore->CALC_totalRootDirClustersCount == 0)
		return FILEIO_ERROR_INVALID_ARGUMENT;

	// Сколько записей нужно первой цепочке (две зерезервированы и корневая директория)
	uint32_t fatEntrysForFirstChain = (2 + rStore->CALC_totalRootDirClustersCount);
	// Сколько секторов таблицы FAT32 нужно первой цепочке
	uint32_t fatSectorsForFirstChain = MATH_DivisionCeiling(fatEntrysForFirstChain, 128);

	// Номер первого выравненного по секторам FAT32 кластера (128-кратный),
	// который можно отдать первому файлу
	rStore->CALC_FirstFileClusterNum = fatSectorsForFirstChain * 128;

	// TODO: тут скорее всего есть какая-то ошибка (вычисляется неправильно)
	rStore->PH_ADDR_SectorFirstFile =
			drive->firstDataSector +
			(rStore->CALC_FirstFileClusterNum - 2) * drive->sectorsPerCluster;

	rStore->clusterPerFile =
			RINGSTORE_CONFIG_SECTORS_PER_FILE / drive->sectorsPerCluster;

	return FILEIO_ERROR_NONE;
}
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: проверить файловую систему на соответствие
//FAT32-совместимому формату myFAT (пользовательскому),
//подразумевающему использование доступного пространства памяти
//исключительно для хранения максимального числа файлов MY_FILE
//(файлы c коротким именем, занимающие по 10 Мб, цепочки кластеров
//которых непрерывны и выровненны по секторам таблиц FAT).
//Помимо всего вышеперечисленного устанавливает курсор записи.
//Аргументы: LOGICAL_MY_FAT32_DRIVE_INFO *myInfo -
//информация о логическом разделе
//return: MY_FAT32_ERROR (код завершения проверки на
//соответствие/несоответствие формату myFAT)
// ******************************************************************

static FILEIO_ERROR_TYPE RINGSTORE_Check(RINGSTORE_OBJECT *rStore)
{
	// Если необходимо сохранять содержимое носителя между перезагрузками модуля,
	// то при запуске программы должна проводится проверка на предмет того,
	// форматирована ли таблица FAT32 в необходимом нам формате,
	// подразумевающем её разбиение на корневой каталог и файлы по 10 Мб.
	//
	// Отклонение от этого формата будет считаться поводом для перехода
	// к этому формату (старый формат, если он был, будет уничтожен).	

	// Посекторно проверяем первую часть таблицы FAT,
	// хранящую цепочку корневой директории,
	// на предмет соответствия нашему формату
	uint32_t fatSectorN, fatSectorEntryN, fatAbsoluteEntryN;
	uint32_t fat[FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t)]; // Буфер для чтения 512 сектора

	uint8_t fatN;

	/*ФАЗА №1 (НАЧАЛО): Анализ префиксов и первой цепочки кластеров в таблицах FAT32*/

	// Минимальная длина цепочки long стартовых байтов таблицы FAT и корневого каталога,
	// необходимая для размещения максимально возможного списка MY_FILE
	// (в данном случае 10 Мб файлов).
	uint16_t firstChainOfClustersLength =
			2 + rStore->CALC_totalRootDirClustersCount;

	// Для каждой копии таблицы FAT32
	for (fatN = 0; fatN < drive->fatCopyCount; ++fatN)
	{
		// Для каждого сектора таблиц FAT32
		for (fatSectorN = 0; /*выход через break*/; ++fatSectorN)
		{
			if (!FILEIO_SD_SectorRead(drive->mediaParameters,
					drive->firstFatSector + fatN * drive->fatSectorCount + fatSectorN, (uint8_t*) fat))
				return FILEIO_ERROR_BAD_SECTOR_READ;

			// Первая запись (FAT[0]) в цепочке FAT32 зависит от параметра BPB_Media:
			//
			// "Microsoft Extensible Firmware Initiative FAT32 File System Specification":
			// The first reserved cluster, FAT[0], contains the BPB_Media byte value
			// in its low 8 bits, and all other bits are set to 1.
			// For example, if the BPB_Media value is 0xF8, for FAT12 FAT[0] = 0x0FF8,
			// for FAT16 FAT[0] = 0xFFF8, and for FAT32 FAT[0] = 0x0FFFFFF8.
			//
			// Вторая запись (FAT[1]) хранит два флага предупреждений:
			//
			// The second reserved cluster, FAT[1], is set by FORMAT to the EOC mark.
			// On FAT12 volumes, it is not used and is simply always contains an EOC mark.
			// For FAT16 and FAT32, the file system driver may use the high two bits
			// of the FAT[1] entry for dirty volume flags (all other bits, are always left set to 1).
			// Note that the bit location is different for FAT16 and FAT32,
			// because they are the high 2 bits of the entry.
			// For FAT32:
			// ClnShutBitMask  = 0x08000000; (1 - volume is “clean” / 0 - volume is “dirty”)
			// HrdErrBitMask   = 0x04000000; (1 - no disk read/write errors were encountered / 0 - the file system driver encountered a disk I/O error)

			// Для каждой long записи таблиц FAT
			for (fatSectorEntryN = 0;
					fatSectorEntryN < (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t));
					++fatSectorEntryN, ++fatAbsoluteEntryN)
			{
				// Первые две записи в цепочке FAT32 являются особыми значениями
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

					// Основной выход из этого уровня цикла
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
	/*ФАЗА №1 (КОНЕЦ): Анализ префиксов и первой цепочки кластеров в таблицах FAT32*/

	// Если префиксы и первая цепочка (цепочка корневой директории)
	// соответствуют нашему формату, то следует проанализировать формат цепочек,
	// предназначенных для файлов MY_FILE
	// (непрерывная последовательность файлов по 10 Мб,
	// цепочки которых выравнены по 512 секторам таблиц FAT32)

	/*ФАЗА №2 (НАЧАЛО): Анализ цепочек кластеров кольца MY_FILE в таблицах FAT32*/
	uint32_t clusterPerFile = RINGSTORE_CONFIG_SECTORS_PER_FILE / drive->sectorsPerCluster;
	uint32_t currentFileClusterEntry, myFilesCount;
	uint32_t fatSectorAfterRootDir = fatSectorN + 1;

	// Для каждой копии таблицы FAT32
	for (fatN = 0; fatN < drive->fatCopyCount; ++fatN)
	{
		currentFileClusterEntry = 0, myFilesCount = 0;
		fatAbsoluteEntryN = fatSectorAfterRootDir * (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t));

		// Для каждого сектора таблиц FAT32,
		// начиная со следующего за тем, на котором мы остановились ранее,
		// мы, согласно нашему формату, ожидаем обнаружить разметку оставшейся области на 10 Мб файлы.
		for (fatSectorN = fatSectorAfterRootDir; /*выход через break*/; ++fatSectorN)
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

						// Эта схема предполагает игнорирование оставшейся за EOC части сектора разметки,
						// которой, якобы, не должно оставаться при разметке таблиц на 512-выровненные цепочки.
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
	/*ФАЗА №2 (КОНЕЦ): Анализ цепочек кластеров кольца MY_FILE в таблицах FAT32*/

	/*ФАЗА №3 (НАЧАЛО): Анализ списка записей корневой директории*/
	// Согласно нашему воображаемому формату, список файлов корневой директории
	// должен быть непрерывным (каждый новый файл должен ссылаться на очередную
	// предопределённую цепочку кластеров).
	//
	// В целях экономии вычислительных ресурcов и уменьшения времени инициализации,
	// будем считать это единственным необходимым критериям для подтверждения того,
	// что структура корневой директории соответствует нашему формату,
	// и может быть принята как есть без форматирования.
	FILEIO_DIRECTORY_ENTRY *dirEntrys = (FILEIO_DIRECTORY_ENTRY*) fat;

	uint32_t chainRootDirSectorsCount =
			rStore->CALC_totalRootDirClustersCount * drive->sectorsPerCluster;

	uint32_t dataSectorN;
	uint8_t dataEntryN;

	uint32_t curClusterN, curMyFilesCount = 0;
	uint32_t fNameNum, fSize;

	rStore->CUR_FileNameIndex = 0x00000000;
	bool nullEntryFlag = false;
	// MY: написана ночью, проверялась слабо
	for (dataSectorN = 0; dataSectorN < chainRootDirSectorsCount; ++dataSectorN)
	{
		// Считываем нужный сектор корневого каталога...
		if (!FILEIO_SD_SectorRead(drive->mediaParameters,
				drive->firstDataSector + dataSectorN, (uint8_t*) dirEntrys))
			return FILEIO_ERROR_BAD_SECTOR_READ;

		uint32_t firstClusterOfFile;
		for (dataEntryN = 0;
				dataEntryN < (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / FILEIO_DIRECTORY_ENTRY_SIZE);
				++dataEntryN, ++curMyFilesCount, curClusterN += clusterPerFile)
		{

			if (dataSectorN == 0 && dataEntryN == 0)
				continue; // Пропускаем запись с именем логического раздела

			if (dataSectorN == 0 && dataEntryN == 1)
			{
				curClusterN = rStore->CALC_FirstFileClusterNum;
			} else if (dirEntrys[dataEntryN].name[0] == 0)
			{
				// Обнаружение пустой записи
				nullEntryFlag = true;
				continue;
			} else if (nullEntryFlag == true)
			{
				// Если поле с содержанием встречается после пустого...				
				return FILEIO_ERROR_NOT_FORMATTED;
			}

			firstClusterOfFile = (uint32_t) dirEntrys[dataEntryN].firstClusterHigh;
			firstClusterOfFile <<= sizeof (uint16_t) * RINGSTORE_BIT_PER_BYTE;
			firstClusterOfFile += (uint32_t) dirEntrys[dataEntryN].firstClusterLow;

			if (curMyFilesCount > rStore->CALC_maxFilesCount)
				if (firstClusterOfFile != 0) // Если есть записи файлов за рассчётными пределами
					return FILEIO_ERROR_NOT_FORMATTED;

			if (firstClusterOfFile != curClusterN) // Если первый кластер файла не равен расчётному
				return FILEIO_ERROR_NOT_FORMATTED;

			fNameNum = atoi(dirEntrys[dataEntryN].name);
			if (fNameNum >= rStore->CUR_FileNameIndex) // Обновляем курсор, если это необходимо
			{
				rStore->CUR_FileNameIndex = fNameNum;
				rStore->CUR_FileRootDirIndex = curMyFilesCount - 1;
				fSize = dirEntrys[dataEntryN].fileSize;
			}
		}
	}

	// Вычисляем оставшиеся компоненты курсора
	rStore->CUR_FilePhysicalSectorNum =
			rStore->PH_ADDR_SectorFirstFile
			+ 1l * rStore->CUR_FileRootDirIndex * RINGSTORE_CONFIG_SECTORS_PER_FILE
			+ fSize / FILEIO_CONFIG_MEDIA_SECTOR_SIZE;

	rStore->CUR_SectorByteNum = fSize % FILEIO_CONFIG_MEDIA_SECTOR_SIZE;

	if (rStore->CUR_SectorByteNum != 0)
		if (!FILEIO_SD_SectorRead(drive->mediaParameters,
				rStore->CUR_FilePhysicalSectorNum, rStore->CUR_LoggingBuffer))
			return FILEIO_ERROR_BAD_SECTOR_READ;

	/*ФАЗА №3 (КОНЕЦ): Анализ списка записей корневой директории*/

	return FILEIO_ERROR_NONE;
}
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: отформатировать файловую систему FAT32 в соответствии с
//FAT32-совместимым форматом myFAT (пользовательским),
//подразумевающим использование доступного пространства памяти
//исключительно для хранения максимального числа файлов MY_FILE
//(файлы c коротким именем, занимающие по 10 Мб, цепочки кластеров
//которых непрерывны и выровненны по секторам таблиц FAT).
//Устанавливает курсор записи на исходную позицию.
//Аргументы: LOGICAL_MY_FAT32_DRIVE_INFO *lDriveInfo -
//информация о логическом разделе
//return: MY_FAT32_ERROR (код завершения проверки на
//соответствие/несоответствие формату myFAT)
// ******************************************************************

static FILEIO_ERROR_TYPE RINGSTORE_FormattingTables(RINGSTORE_OBJECT *rStore)
{
	// Посекторно проверяем первую часть таблицы FAT,
	// хранящую цепочку корневой директории,
	// на предмет соответствия нашему формату
	uint32_t fatSectorN, fatSectorEntryN, fatAbsoluteEntryN;
	uint32_t fat[FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t)]; // Буфер для чтения 512 сектора

	uint8_t fatN;

	/*ФАЗА №1 (НАЧАЛО): Форматирование префиксов и первой цепочки кластеров в таблицах FAT32*/

	// Минимальная длина цепочки long стартовых байтов таблицы FAT и корневого каталога,
	// необходимая для размещения максимально возможного списка MY_FILE
	// (в данном случае 10 Мб файлов).
	uint16_t firstChainOfClustersLength =
			2 + rStore->CALC_totalRootDirClustersCount;

	for (fatSectorN = 0; /*выход через break*/; ++fatSectorN)
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

			// Основной выход из цикла инициализации первой цепочки

			// Если это последний кластер цепочки, то отметить его EOC,
			// а оставшуюся часть обнулить.
			if (fatAbsoluteEntryN == (firstChainOfClustersLength - 1))
			{
				// Вариант firstChainOfClusters == 2 (нулевая цепочка RootDir)
				// не должен быть допущен!
				// Сейчас он исключён установщиком FS_getLogicalInfoMyFAT32().
				fat[fatSectorEntryN] = FAT32_MARK_EOC;

				// То нужно обнулить оставшуюся часть сектора,
				// и отправить его на запись, выпрыгнув из цикла.
				memset(
						fat + (fatSectorEntryN + 1),
						0x00,
						FILEIO_CONFIG_MEDIA_SECTOR_SIZE - 4 * (fatSectorEntryN + 1));

				break;
			}

			fat[fatSectorEntryN] = fatAbsoluteEntryN + 1;
		}

		// Запись сектора во все копии таблицы FAT
		for (fatN = 0; fatN < drive->fatSectorCount; ++fatN)
			if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
					drive->firstFatSector + fatN * drive->fatSectorCount + fatSectorN, (uint8_t*) fat, false))
				return FILEIO_ERROR_WRITE;

		// Основной выход из цикла инициализации первой цепочки

		// В таблице FAT достаточно перейти на следующий сектор,		
		if (fatAbsoluteEntryN == (firstChainOfClustersLength - 1))
			break;
	}
	// По выходу из предшествующего цикла,
	// fatSectorN и fatAbsoluteEntryN должны указывать на начало нового сектора таблицы FAT,
	// с которого должны начаться цепочки кластеров колец MY_FILE файлов.

	/*ФАЗА №1 (КОНЕЦ): Форматирование префиксов и первой цепочки кластеров в таблицах FAT32*/

	/*ФАЗА №2 (НАЧАЛО): Форматирование цепочек кластеров кольца MY_FILE в таблицах FAT32*/
	uint32_t clusterPerFile = RINGSTORE_CONFIG_SECTORS_PER_FILE / drive->sectorsPerCluster;
	uint32_t currentFileClusterEntry = 0, myFilesCount = 0;


	// Курсор fatAbsoluteEntryN  также должен быть передвинут
	// на начало следующего сектора
	fatAbsoluteEntryN = fatAbsoluteEntryN % 128 ?
			MATH_DivisionCeiling(fatAbsoluteEntryN, 128) * 128 : fatAbsoluteEntryN;


	for (++fatSectorN; /*выход через break*/; ++fatSectorN)
	{
		for (
				fatSectorEntryN = 0;
				fatSectorEntryN < (FILEIO_CONFIG_MEDIA_SECTOR_SIZE / sizeof (uint32_t));
				++fatSectorEntryN, ++fatAbsoluteEntryN)
		{
			++currentFileClusterEntry;
			if (currentFileClusterEntry == clusterPerFile) // Если перешли на кластер следующего файла
			{
				fat[fatSectorEntryN] = FAT32_MARK_EOC;
				currentFileClusterEntry = 0;
				++myFilesCount;
			} else if (myFilesCount != rStore->CALC_maxFilesCount) // Если дописываем цепочку текущего файла
			{
				fat[fatSectorEntryN] = fatAbsoluteEntryN + 1;
			} else // Если цепочки максимального числа наших файлов (10Mb) готовы
			{
				memset(
						fat + fatSectorEntryN,
						0x00,
						FILEIO_CONFIG_MEDIA_SECTOR_SIZE - 4 * fatSectorEntryN);
				break;
			}
		}

		// Запись сектора во все копии таблицы FAT
		for (fatN = 0; fatN < drive->fatSectorCount; ++fatN)
			if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
					drive->firstFatSector + fatN * drive->fatSectorCount + fatSectorN, (uint8_t*) fat, false))
				return FILEIO_ERROR_WRITE;

		// Основной выход...
		if (myFilesCount == rStore->CALC_maxFilesCount)
			break;
	}
	/*ФАЗА №2 (КОНЕЦ): Форматирование цепочек кластеров кольца MY_FILE в таблицах FAT32*/

	/*ФАЗА №3 (НАЧАЛО): Очистка области памяти, выделенной для корневой директории*/

	// Создание записи первого файла
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

	// Очистка оставшейся части корневой директории	
	uint8_t* nullBuf = (uint8_t*) fat;
	memset(nullBuf, 0, 512);
	uint32_t minRootDirSectorsCount = rStore->CALC_totalRootDirClustersCount * drive->sectorsPerCluster;

	uint32_t dataSectorN;
	for (dataSectorN = 1; dataSectorN < minRootDirSectorsCount; ++dataSectorN)
		if (!FILEIO_SD_SectorWrite(drive->mediaParameters,
				drive->firstDataSector + dataSectorN, nullBuf, false))
			return FILEIO_ERROR_WRITE;
	/*ФАЗА №3 (КОНЕЦ): Очистка области памяти, выделенной для корневой директории*/

	return FILEIO_ERROR_NONE;
}
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: передать очередной пакет для записи на SD-накопитель
//(отправляется на запись через промежуточный буфер, который отправляется
//на запись только тогда, когда в него больше не помещается новый пакет).
//Аргументы: const uint8_t packet[] - указатель на пакет для передачи
//uint8_t pSize - размер пакета для передачи
//return: bool (флаг успеха; возвращает true при штатном завершении)
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
//Функция: 
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