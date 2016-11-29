#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CRC_ENABLE true
#define SD_CARD_DEFAULT_TYPE SD_TYPE_NO_CARD

// Формат команды:
//	Номер_команды	Аргумент_команды		CRC7
// {{0x40 + CMDx}, {0x??, 0x??, 0x??, 0x??}, {0x??}}

// [47]			[46]				[45:40]			[39:8]		[7:1]	[0]
// 1			1					6				32			7		1
// '0'			'1'					x				x			x		'1'
// start bit	transmission bit	command index	argument	CRC7	end bit

#define SD_CMD_MASK (uint8_t) 0x40

typedef enum
{
	SD_CMD_GO_IDLE_STATE = SD_CMD_MASK | 0, // R1 *
	SD_CMD_SEND_OP_COND = SD_CMD_MASK | 1, // R1
	SD_CMD_SEND_IF_COND = SD_CMD_MASK | 8, // R7 *
	SD_CMD_SEND_CSD = SD_CMD_MASK | 9, // R1
	SD_CMD_SEND_CID = SD_CMD_MASK | 10, // R1
	SD_CMD_STOP_TRANSMISSION = SD_CMD_MASK | 12, // R1b
	SD_CMD_SEND_STATUS = SD_CMD_MASK | 13, // R2
	SD_CMD_SET_BLOCKLEN = SD_CMD_MASK | 16, // R1
	SD_CMD_READ_SINGLE_BLOCK = SD_CMD_MASK | 17, // R1 *
	SD_CMD_READ_MULTIPLE_BLOCKS = SD_CMD_MASK | 18, // R1
	SD_CMD_WRITE_BLOCK = SD_CMD_MASK | 24, // R1 *
	SD_CMD_WRITE_MULTIPLE_BLOCKS = SD_CMD_MASK | 25, // R1
	SD_CMD_ERASE_WR_BLK_START_ADDR = SD_CMD_MASK | 32, // R1
	SD_CMD_ERASE_WR_BLK_END_ADDR = SD_CMD_MASK | 33, // R1
	SD_CMD_ERASE = SD_CMD_MASK | 38, // R1b
	SD_CMD_APP_CMD = SD_CMD_MASK | 55, // R1 *
	SD_CMD_READ_OCR = SD_CMD_MASK | 58, // R3
	SD_CMD_CRC_ON_OFF = SD_CMD_MASK | 59 // R1
} SD_CMD;
//------------------------------------------------------------------------------

typedef enum
{
	SD_ACMD_SD_SEND_OP_COND = SD_CMD_MASK | 41, // R1 *
} SD_ACMD;
//------------------------------------------------------------------------------

typedef enum
{
	SD_TYPE_NO_CARD,
	SD_TYPE_UNKNOWN,
	SD_TYPE_DONT_IDLE,
	SD_TYPE_MMC,
	SD_TYPE_SD1,
	SD_TYPE_SD2,
	SD_TYPE_SD2HC
} SD_TYPE;
//------------------------------------------------------------------------------

typedef struct
{
	bool SD_crcEnabled;
	SD_TYPE SD_type;
} SD_CONFIG;
//------------------------------------------------------------------------------

#define SD_START_DATA_TOKEN (unsigned char) 0xFE

SD_TYPE SD_initialiseModeSPI();
bool SD_readSingle512Block(uint8_t destination[], uint32_t blockAddress);
bool SD_writeSingle512Block(uint8_t source[], uint32_t blockAddress);
bool SD_setConfig(uint16_t blocklen, bool crcEnabled);

extern SD_CONFIG SDConfig;