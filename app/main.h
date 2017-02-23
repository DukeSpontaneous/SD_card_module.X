#pragma once

#include <xc.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "system.h"
#include "fileio.h"
#include "sd_spi.h"

#include "ringstore_fat32.h"

#include "rs_legasy.h"

typedef enum
{
	// Объявим возможные состояния работы модуля:
	
	MODULE_STATE_NO_CARD,
	MODULE_STATE_CARD_DETECTED,
	MODULE_STATE_CARD_INITIALIZED,
	MODULE_STATE_FAILED
} MODULE_STATE;

// GetTimestamp will be called by the FILEIO library when it needs a timestamp for a file
void GetTimestamp(FILEIO_TIMESTAMP * timeStamp);

void SetModuleState(MODULE_STATE state);