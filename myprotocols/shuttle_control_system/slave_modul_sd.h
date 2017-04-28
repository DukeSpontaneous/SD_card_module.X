#pragma once

#include <stdint.h>

#include "system_time.h"
#include "ringstore_fat32.h"

#define RX_BUF_SIZE          261
#define TX_BUF_SIZE          261

typedef enum
{
	// Команды системы управления самоходным вагоном:	
	SHUTTLE_SYS_CMD_FLASH_STATUS = 0x80,
	SHUTTLE_SYS_CMD_FLASH_WRITE = 0x81
} SHUTTLE_SYSTEM_CMD;

typedef union
{
	uint8_t rx_bytes[RX_BUF_SIZE];

	struct
	{
		uint8_t destinationAddress;
		uint8_t command;
		uint16_t dataSize;

		union
		{
			uint8_t data_bytes[RX_BUF_SIZE -
			sizeof (uint8_t) - sizeof (uint8_t) -
			sizeof (uint16_t)];

			uint32_t globalSecondsTime1980;
		};
	};

} MASTER_FRAME;

typedef union
{
	uint8_t tx_bytes[TX_BUF_SIZE];

	struct
	{
		uint8_t destinationAddress;
		uint8_t senderAddress;
		uint8_t dataSize;

		uint8_t flags_flash;
		uint32_t totalKb;
		uint32_t freeKb;

		uint8_t crc;
	};

} SLAVE_SD_FRAME;


void SLAVE_SD_TaskProcessing(MASTER_FRAME *mFrame, uint16_t frameSize);