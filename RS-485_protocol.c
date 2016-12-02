#include "RS-485_protocol.h"
#include "pins.h"

StatusTR statusTR;
StatusRX statusRX;

uint8_t buff_tr[sizetrbuff], buff_rx[sizerxbuff];

void RS485_initial(void)// инициализация порта uart
{
	RPOR3bits.RP6R = 3; // TX1
	RPINR18 = 8; // RX1 RP8 UART1
	U1BRG = BRGVAL; // настройка битрейта
	R_T = 0;
	PR2 = timerx;
	U1MODE = 0b1010000000000000;
	U1STA = 0b1000010011000000;

	IPC2bits.U1RXIP = 5; // приоритет прерываний на прием и передачу
	IPC3bits.U1TXIP = 5;

	IFS0bits.U1RXIF = 0; // обнуление сработки прерываний
	IFS0bits.U1TXIF = 0;
	IEC0bits.U1RXIE = 1; // включение прерываний
	statusRX.address = ADDRESS_MODULE_SD;
	T2initial();
}
//------------------------------------------------------------------------------

void RS485_1_TR(void)// передача по uart1
{
	if (statusTR.tr_on == 1)
	{
		for (statusTR.sendedCount = 0; statusTR.sendedCount < statusTR.waitForSendCount;)
		{
			U1TXREG = buff_tr[statusTR.sendedCount++];

			if (U1STAbits.UTXBF == 1)
			{
				break;
			}
		}
	}
	IEC0bits.U1TXIE = 1;
}
//------------------------------------------------------------------------------

void turn_buffer(uint8_t j)// отправка буффера данных
{
	statusTR.waitForSendCount = j; // устанавливаем количество байт для передачи
	IEC0bits.U1RXIE = 0;
	switch (statusTR.time) // проверка на минимальное время между посылками
	{
		case 0: // время меньше минимально допустимого
			statusTR.turn = 1; //есть данные для отправки, данные поставлены в очередь
			break;
		case 1: // время больше минимально допустимого
			statusTR.turn = 0; // сброс очереди на отправку
			statusTR.tr_on = 1; // передатчик занят
			RS485_1_TR(); // отправляем данные
			break;
	}
}
//------------------------------------------------------------------------------