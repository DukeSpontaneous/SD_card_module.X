#pragma once

#include "fileio.h"
#include "sd_spi.h"

// ќбъ€вим возможные состо€ни€ работы модул€

typedef enum
{
	MODULE_STATE_NO_CARD,
	MODULE_STATE_CARD_DETECTED,
	MODULE_STATE_CARD_INITIALIZED,
	MODULE_STATE_FAILED
} MODULE_STATE;

// GetTimestamp will be called by the FILEIO library when it needs a timestamp for a file
void GetTimestamp(FILEIO_TIMESTAMP * timeStamp);

void SetModuleState(MODULE_STATE state);