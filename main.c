#include <stdbool.h>
#include <string.h>

#include "configbits.h"
#include "pins.h"
#include "SPI_protocol.h"
#include "SD_protocol.h"
#include "myFAT32.h"

// Объявим возможные состояния работы модуля

typedef enum
{
	MODULE_STATE_NO_CARD,
	MODULE_STATE_CARD_DETECTED,
	MODULE_STATE_CARD_INITIALIZED,
	MODULE_STATE_FAILED
} MODULE_STATE;

static bool tryValidationCard()
{
	SDConfig.SD_type = SD_initialiseModeSPI();
	if (
			SDConfig.SD_type != SD_TYPE_UNKNOWN &&
			SDConfig.SD_type != SD_TYPE_DONT_IDLE &&
			SDConfig.SD_type != SD_TYPE_NO_CARD)
	{
		if (!SD_setConfig(BYTE_PER_SECTOR, SDConfig.SD_crcEnabled))
			return false;

		if (!FS_getMyFAT32Info(&LogacalDriveInfo))
			return false;

		// Помимо прочего устанавливает курсор
		switch (FS_checkMyFAT32(&LogacalDriveInfo))
		{
			case MY_FAT32_ERROR_OK: // Если формат нормальный, то можно продолжать работать с ним:				
				break;
			case MY_FAT32_ERROR_FORMAT: // Если формат имеет отклонение, то пытаемся отформатировать:				
				if (FS_formattingFAT32ToMyFAT32(&LogacalDriveInfo) != MY_FAT32_ERROR_OK)
					return false; // TODO: инициализация завершена аварийно!
				break;
			case MY_FAT32_ERROR_CALCULATION:
				return false; // TODO: инициализация завершена аварийно!
				break;
			default:
				return false; // TODO: инициализация завершена аварийно!
				break;
		}

		// 24584 - 512 0xFF (было)

	} else
	{
		return false;
	}

	return true;
}
//------------------------------------------------------------------------------

void main(void)
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
	TRISBbits.TRISB0 = 0; /*cs_spi*/
	TRISBbits.TRISB1 = 0; /*DO*/
	TRISBbits.TRISB2 = 0; /*sck*/
	TRISBbits.TRISB3 = 1; /*si*/
	TRISBbits.TRISB4 = 1; /*CD*/
	TRISBbits.TRISB5 = 1; /*WP*/
	TRISBbits.TRISB7 = 0; /*de rs485*/
	TRISBbits.TRISB6 = 0; /*out rs485*/
	TRISBbits.TRISB8 = 1; /*in rs485*/
	TRISBbits.TRISB4 = 1; /*cD cheak*/
	TRISBbits.TRISB5 = 1; /*wp cheak*/
	TRISBbits.TRISB15 = 1; /*input mikric (0 - закрыт, 1 - открыт)*/

	RED_LED = 1; //off
	WHITE_LED = 0; //off
	BLUE_LED = 1; //off

	SPI_initial(); // инициализация SPI для флешки
	//    T1initial(); // таймер времени
	//    T2initial(); // RS-485
	//    T4initial(); // отстутствие связи
	//    RS485_initial(); // инициализация порта UART

	const char data[] = "...Testing";
	MODULE_STATE moduleState = MODULE_STATE_CARD_DETECTED; // MODULE_STATE_NO_CARD;	
	while (true)
	{
		switch (moduleState)
		{
			case MODULE_STATE_NO_CARD:
				// Искать карту
				break;
			case MODULE_STATE_CARD_DETECTED:
				// Инициализировать карту				
				moduleState = tryValidationCard() ?
						MODULE_STATE_CARD_INITIALIZED : MODULE_STATE_FAILED;
				break;
			case MODULE_STATE_CARD_INITIALIZED:				
				// Записать буфер, если он готов
				if (!FS_loggingPacket((const uint8_t*) data, strlen(data)))
					moduleState = MODULE_STATE_FAILED;
				break;
			default:
				// Сигнализировать об аварийной ситуации и обработать её
				while (true);
				break;

		}
	}
}
//------------------------------------------------------------------------------