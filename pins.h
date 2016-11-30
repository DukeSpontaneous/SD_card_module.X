#pragma once

#include <xc.h>

#define SD_CS_ASSERT	LATBbits.LATB0 = 0
#define SD_CS_DEASSERT	LATBbits.LATB0 = 1
#define R_T1			LATBbits.LATB7 //de RS485

#define BLUE_LED		LATAbits.LATA0
#define RED_LED			LATAbits.LATA1
#define WHITE_LED		LATBbits.LATB9
#define R_T				LATBbits.LATB7 //de RS485
