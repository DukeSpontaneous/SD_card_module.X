/********************************************************************
 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the "Company") for its PIC(R) Microcontroller is intended and
 supplied to you, the Company's customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *******************************************************************/

#include "system_config.h"
#include "system.h"
#include <xc.h>
#include <stdbool.h>
#include <sd_spi.h>

/** CONFIGURATION Bits **********************************************/
// PIC24HJ128GP502 Configuration Bit Settings

// 'C' source line config statements

// FBS
#pragma config BWRP = WRPROTECT_OFF     // Boot Segment Write Protect (Boot Segment may be written)
#pragma config BSS = NO_FLASH           // Boot Segment Program Flash Code Protection (No Boot program Flash segment)
#pragma config RBS = NO_RAM             // Boot Segment RAM Protection (No Boot RAM)

// FSS
#pragma config SWRP = WRPROTECT_OFF     // Secure Segment Program Write Protect (Secure segment may be written)
#pragma config SSS = NO_FLASH           // Secure Segment Program Flash Code Protection (No Secure Segment)
#pragma config RSS = NO_RAM             // Secure Segment Data RAM Protection (No Secure RAM)

// FGS
#pragma config GWRP = OFF               // General Code Segment Write Protect (User program memory is not write-protected)
#pragma config GSS = OFF                // General Segment Code Protection (User program memory is not code-protected)

// FOSCSEL
#pragma config FNOSC = PRIPLL           // Oscillator Mode (Primary Oscillator (XT, HS, EC) w/ PLL)
#pragma config IESO = ON                // Internal External Switch Over Mode (Start-up device with FRC, then automatically switch to user-selected oscillator source when ready)

// FOSC
#pragma config POSCMD = XT              // Primary Oscillator Source (XT Oscillator Mode)
#pragma config OSCIOFNC = OFF           // OSC2 Pin Function (OSC2 pin has clock out function)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow Only One Re-configuration)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor (Both Clock Switching and Fail-Safe Clock Monitor are disabled)

// FWDT
#pragma config WDTPOST = PS32768        // Watchdog Timer Postscaler (1:32,768)
#pragma config WDTPRE = PR128           // WDT Prescaler (1:128)
#pragma config WINDIS = OFF             // Watchdog Timer Window (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (Watchdog timer enabled/disabled by user software)

// FPOR
#pragma config FPWRT = PWR128           // POR Timer Value (128ms)
#pragma config ALTI2C = OFF             // Alternate I2C  pins (I2C mapped to SDA1/SCL1 pins)

// FICD
#pragma config ICS = PGD2               // Comm Channel Select (Communicate on PGC2/EMUC2 and PGD2/EMUD2)
#pragma config JTAGEN = OFF             // JTAG Port Enable (JTAG is Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

void SYSTEM_Initialize(void)
{
	PLLFBD = 38; // M = 32
	CLKDIVbits.PLLPOST = 0; // N1 = 2
	CLKDIVbits.PLLPRE = 0; // N2 = 2
	// Initiate Clock Switch to Primary Oscillator with PLL (NOSC = 0b011)
	__builtin_write_OSCCONH(0x03);
	__builtin_write_OSCCONL(0x01);
	// Wait for Clock switch to occur
	while (OSCCONbits.COSC != 0b011)
	{
	};
	// Wait for PLL to lock
	while (OSCCONbits.LOCK != 1)
	{
	};
	AD1PCFGL = 0xFFFF; // конфигурация цифровые I\O
	TRISAbits.TRISA0 = 0; /*led1*/
	TRISAbits.TRISA1 = 0; /*led2*/
	TRISAbits.TRISA4 = 1; //vdd_on_off
	TRISBbits.TRISB9 = 0; /*led3*/
	TRISBbits.TRISB1 = 0; /*DO*/
	TRISBbits.TRISB2 = 0; /*sck*/
	TRISBbits.TRISB3 = 1; /*si*/
	TRISBbits.TRISB7 = 0; /*de rs485*/
	TRISBbits.TRISB6 = 0; /*out rs485*/
	TRISBbits.TRISB8 = 1; /*in rs485*/
	TRISBbits.TRISB15 = 1; /*input mikric (0 - закрыт, 1 - открыт)*/

	USER_SetLedRed(false);
	USER_SetLedBlue(false);
	USER_SetLedWhite(false);
}

