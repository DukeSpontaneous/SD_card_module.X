#pragma once

#include <time.h>

#include "fileio.h"

void SYSTEM_TIME_Initialize();
void SYSTEM_TIME_Update(uint32_t time_1980);

// GetTimestamp will be called by the FILEIO library when it needs a timestamp for a file
void GetTimestamp(FILEIO_TIMESTAMP * timeStamp);