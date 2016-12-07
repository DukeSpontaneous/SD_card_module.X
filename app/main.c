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

// ������� ��������� ��������� ������ ������

typedef enum
{
	MODULE_STATE_NO_CARD,
	MODULE_STATE_CARD_DETECTED,
	MODULE_STATE_CARD_INITIALIZED,
	MODULE_STATE_FAILED
} MODULE_STATE;

int main(void)
{
	BSP_RTCC_DATETIME dateTime;
	RINGSTORE_OBJECT rStore;

	SYSTEM_Initialize();

	dateTime.bcdFormat = false;
	RTCC_BuildTimeGet(&dateTime);
	RTCC_Initialize(&dateTime);

	// Initialize the library
	if (!FILEIO_Initialize())
	{
		while (1);
	}

	// Register the GetTimestamp function as the timestamp source for the library.
	FILEIO_RegisterTimestampGet(GetTimestamp);

	//    T1initial(); // ������ �������
	//    T2initial(); // RS-485
	//    T4initial(); // ����������� �����
	//    RS485_initial(); // ������������� ����� UART

	const char sampleData[5] = "Duke";

	FILEIO_ERROR_TYPE error;

	MODULE_STATE moduleState = MODULE_STATE_NO_CARD; // MODULE_STATE_NO_CARD;	
	while (true)
	{
		switch (moduleState)
		{
			case MODULE_STATE_NO_CARD:
				// TODO: ������ �����
				// Loop on this function until the SD Card is detected.
				if (FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters) == true)
				{
					moduleState = MODULE_STATE_CARD_DETECTED;
				}
				break;
			case MODULE_STATE_CARD_DETECTED:
				// ���������������� �����
				moduleState = RINGSTORE_Open(&rStore, &gSdDrive, &sdCardMediaParameters) ?
						MODULE_STATE_FAILED : MODULE_STATE_CARD_INITIALIZED;

				break;
			case MODULE_STATE_CARD_INITIALIZED:
				// �������� �����, ���� �� �����				
				error = RINGSTORE_StorePacket(&rStore, (const uint8_t*) sampleData, 4);
				if (error != FILEIO_ERROR_NONE)
				{
					moduleState = MODULE_STATE_FAILED;
				}

				if (FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters) == false)
				{
					moduleState = RINGSTORE_TryClose(&rStore) ?
							MODULE_STATE_FAILED : MODULE_STATE_NO_CARD;
				}
				break;
			default:
				// TODO: ��������������� �� ��������� �������� � ���������� �
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
