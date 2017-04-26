#pragma once

#include <xc.h>

#include "rs_legasy.h"
#include "system_time.h"
#include "slave_modul_sd.h"

#define RX_BUF_SIZE          261
#define TX_BUF_SIZE          128

typedef enum
{
	// Адреса системы модулей:
	MODULE_ADDRESS_SD = 0x76, // Адрес флеш-модуля
	MODULE_ADDRESS_MASTER = 0xFF // Адрес мастера
} MODULE_ADDRESS;

#define code_stats 0x80	// Команда запроса
#define code_write 0x81	// Команда записи

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
		unsigned error : 1; // ошибка передачи
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
			unsigned b4rx : 1; // Принято минимум 4 байта
			unsigned error : 1; // Ошибка приёма
			unsigned indic : 1; // Нет ошибок приёма по линии связи для индикации
			unsigned timer : 1;
			unsigned isMyAddress : 1; // Адрес устройства совпал
		};
	};
	uint16_t bytesReceived; // Количество принятых байт
	uint16_t bytesExpected; // Количество байт которые должны приняться
	uint8_t command;
	uint8_t crc;
	uint8_t temp_buffer;
} StatusRX;
//------------------------------------------------------------------------------

uint8_t gen_crc8(uint8_t *array, uint8_t length);
void send_answer_status(void);

//void T2_Initialize(void);
//void RS485_Initialize(void);
void UART1_TR(void);