void USER_SdSpiConfigurePins(void)
{
	// Configure SPI1 PPS pins

	RPOR1bits.RP2R = 8; // assign RP? for SCK1
	RPOR0bits.RP1R = 7; // assign RP? for SDO1
	RPINR20bits.SDI1R = 3; // assign RP? for SDI1 /*RB3*/

	// Deassert the chip select pin
	LATBbits.LATB0 = 1;
	// Configure CS pin as an output
	TRISBbits.TRISB0 = 0;
	// Configure CD pin as an input
	TRISBbits.TRISB4 = 1;
	// Configure WP pin as an input
	TRISBbits.TRISB5 = 1;

	// Configure SPI1...

	SPI1CON1bits.DISSCK = 0;
	SPI1CON1bits.DISSDO = 0;
	SPI1CON1bits.MODE16 = 0;
	SPI1CON1bits.SMP = 0;
	SPI1CON1bits.CKE = 0;
	// SPI1CON1bits.SSEN = 0;
	SPI1CON1bits.CKP = 1;
	SPI1CON1bits.MSTEN = 1;

	// SPRE<2:0>: Secondary Prescale bits (Master mode) (2)
	// 111 = Secondary prescale 1:1
	// 110 = Secondary prescale 2:1
	// ...
	// 000 = Secondary prescale 8:1
	SPI1CON1bits.SPRE = 0b101; //13MHZ 101

	// PPRE<1:0>: Primary Prescale bits (Master mode) (2)
	// 11 = Primary prescale 1:1
	// 10 = Primary prescale 4:1
	// 01 = Primary prescale 16:1
	// 00 = Primary prescale 64:1
	SPI1CON1bits.PPRE = 0b11;

	SPI1CON2bits.FRMEN = 0;
	SPI1CON2bits.SPIFSD = 0;
	SPI1CON2bits.FRMPOL = 0;
	SPI1CON2bits.FRMDLY = 0;

	SPI1STATbits.SPIEN = 1;
	SPI1STATbits.SPISIDL = 0;
	SPI1STATbits.SPIROV = 0;

	SPI1STATbits.SPIEN = 1;
}

inline void USER_SdSpiSetCs(uint8_t a)
{
	LATBbits.LATB0 = a;
}

inline bool USER_SdSpiGetCd(void)
{
	// На этих модулях RB4 почему-то всегда в логическом нуле,
	// поэтому пытаться
	return (!PORTBbits.RB4 && !PORTBbits.RB15) ? true : false;
}

inline bool USER_SdSpiGetWp(void)
{
	return (PORTBbits.RB5) ? true : false;
}

// The sdCardMediaParameters structure defines user-implemented functions needed by the SD-SPI fileio driver.
// The driver will call these when necessary.  For the SD-SPI driver, the user must provide
// parameters/functions to define which SPI module to use, Set/Clear the chip select pin,
// get the status of the card detect and write protect pins, and configure the CS, CD, and WP
// pins as inputs/outputs (as appropriate).
// For this demo, these functions are implemented in system.c, since the functionality will change
// depending on which demo board/microcontroller you're using.
// This structure must be maintained as long as the user wishes to access the specified drive.
FILEIO_SD_DRIVE_CONFIG sdCardMediaParameters = {
	1, // Use SPI module 1
	USER_SdSpiSetCs, // User-specified function to set/clear the Chip Select pin.
	USER_SdSpiGetCd, // User-specified function to get the status of the Card Detect pin.
	USER_SdSpiGetWp, // User-specified function to get the status of the Write Protect pin.
	USER_SdSpiConfigurePins // User-specified function to configure the pins' TRIS bits.
};

inline void USER_SetLedBlue(bool stat)
{
	LATAbits.LATA0 = stat ? false : true;
}

inline void USER_SetLedRed(bool stat)
{
	LATAbits.LATA1 = stat ? false : true;
}

inline void USER_SetLedWhite(bool stat)
{
	LATBbits.LATB9 = stat;
}