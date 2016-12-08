#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "driver/spi/drv_spi.h"

// Definition for system clock
#define SYS_CLK_FrequencySystemGet()        32000000
// Definition for peripheral clock
#define SYS_CLK_FrequencyPeripheralGet()    SYS_CLK_FrequencySystemGet()
// Definition for instruction clock
#define SYS_CLK_FrequencyInstructionGet()   (SYS_CLK_FrequencySystemGet() / 2)

#define R_T				LATBbits.LATB7 //de RS485
#define R_T1			LATBbits.LATB7 //de RS485

// System initialization function
void SYSTEM_Initialize (void);


inline void USER_SetLedBlue(bool stat);
inline void USER_SetLedRed(bool stat);
inline void USER_SetLedWhite(bool stat);