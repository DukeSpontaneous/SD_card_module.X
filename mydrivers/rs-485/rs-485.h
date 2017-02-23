#pragma once

#include <xc.h>

#include "rs_legasy.h"
#include "timers.h"
#include "system_time.h"
#include "slave_modul_sd.h"

#define R_T	LATBbits.LATB7 //de RS485

typedef enum
{
	// Адреса системы модулей:
	MODULE_ADDRESS_SD = 0x76, // Адрес флеш-модуля
	MODULE_ADDRESS_MASTER = 0xFF // Адрес мастера
} MODULE_ADDRESS;


#define FCY				40000000UL
#define BAUDRATE		57600
#define BRGVAL			((FCY/BAUDRATE)/16)-1

#define F (unsigned int)	(40/8)	// МГц/делитель для таймеров времени между посылками T2
#define timetr				F*200	// установка таймера на  us передачи
#define timerx				F*1550	// установка таймера на  us для отслеживания посылок (промежуток между посылками + 4 байта в фифо)
#define timerx_ob			F*1500	// для отслеживания срыва посылки первые 4 байта принято, ожидание приема еще 4-х или одного байта
#define time_free_line		F*150	// время удержания линии после передачи в передаче
#define sizerxbuff			128

typedef struct
{

	union
	{
		unsigned char status;

		struct
		{
			unsigned turn : 1; // есть данные для передачи
			unsigned time : 1; // время между передачами превысило минимальное
			unsigned tr_on : 1; // передача запущена
		};
	};
	unsigned char k_vo_peredano;
	unsigned char k_vo_na_peredachu;
} StatusTR;
//------------------------------------------------------------------------------

typedef struct
{

	union
	{
		unsigned int status;

		struct
		{
			unsigned eror : 1; // ошибка приема
			unsigned indic : 1; //нет ошибок приема по линии связи для индикации
			unsigned timer : 1;
			unsigned adres_agree : 1; // адрес устройства совпал
		};
	};
	unsigned int k_vo_priniatix; // количество принятых  байт
	unsigned int k_vo_dolzno_priniatsy; // количество байт которые долны приняться
	unsigned char command;
	unsigned char crc;
	unsigned char temp_buffer;
	unsigned char adres;
} StatusRX;
//------------------------------------------------------------------------------

void RS485_Initialize(void);
unsigned char Crc8(unsigned char *pcBlock, unsigned char len);
void RS485_1_TR(void);

void turn_tr_buffer(unsigned char j);
void send_answer_status();