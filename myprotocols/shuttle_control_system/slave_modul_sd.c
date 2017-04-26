#include "slave_modul_sd.h"

// TODO: проверить доступность
extern RINGSTORE_OBJECT rStore;
extern uint32_t globalSecondsTime1980;

void SLAVE_SD_MasterQueryProcessing(uint8_t *incomingFrame, uint16_t frameSize)
{
	if (frameSize < 8)
		return;

	SHUTTLE_SYSTEM_CMD cmd = incomingFrame[1];

	uint16_t dataSize =
			(incomingFrame[2] << 8 * 1) +
			(incomingFrame[3] << 8 * 0);

	if (frameSize < (4 + dataSize))
		return;

	if (rStore.FLAG_IsAvailable == true)
	{
		rStore.FLAG_IsAvailable = false;

		globalSecondsTime1980 =
				(incomingFrame[4] << 8 * 3) +
				(incomingFrame[5] << 8 * 2) +
				(incomingFrame[6] << 8 * 1) +
				(incomingFrame[7] << 8 * 0);

		rStore.FLAG_IsAvailable = true;
	}

	switch (cmd) // Обработка полученных данных
	{
		case SHUTTLE_SYS_CMD_FLASH_STATUS:

			break;
		case SHUTTLE_SYS_CMD_FLASH_WRITE:

			if (rStore.FLAG_IsAvailable == true)
			{
				rStore.FLAG_IsAvailable = false;

				uint8_t *data = incomingFrame + 4;
				RINGSTORE_StorePacket(&rStore,
						(const uint8_t*) data,
						dataSize);

				rStore.FLAG_IsAvailable = true;
			}

			break;
		default:

			break;
	}
}
