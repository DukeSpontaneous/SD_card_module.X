#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define BIT_PER_BYTE 8
#define BYTE_PER_SECTOR 512 // Наиболее распространённый размер сектора данных
#define BYTE_PER_SHORT_DIR_ENTRY 32 // Длина кототкой записи Directory Entry
#define FILE_10MB_IN_512_SECTORS_SIZE (10 * 0x800) // Выбранный размер файла (10 Мбайт)

#define FAT32_MAX_512_SECTORS_IN_CLUSTER 64 // Стандарт максимального размера кластера (в секторах)
#define FAT32_SHORT_FILE_ENTRY_BYTES 32 // Размер короткой записи о файле в директории (в байтах)

typedef enum
{
	FAT32_MARK_BAD_CLUSTER = 0x0FFFFFF7, // Метка плохого кластера
	FAT32_MARK_BPB_MEDIA = 0x0FFFFFF8, //  Байт FAT32 FAT[0] (почти всегда равняется этому значению)
	FAT32_MARK_EOC = 0x0FFFFFFF, // Метка "End Of Clusterchain"
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

// Таблица разделения физического диска на логические устройства (Partition Tables)

#pragma pack(push,1)	

typedef struct
{
	// https://ru.wikipedia.org/wiki/Главная_загрузочная_запись#.D0.A1.D1.82.D1.80.D1.83.D0.BA.D1.82.D1.83.D1.80.D0.B0_.D0.BE.D0.BF.D0.B8.D1.81.D0.B0.D0.BD.D0.B8.D1.8F_.D1.80.D0.B0.D0.B7.D0.B4.D0.B5.D0.BB.D0.B0
	uint8_t status; // Флаг загрузки (0x80 - активный / 0x00 - обычный раздел) - 1 байт

	uint8_t headStart; // Начальный физический сектор раздела (головка) - 1 байт
	uint16_t cylSectStart; // Начальный физический сектор раздела (сектор и цилиндр) - 2 байта

	uint8_t type; // Код типа раздела - 1 байт

	uint8_t headEnd; // Конечный физический сектор раздела (головка) - 1 байт
	uint16_t cylSectEnd; // Конечный физический сектор раздела (сектор и цилиндр) - 2 байта

	uint32_t firstSector; // Число секторов предшествующих разделу - 4 байта
	uint32_t sectorsTotal; // Общее количество секторов в данном разделе - 4 байта
} PARTITION_INFO_16_BYTE_LINE;
#pragma pack(pop)
//------------------------------------------------------------------------------

// Структура Главной Загрузочной Записи MBR (Master Boot Record),
// располагающаяся в нулевом 512-секторе пространства памяти.

#pragma pack(push,1)

typedef struct
{
	// Программа IPL 1 (программа загрузчика занимает зону от адреса 00h до 1BEh)
	uint8_t nothing[446];

	// Таблица разделения физического диска на логические устройства (Partition Tables)
	// (4 строки по 16 байт = 64 байта) занимает зону с адреса 1BEh до 1FDh	
	uint8_t partitionData[64];

	// 2 последних байта в блоке данных сектора с адреса 1FE по 1FF - концевая сигнатура (Ending Signature)
	uint16_t signature; //0xAA55
	// 55АА - отмечает конец MBR. Проверяется программой начального загрузчика
} MBR_INFO_STRUCTURE;
#pragma pack(pop)
//------------------------------------------------------------------------------

// Структура загрузочного сектора; описана в спецификации FAT32:
// "Microsoft Extensible Firmware Initiative // FAT32 File System Specification"
// P.S. Используется логическая адресация (в рамках тома).

#pragma pack(push,1)

typedef struct
{
	/* MS Spec: Boot Sector and BPB Structure */

	uint8_t BS_jmpBoot[3]; // Команды JMP и NOP												// default: 0x009000EB	//	[0, 2]		*0x4D9058EB
	char BS_OEMName[8]; // Название и версия ОС											// "MSDOS5.0"			//	[3, 10]

	// Эта часть загрузочного сектора известна как BIOS Parameter Block (BPB) (блок параметров BIOS).
	// Она содержит физические характеристики диска, которые MS-DOS и Windows используют при поиске определенного участка.
	// Складывая или перемножая значения этих параметров, операционная система узнает,
	// где находится таблица FAT, корневой каталог, где начинается и кончается область данных.

	// Формат блока параметров BIOS: DOS 7.1 EBPB (FAT32):

	// DOS 2.0 BPB
	uint16_t BPB_BytsPerSec; // Количество байтов на сектор										// deafault: 512		//	[11, 12]	*0x0200		= 512
	uint8_t BPB_SecPerClus; // Количество секторов на кластер (всегда кратно двум в степени п)						//	[13]		*0x08
	uint16_t BPB_RsvdSecCnt; // Количество зарезервированных секторов перед первой FAT									//	[14, 15]	*0x186E		= 6254 (нумерация с нуля, последний резервированный это 6253: в логическом секторе 6254 начинается FAT1)
	// All Microsoft file system drivers will support a value other than 2,
	// but it is still highly recommended that no value other than 2 be used in this field
	uint8_t BPB_NumFATs; // Количество таблиц FAT																		//	[16]		*0x02
	uint16_t BPB_RootEntCnt; // Количество элементов в корневом каталоге (максимальный предел)	// must be 0 for FAT32	//	[17, 18]
	uint16_t BPB_TotSec16; // Общее число секторов FAT16										// must be 0 for FAT32	//	[19, 20]
	uint8_t BPB_Media; // Дескриптор среды					// в данном случае F8 (жёсткий диск с любой ёмкостью)		//	[21]
	uint16_t BPB_FATSz16; // Количество секторов на элемент таблицы FAT16						// must be 0 for FAT32	//	[22, 23]

	// DOS 3.31 ВРВ
	uint16_t BPB_SecPerTrk; // Количество секторов на дорожку													//	[24, 25]
	uint16_t BPB_NumHeads; // Число головок																		//	[26, 27]
	uint32_t BPB_HiddSec; // Количество скрытых секторов														//	[28, 31]	*0x000000E9	= 233
	uint32_t BPB_TotSec32; // Общее число секторов, если размер диска больше 32 Мб								//	[32, 35]	*0x000F4317	= 1000215

	/* MS Spec: FAT32 Structure Starting at Offset 36 */

	// DOS 7.1 EBPB
	uint32_t BPB_FATSz32; // Число секторов, занятых одной FAT														//	[36, 39]	*0x000003C9	= 969
	uint16_t BPB_ExtFlags; // Двойное поле флагов (два одинаковых байтовых поля)									//	[40, 41]
	uint16_t BPB_FSVer; // Версия // 0x0000 (defines version 0.0)													//	[42, 43]
	uint32_t BPB_RootClus; // Номер первого кластера, занимаемого корневым каталогом на FАТ32-диске	// (=2)`	//	[44, 47]	*0x00000002

	// BPB_FSInfo:
	// Указывает на второй загрузочный сектор. В нём содержится информация о том,
	// сколько на диске всего кластеров, сколько из них свободно и какой кластер был выделен самым последним.
	// Таким образом, чтобы получить эту часто используемую информацию, теперь не нужно считывать всю таблицу FAT.
	uint16_t BPB_FSInfo; // Указывает на сектор FSinfo structure										// (=1)		//	[48, 49]
	uint16_t BPB_BkBootSec; // Номер логического сектора, хранящий копию загрузочного					// 0x0006	//	[50, 51]
	uint8_t BPB_Reserved[12]; // Зарезервировано (имя файла загрузки)												//	[52, 63]
	uint8_t BS_DrvNum; // Номер физического диска							// 0x80 идентифицирует основной раздел	//	[64]
	uint8_t BS_Reserved1; // Флаги																				//	[65]
	uint8_t BS_BootSig; // Расширенная загрузочная запись						// всегда 0x29						//	[66]
	uint32_t BS_VolID; // Серийный номер тома																		//	[67, 70]
	char BS_VolLab[11]; // Метка тома										// "NO NAME    "					//	[71, 81]
	char BS_FilSysType[8]; // Тип файловой системы (12- или 16-разрядная)	// "FAT32   "						//	[82, 89]

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
	ATTR_ARCHIVE = 0x20, // Windows зачем-то отмечает так все простые файлы
	ATTR_LONG_NAME = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID

	/*
	 * The upper two bits of the attribute byte are reserved
	 * and should always be set to 0 when a file is created
	 * and never modified or looked at after that.
	 */
} DIR_ATTR;

// Structure to access Directory Entry in the FAT; описана в спецификации FAT32:
// "Microsoft Extensible Firmware Initiative // FAT32 File System Specification"

#pragma pack(push,1)

typedef struct
{
	char DIR_Name[11];

	uint8_t DIR_Attr; // DIR_ATTR file attributes

	// MY: на самом деле похоже бывает и отличен от нуля... 
	uint8_t DIR_NTRes; //Reserved for use by Windows NT. Set value to 0 when a file is created and never modify or look at it after that.

	uint8_t DIR_CrtTimeTenth; // count of tenths of a second and its valid value range is 0-199 inclusive.

	// Bits 0–4: 2-second count, valid value range 0–29 inclusive (0 – 58 seconds).
	// Bits 5–10: Minutes, valid value range 0–59 inclusive.
	// Bits 11–15: Hours, valid value range 0–23 inclusive.
	// The valid time range is from Midnight 00:00:00 to 23:59:58.
	uint16_t DIR_CrtTime; //Time file was created.

	// Bits 0–4: Day of month, valid value range 1-31 inclusive.
	// Bits 5–8: Month of year, 1 = January, valid value range 1–12 inclusive.
	// Bits 9–15: Count of years from 1980, valid value range 0–127 inclusive (1980–2107).
	uint16_t DIR_CrtDate; //Date file was created.

	uint16_t DIR_LstAccDate; // Last access date. Note that there is no last access time, only a date. This is the date of last read or write. In the case of a write, this should be set to the same date as DIR_WrtDate.
	uint16_t DIR_FstClusHI; //higher word of the first cluster number
	uint16_t DIR_WrtTime; //Time of last write. Note that file creation is considered a write.
	uint16_t DIR_WrtDate; //Date of last write. Note that file creation is considered a write.
	uint16_t DIR_FstClusLO; //lower word of the first cluster number
	uint32_t DIR_FileSize; //32-bit DWORD holding this file’s size in bytes.
} DIRECTORY_ENTRY_STRUCTURE;
#pragma pack(pop)
//------------------------------------------------------------------------------

typedef struct
{
	// При такой схеме можно практически полностью уйти от кластерной системы,
	// которая остаётся нужна лишь для реализации браковки неисправных кластеров.

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
	// Минимальная длина цепочки кластеров корневой директории (RootDir), необходимая
	// для адресации MyFile файлов, заполняющих всё доступное пространство памяти.
	uint16_t CALC_totalRootDirClustersCount;

	uint32_t CUR_FileNameIndex;	// Индексация с нуля?
	uint16_t CUR_FileRootDirIndex;		// Индексация с нуля?
	uint32_t CUR_FilePhysicalSectorNum;
	uint16_t CUR_SectorByteNum;
	
	uint8_t CUR_LoggingBuffer[BYTE_PER_SECTOR];
} LOGICAL_MY_FAT32_DRIVE_INFO;

extern LOGICAL_MY_FAT32_DRIVE_INFO LogacalDriveInfo;
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: передать очередной пакет для записи на SD-накопитель
//(отправляется на запись через промежуточный буфер, который отправляется
//на запись только тогда, когда в него больше не помещается новый пакет).
//Аргументы: const uint8_t packet[] - указатель на пакет для передачи
//uint8_t pSize - размер пакета для передачи
//return: bool (флаг успеха; возвращает true при штатном завершении)
// ******************************************************************
bool FS_loggingPacket(const uint8_t packet[], uint8_t pSize);
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: принудительно сохранить 512-буфер сектора по физическому
//адресу, на который указывает курсор (с обнулением свободной части),
//и сдвинуть курсор в LOGICAL_MY_FAT32_DRIVE_INFO.
//return: bool (флаг успеха; возвращает true при штатном завершении)
// ******************************************************************
bool FS_saveBuffer();
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: рассчитать/установить параметры подключенного SD-накопителя,
//на котором ожидается обнаружить FAT32-совместимую файловую систему.
//Аргументы: LOGICAL_MY_FAT32_DRIVE_INFO *myInfo - приёмник параметров 
//return: bool (флаг успеха; возвращает true при штатном завершении)
// ******************************************************************
bool FS_getMyFAT32Info(LOGICAL_MY_FAT32_DRIVE_INFO *lDriveInfo);
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
MY_FAT32_ERROR FS_checkMyFAT32(LOGICAL_MY_FAT32_DRIVE_INFO *lDriveInfo);
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
MY_FAT32_ERROR FS_formattingFAT32ToMyFAT32(LOGICAL_MY_FAT32_DRIVE_INFO *lDriveInfo);
//------------------------------------------------------------------------------