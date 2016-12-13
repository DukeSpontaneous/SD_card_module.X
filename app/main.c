#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <xc.h>

#include "main.h"
#include "system.h"
#include "rtcc.h"

#include "ringstore_fat32.h"

extern FILEIO_SD_DRIVE_CONFIG sdCardMediaParameters;

// The gSDDrive structure allows the user to specify which set of driver functions should be used by the
// FILEIO library to interface to the drive.
// This structure must be maintained as long as the user wishes to access the specified drive.
const FILEIO_DRIVE_CONFIG gSdDrive = {
	(FILEIO_DRIVER_IOInitialize) FILEIO_SD_IOInitialize, // Function to initialize the I/O pins used by the driver.
	(FILEIO_DRIVER_MediaDetect) FILEIO_SD_MediaDetect, // Function to detect that the media is inserted.
	(FILEIO_DRIVER_MediaInitialize) FILEIO_SD_MediaInitialize, // Function to initialize the media.
	(FILEIO_DRIVER_MediaDeinitialize) FILEIO_SD_MediaDeinitialize, // Function to de-initialize the media.
	(FILEIO_DRIVER_SectorRead) FILEIO_SD_SectorRead, // Function to read a sector from the media.
	(FILEIO_DRIVER_SectorWrite) FILEIO_SD_SectorWrite, // Function to write a sector to the media.
	(FILEIO_DRIVER_WriteProtectStateGet) FILEIO_SD_WriteProtectStateGet, // Function to determine if the media is write-protected.
};

int main(void)
{
	BSP_RTCC_DATETIME dateTime;
	RINGSTORE_OBJECT rStore;

	SYSTEM_Initialize();

	dateTime.bcdFormat = false;
	RTCC_BuildTimeGet(&dateTime);
	
	// TODO: не сконфигурирована для этого модуля
	//RTCC_Initialize(&dateTime);

	// Initialize the library
	if (!FILEIO_Initialize())
	{
		while (1);
	}

	// Register the GetTimestamp function as the timestamp source for the library.
	FILEIO_RegisterTimestampGet(GetTimestamp);

	//    T1initial(); // таймер времени
	//    T2initial(); // RS-485
	//    T4initial(); // отстутствие связи
	//    RS485_initial(); // инициализация порта UART

	const char sampleData[8] = __TIME__;

	FILEIO_ERROR_TYPE error;	

	MODULE_STATE moduleState = MODULE_STATE_NO_CARD; // MODULE_STATE_NO_CARD;	
	while (true)
	{
		SetModuleState(moduleState);
		switch (moduleState)
		{
			case MODULE_STATE_NO_CARD:
				// TODO: Искать карту
				// Loop on this function until the SD Card is detected.
				if (FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters) == true)
				{
					moduleState = MODULE_STATE_CARD_DETECTED;
				}
				break;
			case MODULE_STATE_CARD_DETECTED:
				// Инициализировать карту
				error = RINGSTORE_Open(&rStore, &gSdDrive, &sdCardMediaParameters);
				moduleState = error ?
						MODULE_STATE_FAILED : MODULE_STATE_CARD_INITIALIZED;
				break;
			case MODULE_STATE_CARD_INITIALIZED:
				// Записать буфер, если он готов
				error = RINGSTORE_StorePacket(&rStore, (const uint8_t*) sampleData, 8);
				if (error != FILEIO_ERROR_NONE)
				{
					moduleState = RINGSTORE_TryClose(&rStore) ?
							MODULE_STATE_FAILED : MODULE_STATE_NO_CARD;
				}

				error = FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters);
				if (error == false)
				{
					moduleState = RINGSTORE_TryClose(&rStore) ?
							MODULE_STATE_FAILED : MODULE_STATE_NO_CARD;
				}
				break;
			default:
				// TODO: Сигнализировать об аварийной ситуации и обработать её
				RINGSTORE_TryClose(&rStore);
				if (FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters) == false)
				{
					moduleState = MODULE_STATE_NO_CARD;
				}
				break;

		}
	}
}
//------------------------------------------------------------------------------

void GetTimestamp(FILEIO_TIMESTAMP * timeStamp)
{
	BSP_RTCC_DATETIME dateTime;

	dateTime.bcdFormat = false;

	RTCC_TimeGet(&dateTime);

	timeStamp->timeMs = 0;
	timeStamp->time.bitfield.hours = dateTime.hour;
	timeStamp->time.bitfield.minutes = dateTime.minute;
	timeStamp->time.bitfield.secondsDiv2 = dateTime.second / 2;

	timeStamp->date.bitfield.day = dateTime.day;
	timeStamp->date.bitfield.month = dateTime.month;
	// Years in the RTCC module go from 2000 to 2099.  Years in the FAT file system go from 1980-2108.
	timeStamp->date.bitfield.year = dateTime.year + 20;
	;
}
//------------------------------------------------------------------------------

void SetModuleState(MODULE_STATE state)
{
	switch (state)
	{
		case MODULE_STATE_NO_CARD:
			USER_SetLedWhite(false);
			USER_SetLedBlue(true);
			USER_SetLedRed(false);
			break;
		case MODULE_STATE_CARD_DETECTED:
			USER_SetLedWhite(false);
			USER_SetLedBlue(false);
			USER_SetLedRed(true);
			break;
		case MODULE_STATE_CARD_INITIALIZED:
			USER_SetLedWhite(false);
			USER_SetLedBlue(true);
			USER_SetLedRed(true);
			break;
		case MODULE_STATE_FAILED:
			USER_SetLedWhite(true);
			USER_SetLedBlue(false);
			USER_SetLedRed(false);
			break;
	}
}