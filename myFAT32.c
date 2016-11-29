#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "myFAT32.h"

#include "SD_protocol.h"
#include "MathHelper.h"
#include "TimeHelper.h"

LOGICAL_MY_FAT32_DRIVE_INFO LogacalDriveInfo;

// TODO: заменить на какой-то нормальный суррогат реального времени	
static struct tm currentTime;


// ******************************************************************
//Функция: проверить BIOS Parameter Block запоминающего устройства
//(SD карты) на совместимость с myFAT32
//(под этим подразумевается наиболее распространённое состояние
//FAT32, при котором в файловая система содержит две FAT,
//содержит корневую директорию во втором кластере и т.д.).
//Аргументы: BIOS_PARAMETER_BLOCK_STRUCTURE *bpb (указатель на приёмник)
//return: bool (флаг успеха; возвращает true при штатном завершении)
// ******************************************************************

static bool FS_validationMyFAT32(const BIOS_PARAMETER_BLOCK_STRUCTURE *bpb)
{
	// Если целевой сектор не является загрузочным,
	if (bpb->BS_jmpBoot[0] != 0xE9 && bpb->BS_jmpBoot[0] != 0xEB)
		return false; // то это аварийная ситуация.

	// если последних два его байта не соответствуют сигнатуре 55AAh,
	// выдается сообщение об ошибке: Missing operating system и система останавливается)
	if (bpb->bootEndSignature != 0xAA55)
		return false;

	if (bpb->BPB_BytsPerSec != BYTE_PER_SECTOR)
		return false;

	// Если число секторов в кластере не равно степени двойки,
	// или выходит из установленного диапазона [1, 64]
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
//Функция: получить BIOS Parameter Block запоминающего устройства (SD карты)
//Аргументы: BIOS_PARAMETER_BLOCK_STRUCTURE *bpb (указатель на приёмник)
//return: bool (флаг успеха; возвращает true при штатном завершении)
// ******************************************************************

static bool FS_getBIOSParameterBlock(BIOS_PARAMETER_BLOCK_STRUCTURE *bpb)
{
	if (!SD_readSingle512Block((uint8_t*) bpb, 0))
		return false;

	// если последних два его байта не соответствуют сигнатуре 55AAh,
	// выдается сообщение об ошибке: Missing operating system и система останавливается)
	if (bpb->bootEndSignature != 0xAA55)
		return false;

	// Если первый байт этого сектора SD карты равен 0xEB или 0xE9,
	// то это загрузочный сектор.
	// В противном случае это MBR, и следует перейти к логической адресации секторов.
	if (bpb->BS_jmpBoot[0] != 0xE9 && bpb->BS_jmpBoot[0] != 0xEB)
	{
		PARTITION_INFO_16_BYTE_LINE *partition = (PARTITION_INFO_16_BYTE_LINE*) ((MBR_INFO_STRUCTURE*) bpb)->partitionData; //first partition

		// Это значение в русском описании интерпретируется как "Число секторов предшествующих разделу"
		long firstLogicalSector = partition->firstSector; //the unused sectors, hidden to the FAT

		if (!SD_readSingle512Block((uint8_t*) bpb, firstLogicalSector))
			return false;
	}

	if (!FS_validationMyFAT32(bpb))
		return false; // то это аварийная ситуация.		

	return true;
}
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: вычислить максимальное число файлов MY_FILE (FAT32),
//которые можно разместить в одной директории на запоминающем устройстве.
//Аргументы: unsigned long totalSectorsCount - число доступных секторов
//unsigned char sectorPerCluster - размер кластера в секторах
//return: unsigned long (максимальное число файлов,
//которое можно разместить на ЗУ; 0 при недопустимых аргументах)
// ******************************************************************

static uint32_t FS_calcMax10MBFilesCount(const uint32_t totalDataSectorsCount, const uint8_t sectorPerCluster)
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
	uint32_t clusterPerFile = FILE_10MB_IN_512_SECTORS_SIZE / sectorPerCluster;

	uint32_t maxFilesCount = totalClustersCount / clusterPerFile;

	// Узнаём, сколько байтов нужно для размещения требуемой корневой директории
	uint32_t rootDirBytesCount = maxFilesCount * FAT32_SHORT_FILE_ENTRY_BYTES;
	// Узнаём, сколько секторов нужно для размещения требуемой корневой директории
	uint32_t rootDirSectorsCount = MATH_DivisionCeiling(rootDirBytesCount, BYTE_PER_SECTOR);
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

static uint16_t FS_calcMinMyFilesRootDirClusterChainLength(const uint32_t maxFilesCount, const uint8_t sectorPerCluster)
{
	// Защита от деления на 0
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
	 * Bits 0–4: Day of month, valid value range 1-31 inclusive.
	 * Bits 5–8: Month of year, 1 = January, valid value range 1–12 inclusive.
	 * Bits 9–15: Count of years from 1980, valid value range 0–127 inclusive (1980–2107).
	 */

	uint16_t crtDate = 0;

	crtDate += time->tm_mday; // int tm_mday; День месяца - [1,31]
	crtDate += (time->tm_mon + 1) << 5; // int tm_mon; Месяцы после января - [0,11]
	crtDate += (time->tm_year - 80) << 9; // int tm_year; Года с 1900

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

	entry->DIR_CrtTimeTenth = (time->tm_sec % 2) * 100; // int tm_sec; Секунды от начала минуты - [0,60]

	/*
	 * Time Format. A FAT directory entry time stamp is a 16-bit field that has a granularity of 2 seconds. Here is the format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the 16-bit word).
	 * 
	 * Bits 0–4: 2-second count, valid value range 0–29 inclusive (0 – 58 seconds).
	 * Bits 5–10: Minutes, valid value range 0–59 inclusive.
	 * Bits 11–15: Hours, valid value range 0–23 inclusive.
	 * 
	 * The valid time range is from Midnight 00:00:00 to 23:59:58. 
	 */

	uint16_t crtTime = 0;

	crtTime += time->tm_sec > 58 ? (58 / 2) : (time->tm_sec / 2); // int tm_sec; Секунды от начала минуты - [0,60]
	crtTime += time->tm_min << 5; // int tm_min;	Минуты от начала часа - [0,59]
	crtTime += time->tm_hour << 11; // int tm_hour;	Часы от полуночи - [0,23]

	entry->DIR_CrtTime = crtTime;
	entry->DIR_WrtTime = crtTime;
}
//------------------------------------------------------------------------------

static void FS_setFileEntryWrtTime(DIRECTORY_ENTRY_STRUCTURE *entry, const struct tm *time)
{
	/*
	 * Date Format. A FAT directory entry date stamp is a 16-bit field that is basically a date relative to the MS-DOS epoch of 01/01/1980. Here is the format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the 16-bit word):
	 * 
	 * Bits 0–4: Day of month, valid value range 1-31 inclusive.
	 * Bits 5–8: Month of year, 1 = January, valid value range 1–12 inclusive.
	 * Bits 9–15: Count of years from 1980, valid value range 0–127 inclusive (1980–2107).
	 */

	uint16_t wrtDate = 0;

	wrtDate += time->tm_mday; // int tm_mday; День месяца - [1,31]
	wrtDate += (time->tm_mon + 1) << 5; // int tm_mon; Месяцы после января - [0,11]
	wrtDate += (time->tm_year - 80) << 9; // int tm_year; Года с 1900

	entry->DIR_WrtDate = wrtDate;
	entry->DIR_LstAccDate = wrtDate;

	/*
	 * Time Format. A FAT directory entry time stamp is a 16-bit field that has a granularity of 2 seconds. Here is the format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the 16-bit word).
	 * 
	 * Bits 0–4: 2-second count, valid value range 0–29 inclusive (0 – 58 seconds).
	 * Bits 5–10: Minutes, valid value range 0–59 inclusive.
	 * Bits 11–15: Hours, valid value range 0–23 inclusive.
	 * 
	 * The valid time range is from Midnight 00:00:00 to 23:59:58. 
	 */

	uint16_t wrtTime = 0;

	wrtTime += time->tm_sec > 58 ? (58 / 2) : (time->tm_sec / 2); // int tm_sec; Секунды от начала минуты - [0,60]
	wrtTime += time->tm_min << 5; // int tm_min;	Минуты от начала часа - [0,59]
	wrtTime += time->tm_hour << 11; // int tm_hour;	Часы от полуночи - [0,23]

	entry->DIR_WrtTime = wrtTime;
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
		// Номер очередного файла превышает вычисленный максимум файлов:

		// Нам следует перейти в начало кольца файлов
		// (в начало кольца записей корневой директории)		
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
		// Мы не можем добавить сделующую запись в тот же сектор RootDir:

		// Значит очередная запись находится НЕ В ТОМ ЖЕ секторе директории,
		// что и предыдущая (невозможно обновить обе записи одним действием)
		if (!SD_writeSingle512Block((uint8_t*) dirEntries, indexOfOldSectorInRootDir))
			return false;

		// INFO: пустые записи в директории могут быть интерпретированы
		// файловым менеджером как конец списка файлов.

		indexOfNewSectorInRootDir = indexOfOldSectorInRootDir + 1;

		if (!SD_readSingle512Block((uint8_t*) dirEntries, indexOfNewSectorInRootDir))
			return false;
		FS_createFileEntryUpdate(&dirEntries[0], myInfo, &currentTime);

	} else
	{
		// Мы можем добавить сделующую запись в тот же сектор RootDir:

		// Значит очередная запись находится В ТОМ ЖЕ секторе директории,
		// что и предыдущая (возможно обновить обе записи одним действием)
		FS_createFileEntryUpdate(&dirEntries[indexOfOldEntryInSector + 1], myInfo, &currentTime);

		indexOfNewSectorInRootDir = indexOfOldSectorInRootDir;
	}
	if (!SD_writeSingle512Block((uint8_t*) dirEntries, indexOfNewSectorInRootDir))
		return false;

	return true;
}
//------------------------------------------------------------------------------

