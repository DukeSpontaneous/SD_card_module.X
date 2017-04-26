#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <xc.h>
#include <sd_spi.h>

#include "driver/spi/drv_spi.h"
#include "system_config.h"

// Definition for system clock
#define SYS_CLK_FrequencySystemGet()        40000000
// Definition for peripheral clock
#define SYS_CLK_FrequencyPeripheralGet()    SYS_CLK_FrequencySystemGet()
// Definition for instruction clock
#define SYS_CLK_FrequencyInstructionGet()   (SYS_CLK_FrequencySystemGet() / 2)

#define F (uint16_t)	(40/8)		// МГц/делитель для таймеров времени между посылками T2
#define TIME_TR			F * 200		// Установка таймера на us передачи
#define TIME_RX			F * 600		// Установка таймера на us для отслеживания посылок
#define TIME_BREAK_RX	F * 2000	// Для отслеживания срыва посылки - при работе с PC отключить обработку в _T2Interrupt case timerx_ob
#define TIME_FREE_LINE	F * 150

#define BAUDRATE 57600
#define FCY 40000000UL
#define BRGVAL   ((FCY/BAUDRATE)/16)-1
#define R_T	LATBbits.LATB7 // RS-485

// Таймер будет использоваться для нужд RS-485
void T2_Initialize(void);
// Инициализация порта UART1
void RS485_Initialize(void);
// System initialization function
void SYSTEM_Initialize(void);

inline void USER_SetLedBlue(bool stat);
inline void USER_SetLedRed(bool stat);
inline void USER_SetLedWhite(bool stat);