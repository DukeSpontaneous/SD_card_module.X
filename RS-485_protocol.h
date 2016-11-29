#pragma once

#include <stdint.h>

#define FCY                 40000000
#define F (uint16_t)		(40 / 8)	// Мгц/делитель для таймеров времени между посылками T2
#define timetr              200 * F		// установка таймера на  us передачи
#define timerx              6000 * F	// установка таймера на  us для отслеживания посылок (промежуток между посылками + 4 байта в фифо)
#define timerx_ob           1500 * F	// для отслеживания срыва посылки первые 4 байта принято, ожидание приема еще 4-х или одного байта
#define time_free_line      150 * F		// время удержания линии после передачи в передаче

#define BAUDRATE        57600
#define BRGVAL          ((FCY / BAUDRATE) / 16) - 1

typedef enum
{
	ADDRESS_MODULE_SD = 0x76, // Адрес SD-модуля
	ADDRESS_MODULE_MASTER = 0xFF // Адрес мастер-модуля
} ADDRESS_MODULE;

void RS485_initial(void);
void RS485_1_TR(void);
void turn_buffer(uint8_t j);

typedef struct
{

	union
	{
		uint8_t status;

		struct
		{
			unsigned turn : 1; // есть данные для передачи
			unsigned time : 1; // время между передачами превысило минимальное
			unsigned tr_on : 1; // передача запущена
		};
	};
	uint8_t sendedCount;
	uint8_t waitForSendCount;
} StatusTR;
//------------------------------------------------------------------------------

typedef struct
{

	union
	{
		uint16_t status;

		struct
		{
			unsigned error : 1; // ошибка приема
			unsigned indication : 1; //нет ошибок приема по линии связи для индикации
			unsigned timer : 1;
			unsigned address_agree : 1; // адрес устройства совпал
		};
	};
	uint16_t reseivedCount; // количество принятых  байт
	uint16_t waitForReseiveCount; // количество байт которые долны приняться
	uint8_t command;
	uint8_t crc;
	uint8_t temp_buffer;
	uint8_t address;
} StatusRX;
//------------------------------------------------------------------------------

#define sizerxbuff    70
#define sizetrbuff    30


extern StatusTR statusTR;
extern StatusRX statusRX;
extern uint8_t buff_tr[];
extern uint8_t buff_rx[];

void T2initial(void);
void T4initial(void);
void T1initial(void);