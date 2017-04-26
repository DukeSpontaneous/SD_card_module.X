#include "rs-485.h"

StatusRX statusRX; // Состояние приёма
StatusTR statusTR; // Состояние передачи

struct_flags_flash flags_flash;

uint8_t tr_frame[TX_BUF_SIZE], rx_frame[RX_BUF_SIZE]; // Буферы приёма и передачи
uint16_t bytesTransfer, bytesTransferTotal;

// RS-485

void __attribute__((interrupt, auto_psv)) _T2Interrupt(void)
{
	switch (PR2)
	{
		case TIME_TR:
			if (statusTR.turn == 1) // Передача данных разрешена
			{
				T2CONbits.TON = 0; // Выключаем таймер для отправки сообщения
				statusTR.tr_on = 1;
				statusTR.turn = 0; // Сброс очереди
				UART1_TR(); // Передача данных
			}
			break;
		case TIME_RX: // Промежуток между посылками
			statusRX.timer = 1;
			U1STAbits.OERR = 0;
			break;
		case TIME_BREAK_RX:// Обрыв приёма (отключить при тестированиее с PC)

			U1STAbits.OERR = 0;
			while (U1STAbits.URXDA == 1)
			{
				statusRX.temp_buffer = U1RXREG;
			}
			U1STAbits.URXISEL = 0;
			statusRX.status = 0;
			statusRX.bytesReceived = 0;
			TMR2 = 0;
			PR2 = TIME_RX;
			statusRX.error = 1;

			break;
		case TIME_FREE_LINE:
			PR2 = TIME_RX;
			R_T = 0; // Переводим max485 в режим приёма
			break;
	}
	IFS0bits.T2IF = 0;
}
//------------------------------------------------------------------------------

// Функция прерываний по приему UART1

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{
	TMR2 = 0; // Обнулить таймер
	if (statusRX.timer == 1) // Промежуток между посылками больше минимального
	{
		if (statusRX.isMyAddress == 0) // Если кадр для нашего устройства
		{
			rx_frame[0] = U1RXREG;
			if (rx_frame[0] == MODULE_ADDRESS_SD)
			{
				U1STAbits.URXISEL1 = 1; // Переключить на приём по 4 байта
				statusRX.isMyAddress = 1; // Адрес совпал
				statusRX.bytesReceived++;
				PR2 = TIME_BREAK_RX;
			} else //не совпал
			{
				if (rx_frame[0] != 0)
				{
					statusRX.timer = 0; // Сброс сработки таймера (посылка данных другому устройству)
				}
			}
		} else // Приём данных
		{
			if (statusRX.b4rx == 0) // Приём первых байт
			{
				while (U1STAbits.URXDA == 1) // Приём данных
				{
					rx_frame[statusRX.bytesReceived++] = U1RXREG;
				}
				statusRX.command = rx_frame[1]; // Код команды
				statusRX.bytesExpected = rx_frame[3] + 5; // Количество данных (все данные которые принимаются включая CRC и служебные)
				statusRX.b4rx = 1; // Служебная информация принята
				if (statusRX.bytesExpected == 5) // Если в кадре нет поля данных
				{
					statusRX.crc = gen_crc8(rx_frame, 4); // Вычисляем CRC
					if (statusRX.crc == rx_frame[4]) // CRC совпал
					{
						statusRX.status = 0; // Приём завершился удачно
					}
				} else// Есть данные для приёма
				{
					if (statusRX.bytesExpected < 9)
					{
						U1STAbits.URXISEL1 = 0; // Побайтовый режим приема
					}
				}
			} else// Данные
			{

				while (U1STAbits.URXDA == 1) // Приём данных
				{
					rx_frame[statusRX.bytesReceived++] = U1RXREG;
				}
				if (statusRX.bytesReceived < statusRX.bytesExpected)
				{
					if (statusRX.bytesExpected < statusRX.bytesReceived + 4)
					{
						U1STAbits.URXISEL1 = 0; // Побайтовый режим приёма
					}
				} else// Приём завершён
				{
					PR2 = TIME_TR;
					statusRX.crc = gen_crc8(rx_frame, statusRX.bytesReceived - 1);
					if (statusRX.crc == rx_frame[statusRX.bytesReceived - 1])
					{
						statusRX.status = 0; // Приём завершился удачно
						
						SLAVE_SD_MasterQueryProcessing(rx_frame, statusRX.bytesReceived);
						send_answer_status();
						CLRWDT();
						
					} else
					{
						statusRX.status = 0;
						statusRX.bytesReceived = 0;
						statusRX.error = 1;
						PR2 = TIME_RX;
					}
				}
			}
		}
	} else
	{
		U1STAbits.OERR = 0; // сброс ошибок буфера приема
		while (U1STAbits.URXDA == 1)//прием данных
		{
			rx_frame[0] = U1RXREG;
		}

	}

	IFS0bits.U1RXIF = 0;
}
//------------------------------------------------------------------------------

// Функция прерываний по завершению передачи UART1

