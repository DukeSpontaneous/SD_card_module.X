#pragma once

#include <xc.h>

#include "main.h"
#include "fileio_private.h"

typedef union
{
	uint8_t char_flags;

	struct
	{
		unsigned on_off : 1;
		unsigned wp_on_off : 1;
		unsigned micro_in_off : 1;
		unsigned vdd_on_off : 1;
		unsigned flash_initialized : 1;
		unsigned flash_init_answer : 1;
		unsigned delay_before_init : 1;
		unsigned start_write : 1;
	};
} struct_flags_flash;

uint32_t FSectorSize(void);
uint32_t FTotalClusters(void);
uint32_t FgetSetFreeCluster(void);

void getSDFlags(struct_flags_flash *flags_flash);
void swapBytes(uint8_t array[], size_t size);