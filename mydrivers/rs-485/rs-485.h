#pragma once

#include <xc.h>

#include "rs_legasy.h"
#include "timers.h"
#include "system_time.h"
#include "slave_modul_sd.h"

#define R_T	LATBbits.LATB7 //de RS485

#define BAUDRATE 57600
#define FCY 40000000UL
#define BRGVAL   ((FCY/BAUDRATE)/16)-1
#define R_T1  LATBbits.LATB7//de rs485
#define CLRWDT() {__asm__ volatile ("clrwdt");}

#define BLUE_LED	LATAbits.LATA0
#define RED_LED		LATAbits.LATA1
#define WHITE_LED	LATBbits.LATB9

#define RX_BUF_SIZE          261
#define TX_BUF_SIZE          128

#define F (uint16_t)	(40/8)		// Мгц/делитель для таймеров времени между посылками T2
#define TIME_TR			F * 200		// установка таймера на  us передачи
#define TIME_RX			F * 600		// установка таймера на  us для отслеживания посылок
#define timerx_ob		F * 2000	// для отслеживания срыва посылки - при работе с PC отключить обработку в _T2Interrupt case timerx_ob
#define time_free_line	F * 150

#define ADDRESS_Ustroistva	0x76	//Адрес флеш-модуля
#define ADDRESS_Master		0xFF	//Адрес мастера

#define code_stats 0x80             //Команда запроса
#define code_write 0x81             //Команда записи

typedef union
{
	uint8_t status;

	struct
	{
		unsigned turn : 1; // есть данные для передачи
		unsigned time : 1; // время между передачами превысило минимальное
		unsigned tr_on : 1; // передача запущена
		unsigned rerun : 1; // повторная передача (если нет ответа от первой передачи )
		unsigned reply_ok : 1; // передача завершилась успешно
		unsigned wait_reply : 1; // ожидание ответной передачи
		unsigned eror : 1; // ошибка передачи
		unsigned type : 1; // тип передачи с ответом
	};

} StatusTR;
//------------------------------------------------------------------------------

typedef struct
{
	union
	{
		uint16_t status;

		struct
		{
			unsigned b4rx : 1; // принято минимум 4 байта
			unsigned eror : 1; // ошибка приема
			unsigned indic : 1; // нет ошибок приема по линии связи для индикации
			unsigned timer : 1;
			unsigned address_agree : 1; // адрес устройства совпал
		};
	};
	uint16_t numr; // количество принятых  байт
	uint16_t numkr; // количество байт которые долны приняться
	uint8_t command;
	uint8_t crc;
	uint8_t temp_buffer;
} StatusRX;
//------------------------------------------------------------------------------

typedef struct//Структура флагов и счётчиков
{
	unsigned long GlobalFreeCluster; //Количество свободных кластеров на флеше (обновляется в таймере 1 раз в сек.)
	unsigned char fread; //Флаг для чтения (0 - чтение разрешено, 1 - чтение запрещено, т.к. происходит запись)
	unsigned char lenrx; //Длина полученных данны в буфере через RS
	unsigned char spi_enter_flag; //Флаг входа в функцию SPI1_rw (чтение/запись байта по SPI) для предотвращения повторного входа в функцию
	unsigned char CountGetFreeClasters; //Счётчик для вычитывания количества свободных кластеров с флешки
	unsigned char NeedWriteFlushBufLog; //Индикатор сброса буфера на флеш через время TimeForFlushPeriodSec (0 - сброс не требуется, 1 - требуется сброс на флеш)
	unsigned char VariableCounter; //Задержка после включения (2сек или 5сек)
} Flags_Counts_Datas;

typedef struct
{
	//unsigned char mas[1010];
	unsigned int rs_buff_counter;
	unsigned char rs_buff[512];
	unsigned long sec_from_1980;
	unsigned long returned_sec_from_1980;
	unsigned char string[256];
	unsigned int str_len;
} FlashParams;

void status(void);
void status_err_read(void);
void readedMessage(uint8_t *str, uint8_t len, uint32_t time);
uint16_t CRC16(uint8_t *item, uint8_t lngth);

uint8_t Crc8(uint8_t *pcBlock, uint8_t len);
void RS485_initial(void);
void RS485_1_TR(void);
void RS485_TR_buff(uint8_t *str);