void __attribute__((interrupt, auto_psv)) _U1TXInterrupt(void)
{
	if (bytesTransfer < bytesTransferTotal) // Все данные загружены в FIFO?
	{
		while (bytesTransfer < bytesTransferTotal) // Добавляем в буфер FIFO
		{
			if (U1STAbits.UTXBF == 1) // Буфер полон?
			{
				break;
			}
			U1TXREG = tr_frame[bytesTransfer++]; // Добавляем данные в буфер

			if (bytesTransfer == bytesTransferTotal) // Все данные загружены в FIFO, выходим, ожидаем передачу последнего байта
			{
				U1STAbits.UTXISEL1 = 0; // Переключаем прерывание на опустошение сдвигового регистра
				U1STAbits.UTXISEL0 = 1;
			}
		}
	} else // Передача завершена
	{
		if ((U1STAbits.TRMT == 0) && (U1STAbits.UTXISEL1 == 1))// Для посылки меньше 5 байт, проверяем на наличие данных в сдвиговом регистре
		{
			U1STAbits.UTXISEL1 = 0;
			U1STAbits.UTXISEL0 = 1; // Переключаем прерывание на опустошение сдвигового регистра
			IFS0bits.U1TXIF = 0; // Сброс прерываний по передаче
			return;
		}

		IEC0bits.U1TXIE = 0; // Отключаем прерывание по передаче

		statusTR.time = 0;
		statusTR.tr_on = 0;
		PR2 = TIME_FREE_LINE; // Время ожидания отпускания линии
		T2CONbits.TON = 1; // Включаем таймер для ожидания ответа
		U1STAbits.OERR = 0; // Сброс ошибок буфера приёма
		while (U1STAbits.URXDA == 1)
		{
			statusRX.temp_buffer = U1RXREG;
		}
		statusRX.b4rx = 0; // Очищаем состояние приёмника
		statusRX.bytesReceived = 0;
		statusRX.bytesExpected = 0;

		IFS0bits.U1RXIF = 0;
		IEC0bits.U1RXIE = 1;
	}

	IFS0bits.U1TXIF = 0; // Сброс прерываний по передаче
}
//------------------------------------------------------------------------------

// Передача по UART1

void UART1_TR(void)
{
	R_T = 1; // Перевести MAX485CSA в режим передачи
	bytesTransferTotal = bytesTransfer; // Количество передаваемых байт

	//IEC0bits.U1TXIE = 1; // Включить UART TX прерывания

	for (bytesTransfer = 0; bytesTransfer < bytesTransferTotal;)
	{
		U1TXREG = tr_frame[bytesTransfer++];

		if (U1STAbits.UTXBF == 1)
			break;
	}

	U1STAbits.UTXISEL0 = 1; // Переключаем прерывание в режим опустошения буффера
	U1STAbits.UTXISEL1 = 0;
	IEC0bits.U1TXIE = 1;

	/*
	//Подготовка к передаче первого байта для срабатывания обработчика _U1TXInterrupt
	num1_p = num1 - 1;//Количество байт для передачи - первый байт для срабатывания прерывания _U1TXInterrupt
	num1 = 0;//Индекс для доступа к массиву передачи

	 R_T1 = 1;//Перевести MAX485CSA в режим передачи
	 IEC0bits.U1TXIE = 1; // Включить UART TX прерывания

	 U1TXREG = buff_tr1[0];//Передача первого байта с генерацией прерывания _U1TXInterrupt
	 //Далее передача данных происходит самостоятельно через прерывание _U1TXInterrupt
	 */
}
//------------------------------------------------------------------------------

// Отправка буфера данных

void send_tr_frame(uint8_t length)
{
	bytesTransfer = length; // Устанавливаем количество байт для передачи
	IEC0bits.U1RXIE = 0;
	switch (statusTR.time) // Проверка на минимальное время между посылками
	{
		case 0: // время меньше минимально допустимого
			statusTR.turn = 1; //есть данные для отправки, данные поставлены в очередь
			statusTR.reply_ok = 0;
			break;
		case 1: // время больше минимально допустимого
			statusTR.turn = 0; // сброс очереди на отправку
			statusTR.tr_on = 1; // передатчик занят
			statusTR.reply_ok = 0;
			UART1_TR(); // отправляем данные
			break;
	}
}

//------------------------------------------------------------------------------

void send_answer_status(void)
{
	uint32_t totalMemory, freeMemory, freeClusters, sectorsize;

	sectorsize = FSectorSize(); // Делёный на 256

	totalMemory = (sectorsize * FTotalClusters()) >> 2; // Общий объём флеш в Kb (делёный на 4)

	// Общее деление на 1024

	freeClusters = FgetSetFreeCluster(); // Чтение из переменной обновляемой 1 раз в сек.
	if (freeClusters == 0xFFFFFFFF) freeClusters = 0; // Ошибка
	freeMemory = (sectorsize * freeClusters) >> 2; // Общий объём свободной памяти флеш в Kb

	tr_frame[0] = MODULE_ADDRESS_MASTER;
	tr_frame[1] = MODULE_ADDRESS_SD;
	tr_frame[2] = 0x09; // Количество байт 

	getSDFlags(&flags_flash);
	tr_frame[3] = flags_flash.char_flags; // Stat

	// Общее количество в Kb флеш
	tr_frame[4] = totalMemory >> 24; // Со старшего байта
	tr_frame[5] = totalMemory >> 16;
	tr_frame[6] = totalMemory >> 8;
	tr_frame[7] = totalMemory;

	// Свободное количество в Kb флеш
	tr_frame[8] = freeMemory >> 24; // Со старшего байта
	tr_frame[9] = freeMemory >> 16;
	tr_frame[10] = freeMemory >> 8;
	tr_frame[11] = freeMemory;

	tr_frame[12] = gen_crc8(tr_frame, 12); // CRC

	send_tr_frame(13);
}

//------------------------------------------------------------------------------

/*
  Name  : CRC-8
  Poly  : 0x31    x^8 + x^5 + x^4 + 1
  Init  : 0xFF
  Revert: false
  XorOut: 0x00
  Check : 0xF7 ("123456789")
  MaxLen: 15 байт(127 бит) - обнаружение
	одинарных, двойных, тройных и всех нечетных ошибок
 */
uint8_t gen_crc8(uint8_t *array, uint8_t length)
{
	uint8_t crc = 0xFF;
	uint8_t i;

	while (length--)
	{
		crc ^= *array++;

		for (i = 0; i < 8; i++)
			crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
	}

	return crc;
}
