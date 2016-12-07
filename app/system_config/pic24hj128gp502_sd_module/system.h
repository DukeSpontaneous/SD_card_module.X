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

#define BLUE_LED		LATAbits.LATA0
#define RED_LED			LATAbits.LATA1
#define WHITE_LED		LATBbits.LATB9

#define R_T				LATBbits.LATB7 //de RS485
#define R_T1			LATBbits.LATB7 //de RS485

// System initialization function
void SYSTEM_Initialize (void);


