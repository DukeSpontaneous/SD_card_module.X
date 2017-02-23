#pragma once

#include <stdint.h>

typedef enum
{
	// Команды системы управления самоходным вагоном:	
	SHUTTLE_SYS_CMD_FLASH_INITIAL = 0x80,
	SHUTTLE_SYS_CMD_FLASH_WRITE = 0x81
} SHUTTLE_SYSTEM_CMD;

void SLAVE_SD_MasterQueryProcessing(uint8_t *in_buf, uint8_t *out_buf);