// Записать буфер логирования на ПЗУ и обновить положение курсора

static bool FS_pushLoggingBuffer(LOGICAL_MY_FAT32_DRIVE_INFO *myInfo)
{
	uint16_t freeBytesIndex = myInfo->CUR_SectorByteNum;

	// Обнулить свободную часть буфера
	memset(
			myInfo->CUR_LoggingBuffer + freeBytesIndex,
			0,
			BYTE_PER_SECTOR - freeBytesIndex);

	if (!SD_writeSingle512Block(myInfo->CUR_LoggingBuffer, myInfo->CUR_FilePhysicalSectorNum))
		return false;

	/* Перевод курсора на следующий сектор логического кольца ПЗУ */
	if (
			((myInfo->CUR_FilePhysicalSectorNum + 1
			- myInfo->PH_ADDR_SectorFirstFile)

			% myInfo->sectorPer10MbFile) == 0)
	{
		// Если номер очередного сектора
		// относительно номера сектора первого файла
		// делится на длину файла в секторах без остатка,
		// то это значит, что мы вошли в нулевой сектор следующего файла.

		FS_stepToNextRootDirEntry(myInfo);
	} else
	{
		// Если следующий сектор принадлежит текущему файлу
		++(myInfo->CUR_FilePhysicalSectorNum);
	}

	/* Перевод курсора на исходный байт нового сектора */
	myInfo->CUR_SectorByteNum = 0;

	return true;
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
//Функция: принудительно сохранить 512-буфер сектора по физическому
//адресу, на который указывает курсор (с обнулением свободной части),
//и сдвинуть курсор в LOGICAL_MY_FAT32_DRIVE_INFO.
//return: bool (флаг успеха; возвращает true при штатном завершении)
// ******************************************************************

bool FS_saveBuffer()
{
	return FS_pushLoggingBuffer(&LogacalDriveInfo);
}
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: рассчитать/установить параметры подключенного SD-накопителя,
//на котором ожидается обнаружить FAT32-совместимую файловую систему.
//Аргументы: LOGICAL_MY_FAT32_DRIVE_INFO *myInfo - приёмник параметров 
//return: bool (флаг успеха; возвращает true при штатном завершении)
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

	// Пространство данных начинается ровно за второй таблицей FAT (если их две).
	// По умолчанию (и согласно нашему формату) именно тут должна начинаться
	// корневая директория логического раздела.
	myInfo->PH_ADDR_SectorSecondCluster =
			myInfo->PH_ADDR_SectorFAT1 + myInfo->sectorPerFAT * myInfo->FAT_CountOfTables;

	myInfo->TOTAL_LogicalSectors = bpb.BPB_TotSec32;

	myInfo->sectorPerCluster = bpb.BPB_SecPerClus;
	myInfo->sectorPer10MbFile = FILE_10MB_IN_512_SECTORS_SIZE;

	// Пространство секторов памяти, доступное для размещения папок и файлов
	// (с поправкой на вычет пространства памяти перед вторым кластером)
	uint32_t logicalSectorsDataCount =
			myInfo->TOTAL_LogicalSectors -
			(myInfo->PH_ADDR_SectorSecondCluster - bpb.BPB_HiddSec);

	// Максимальное число файлов FAT32 размера CURRENT_FILE_512_SECTORS_SIZE,
	// которое можно разместить на целевом записывающем устройстве.
	myInfo->CALC_max10MBFilesCount = FS_calcMax10MBFilesCount(
			logicalSectorsDataCount, bpb.BPB_SecPerClus);
	myInfo->CALC_totalRootDirClustersCount = FS_calcMinMyFilesRootDirClusterChainLength(
			myInfo->CALC_max10MBFilesCount, bpb.BPB_SecPerClus);

	if (myInfo->CALC_max10MBFilesCount == 0 || myInfo->CALC_totalRootDirClustersCount == 0)
		return false;

	// Сколько записей нужно первой цепочке (две зерезервированы и корневая директория)
	uint32_t fatEntrysForFirstChain = (2 + myInfo->CALC_totalRootDirClustersCount);
	// Сколько секторов таблицы FAT32 нужно первой цепочке
	uint32_t fatSectorsForFirstChain = MATH_DivisionCeiling(fatEntrysForFirstChain, 128);

	// Номер первого выравненного по секторам FAT32 кластера (128-кратный),
	// который можно отдать первому файлу
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

MY_FAT32_ERROR FS_checkMyFAT32(LOGICAL_MY_FAT32_DRIVE_INFO *fsInfo)
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
	uint32_t fat[(BYTE_PER_SECTOR / sizeof (uint32_t))]; // Буфер для чтения 512 сектора

	uint8_t fatN;

	/*ФАЗА №1 (НАЧАЛО): Анализ префиксов и первой цепочки кластеров в таблицах FAT32*/

	// Минимальная длина цепочки long стартовых байтов таблицы FAT и корневого каталога,
	// необходимая для размещения максимально возможного списка MY_FILE
	// (в данном случае 10 Мб файлов).
	uint16_t firstChainOfClustersLength =
			2 + fsInfo->CALC_totalRootDirClustersCount;

	// Для каждой копии таблицы FAT32
	for (fatN = 0; fatN < fsInfo->FAT_CountOfTables; ++fatN)
	{
		// Для каждого сектора таблиц FAT32
		for (fatSectorN = 0; /*выход через break*/; ++fatSectorN)
		{
			if (!SD_readSingle512Block(
					(uint8_t*) fat,
					fsInfo->PH_ADDR_SectorFAT1 + fatN * fsInfo->sectorPerFAT + fatSectorN))
				return MY_FAT32_ERROR_IO;

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
					fatSectorEntryN < (BYTE_PER_SECTOR / sizeof (uint32_t));
					++fatSectorEntryN, ++fatAbsoluteEntryN)
			{
				// Первые две записи в цепочке FAT32 являются особыми значениями
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

					// Основной выход из этого уровня цикла
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
	/*ФАЗА №1 (КОНЕЦ): Анализ префиксов и первой цепочки кластеров в таблицах FAT32*/

	// Если префиксы и первая цепочка (цепочка корневой директории)
	// соответствуют нашему формату, то следует проанализировать формат цепочек,
	// предназначенных для файлов MY_FILE
	// (непрерывная последовательность файлов по 10 Мб,
	// цепочки которых выравнены по 512 секторам таблиц FAT32)

	/*ФАЗА №2 (НАЧАЛО): Анализ цепочек кластеров кольца MY_FILE в таблицах FAT32*/
	uint32_t clusterPerFile = FILE_10MB_IN_512_SECTORS_SIZE / fsInfo->sectorPerCluster;
	uint32_t currentFileClusterEntry, myFilesCount;
	uint32_t fatSectorAfterRootDir = fatSectorN + 1;

	// Для каждой копии таблицы FAT32
	for (fatN = 0; fatN < fsInfo->FAT_CountOfTables; ++fatN)
	{
		currentFileClusterEntry = 0, myFilesCount = 0;
		fatAbsoluteEntryN = fatSectorAfterRootDir * (BYTE_PER_SECTOR / sizeof (uint32_t));

		// Для каждого сектора таблиц FAT32,
		// начиная со следующего за тем, на котором мы остановились ранее,
		// мы, согласно нашему формату, ожидаем обнаружить разметку оставшейся области на 10 Мб файлы.
		for (fatSectorN = fatSectorAfterRootDir; /*выход через break*/; ++fatSectorN)
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

						// Эта схема предполагает игнорирование оставшейся за EOC части сектора разметки,
						// которой, якобы, не должно оставаться при разметке таблиц на 512-выровненные цепочки.
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
	DIRECTORY_ENTRY_STRUCTURE *dirEntrys = (DIRECTORY_ENTRY_STRUCTURE*) fat;

	uint32_t chainRootDirSectorsCount =
			fsInfo->CALC_totalRootDirClustersCount * fsInfo->sectorPerCluster;

	uint32_t dataSectorN;
	uint8_t dataEntryN;

	uint32_t curClusterN, curMyFilesCount = 0;
	uint32_t fNameNum, fSize;

	// MY: написана ночью, проверялась слабо
	for (dataSectorN = 0; dataSectorN < chainRootDirSectorsCount; ++dataSectorN)
	{
		// Считываем нужный сектор корневого каталога...
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
				continue; // Пропускаем запись с именем логического раздела

			// Если поле пусто (любое поле, кроме первого) то пропускаем его...
			// TODO: но вообще желательно его полностью проверить, наверное
			if (dataSectorN == 0 && dataEntryN == 1)
				curClusterN = fsInfo->CALC_FirstFileClusterNum;
			else if (dirEntrys[dataEntryN].DIR_CrtDate == 0)
				continue;

			firstClusterOfFile = (uint32_t) dirEntrys[dataEntryN].DIR_FstClusHI;
			firstClusterOfFile <<= sizeof (uint16_t) * BIT_PER_BYTE;
			firstClusterOfFile += (uint32_t) dirEntrys[dataEntryN].DIR_FstClusLO;

			if (curMyFilesCount > fsInfo->CALC_max10MBFilesCount)
				if (firstClusterOfFile != 0) // Если есть записи файлов за рассчётными пределами
					return MY_FAT32_ERROR_FORMAT;

			if (firstClusterOfFile != curClusterN) // Если первый кластер файла не равен расчётному
				return MY_FAT32_ERROR_FORMAT;

			fNameNum = atoi(dirEntrys[dataEntryN].DIR_Name);
			if (fNameNum >= fsInfo->CUR_FileNameIndex) // Обновляем курсор, если это необходимо
			{
				fsInfo->CUR_FileNameIndex = fNameNum;
				fsInfo->CUR_FileRootDirIndex = curMyFilesCount - 1;
				fSize = dirEntrys[dataEntryN].DIR_FileSize;
			}
		}
	}

	// Вычисляем оставшиеся компоненты курсора
	fsInfo->CUR_FilePhysicalSectorNum =
			fsInfo->PH_ADDR_SectorFirstFile
			+ fsInfo->CUR_FileRootDirIndex * fsInfo->sectorPer10MbFile
			+ fSize / BYTE_PER_SECTOR;

	fsInfo->CUR_SectorByteNum = fSize % BYTE_PER_SECTOR;

	if (fsInfo->CUR_SectorByteNum != 0)
		if (!SD_readSingle512Block(fsInfo->CUR_LoggingBuffer, fsInfo->CUR_FilePhysicalSectorNum))
			return MY_FAT32_ERROR_IO;

	/*ФАЗА №3 (КОНЕЦ): Анализ списка записей корневой директории*/

	return MY_FAT32_ERROR_OK;
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

MY_FAT32_ERROR FS_formattingFAT32ToMyFAT32(LOGICAL_MY_FAT32_DRIVE_INFO *myInfo)
{
	// Посекторно проверяем первую часть таблицы FAT,
	// хранящую цепочку корневой директории,
	// на предмет соответствия нашему формату
	uint32_t fatSectorN, fatSectorEntryN, fatAbsoluteEntryN;
	uint32_t fat[BYTE_PER_SECTOR / sizeof (uint32_t)]; // Буфер для чтения 512 сектора

	uint8_t fatN;

	/*ФАЗА №1 (НАЧАЛО): Форматирование префиксов и первой цепочки кластеров в таблицах FAT32*/

	// Минимальная длина цепочки long стартовых байтов таблицы FAT и корневого каталога,
	// необходимая для размещения максимально возможного списка MY_FILE
	// (в данном случае 10 Мб файлов).
	uint16_t firstChainOfClustersLength =
			2 + myInfo->CALC_totalRootDirClustersCount;

	for (fatSectorN = 0; /*выход через break*/; ++fatSectorN)
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
				memset((fat + (fatSectorEntryN + 1)), 0x00, 4 * (128 - fatSectorEntryN - 1));

				break;
			}

			fat[fatSectorEntryN] = fatAbsoluteEntryN + 1;
		}

		// Запись сектора во все копии таблицы FAT
		for (fatN = 0; fatN < myInfo->FAT_CountOfTables; ++fatN)
			if (!SD_writeSingle512Block(
					(uint8_t*) fat,
					myInfo->PH_ADDR_SectorFAT1 + fatN * myInfo->sectorPerFAT + fatSectorN))
				return MY_FAT32_ERROR_IO;

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
	uint32_t clusterPerFile = FILE_10MB_IN_512_SECTORS_SIZE / myInfo->sectorPerCluster;
	uint32_t currentFileClusterEntry = 0, myFilesCount = 0;


	// Курсор fatAbsoluteEntryN  также должен быть передвинут
	// на начало следующего сектора
	fatAbsoluteEntryN = fatAbsoluteEntryN % 128 ?
			MATH_DivisionCeiling(fatAbsoluteEntryN, 128) * 128 : fatAbsoluteEntryN;


	for (++fatSectorN; /*выход через break*/; ++fatSectorN)
	{
		for (fatSectorEntryN = 0; fatSectorEntryN < (512 / 4); ++fatSectorEntryN, ++fatAbsoluteEntryN)
		{
			++currentFileClusterEntry;
			if (currentFileClusterEntry == clusterPerFile) // Если перешли на кластер следующего файла
			{
				fat[fatSectorEntryN] = FAT32_MARK_EOC;
				currentFileClusterEntry = 0;
				++myFilesCount;

				if (fatSectorEntryN != 127 && fatSectorEntryN != 63)
					return MY_FAT32_ERROR_CALCULATION;
			} else if (myFilesCount != myInfo->CALC_max10MBFilesCount) // Если дописываем цепочку текущего файла
			{
				fat[fatSectorEntryN] = fatAbsoluteEntryN + 1;
			} else // Если цепочки максимального числа наших файлов (10Mb) готовы
			{
				memset((fat + (fatSectorEntryN)), 0x00, 4 * (128 - fatSectorEntryN));
				break;
			}
		}

		// Запись сектора во все копии таблицы FAT
		for (fatN = 0; fatN < myInfo->FAT_CountOfTables; ++fatN)
			if (!SD_writeSingle512Block(
					(uint8_t*) fat,
					myInfo->PH_ADDR_SectorFAT1 + fatN * myInfo->sectorPerFAT + fatSectorN))
				return MY_FAT32_ERROR_IO;

		// Основной выход...
		if (myFilesCount == myInfo->CALC_max10MBFilesCount)
			break;
	}
	/*ФАЗА №2 (КОНЕЦ): Форматирование цепочек кластеров кольца MY_FILE в таблицах FAT32*/

	/*ФАЗА №3 (НАЧАЛО): Очистка области памяти, выделенной для корневой директории*/

	// Создание записи первого файла
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

	// Очистка оставшейся части корневой директории	
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
	/*ФАЗА №3 (КОНЕЦ): Очистка области памяти, выделенной для корневой директории*/

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