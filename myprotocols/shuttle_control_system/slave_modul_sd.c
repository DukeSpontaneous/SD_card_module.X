#include "slave_modul_sd.h"

// TODO: проверить доступность
extern RINGSTORE_OBJECT rStore;

void SLAVE_SD_TaskProcessing(MASTER_FRAME *mFrame, uint16_t frameSize)
{
	if (frameSize < 8 || frameSize < (4 + mFrame->dataSize))
		return;

	if (rStore.isAvailable == true)
	{
		rStore.isAvailable = false;
		SYSTEM_TIME_Update(mFrame->globalSecondsTime1980);
		rStore.isAvailable = true;
	}

	switch (mFrame->command) // Обработка полученных данных
	{
		case SHUTTLE_SYS_CMD_FLASH_STATUS:

			break;
		case SHUTTLE_SYS_CMD_FLASH_WRITE:

			if (rStore.isAvailable == true)
			{
				rStore.isAvailable = false;
				RINGSTORE_StorePacket(&rStore,
						(const uint8_t*) mFrame->data_bytes,
						mFrame->dataSize);
				rStore.isAvailable = true;
			}

			break;
		default:

			break;
	}
}
