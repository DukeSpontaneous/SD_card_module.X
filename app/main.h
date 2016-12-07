#pragma once

#include "fileio.h"
#include "sd_spi.h"

// GetTimestamp will be called by the FILEIO library when it needs a timestamp for a file
void GetTimestamp(FILEIO_TIMESTAMP * timeStamp);