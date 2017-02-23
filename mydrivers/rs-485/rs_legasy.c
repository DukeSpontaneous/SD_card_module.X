#include "rs_legasy.h"

extern MODULE_STATE moduleState;
extern RINGSTORE_OBJECT rStore;

extern struct
{
	FILEIO_DIRECTORY currentWorkingDirectory;
} globalParameters;

// Возвращает число байтов в кластере, делённое на 256... o_o

uint32_t FSectorSize(void)
{
	uint32_t fSize = 0;

	FILEIO_DRIVE *drive = globalParameters.currentWorkingDirectory.drive;
	if (drive != NULL)
	{
		fSize = (drive->sectorsPerCluster * drive->sectorSize) >> 8;
	}

	return fSize;
}

// Возвращает число кластеров в файловой системе

uint32_t FTotalClusters(void)
{
	uint32_t tClusters = 0;

	FILEIO_DRIVE *drive = globalParameters.currentWorkingDirectory.drive;
	if (drive != NULL)
	{
		tClusters = drive->partitionClusterCount;
	}

	return tClusters;
}

// Возвращает число свободных кластеров в файловой системе

uint32_t FgetSetFreeCluster(void)
{
	uint32_t fClusters = 0;

	FILEIO_DRIVE *drive = globalParameters.currentWorkingDirectory.drive;
	if (drive != NULL)
	{
		// TODO: это заглушка, так как на самом деле в SD-модуль не ведёт учёт
		// свободным кластерам как таковым
		// (понятие "свободного пространства" не совсем корректно по отношению
		// к кольцевому хранилищу, фактически оно "занимает" всё доступное
		// пространство при инициализации карты).
		fClusters = drive->partitionClusterCount - 1;
	}

	return fClusters;
}

void getSDFlags(struct_flags_flash *flags_flash)
{
	flags_flash->char_flags = 0;

	flags_flash->on_off = PORTBbits.RB4;
	flags_flash->wp_on_off = PORTBbits.RB5;
	flags_flash->micro_in_off = PORTBbits.RB15;
	flags_flash->vdd_on_off = PORTAbits.RA4;

	flags_flash->flash_initialized =
			(moduleState == MODULE_STATE_CARD_INITIALIZED) ? 1 : 0;

	// Значение, возвращённое функцией инициализации SD-карты.
	// Никаким иным образом ей ничего не присваивается o_o...
	// Получает 0 при успехе инициализации, 1 - при ошибке.
	// При инициализации обнуляется (вместе со всеми другими флагами).
	flags_flash->flash_init_answer =
			(moduleState == MODULE_STATE_FAILED) ? 1 : 0;

	flags_flash->delay_before_init = 1;

	// Флаг управления критической секцией главного цикла.
	// Значение 1 свидетельствует о том, что идёт запись на SD-карту.
	flags_flash->start_write =
			rStore.FLAG_BufferUsed ? 1 : 0;
}