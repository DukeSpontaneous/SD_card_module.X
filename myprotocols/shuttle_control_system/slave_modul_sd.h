#pragma once

#include <stdint.h>

#include "system_time.h"
#include "ringstore_fat32.h"

typedef enum
{
	// Команды системы управления самоходным вагоном:	
	SHUTTLE_SYS_CMD_FLASH_STATUS = 0x80,
	SHUTTLE_SYS_CMD_FLASH_WRITE = 0x81
} SHUTTLE_SYSTEM_CMD;

void SLAVE_SD_MasterQueryProcessing(uint8_t *incomingFrame, uint16_t frameSize);