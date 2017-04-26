#include "rs-485.h"

StatusRX statusRX; // состояние приема
StatusTR statusTR; // состояние передачи

struct_flags_flash flags_flash;
FlashParams flash_params;
uint8_t buff_tr[TX_BUF_SIZE], buff_rx[RX_BUF_SIZE]; // буфера приема и передачи
uint16_t num1, num1_p;

Flags_Counts_Datas FDatas; //Структура для работы с флагоми и счётчиками

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void) // функция прерываний по приему UART1
{
	TMR2 = 0; // обнулить таймер
	if (statusRX.timer == 1) // промежуток между посылками больше минимального
	{
		if (statusRX.address_agree == 0) // адрес устройства не совпал
		{
			buff_rx[0] = U1RXREG;
			if (buff_rx[0] == ADDRESS_Ustroistva) // сравниваем адрес(совпал)
			{
				U1STAbits.URXISEL1 = 1; // переключить на приём по 4 байта
				statusRX.address_agree = 1; // адрес совпал
				statusRX.numr++;
				PR2 = timerx_ob;
			} else //не совпал
			{
				if (buff_rx[0] != 0)
				{
					statusRX.timer = 0; // сброс сработки таймера (посылка данных другому устройству)
				}
			}
		} else//прием данных
		{
			if (statusRX.b4rx == 0) // прием первых байт
			{
				while (U1STAbits.URXDA == 1) // прием данных
				{
					buff_rx[statusRX.numr++] = U1RXREG;
				}
				statusRX.command = buff_rx[1]; // коммандный байт
				statusRX.numkr = buff_rx[3] + 5; // количество данных (все данные которые принимаются включая crc и служебные)
				statusRX.b4rx = 1; // служебная информация принята
				if (statusRX.numkr == 5) // если количество данных 0
				{

					statusRX.crc = Crc8(buff_rx, 4); // считаем контрольную сумму
					if (statusRX.crc == buff_rx[4]) // контрольная сумма совпала
					{
						statusRX.status = 0; //прием завершился удачно
					}
				} else//есть данные для приема
				{
					if (statusRX.numkr < 9)
					{
						U1STAbits.URXISEL1 = 0; // побайтовый режим приема
					}
				}
			} else//данные
			{

				while (U1STAbits.URXDA == 1) // прием данных
				{
					buff_rx[statusRX.numr++] = U1RXREG;
				}
				if (statusRX.numr < statusRX.numkr)
				{
					if (statusRX.numkr < statusRX.numr + 4)
					{
						U1STAbits.URXISEL1 = 0; // побайтовый режим приема
					}
				} else//прием завершен
				{
					PR2 = TIME_TR;
					statusRX.crc = Crc8(buff_rx, statusRX.numr - 1);
					if (statusRX.crc == buff_rx[statusRX.numr - 1])
					{
						statusRX.status = 0; //прием завершился удачно
						switch (statusRX.command) //обработка полученных данных
						{
							case code_stats: //Вернуть состояние
							{
								status();
								break;
							}
							case code_write: //Попытка записи пришедшего сообщения в буфер
							{
								status(); //Флеш не готов или происходит сброс данных на флеш
								break;
							}
						}
					} else
					{
						statusRX.status = 0;
						statusRX.numr = 0;
						statusRX.eror = 1;
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
			buff_rx[0] = U1RXREG;
		}

	}

	IFS0bits.U1RXIF = 0;
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _U1TXInterrupt(void) // функция прерываний по завершению передачи UART1
{
	if (num1 < num1_p) // все данные загружены в FIFO ?
	{
		while (num1 < num1_p) // добавляем в буфер FIFO
		{
			if (U1STAbits.UTXBF == 1)// буфер полон ?
			{
				break;
			}
			U1TXREG = buff_tr[num1++]; // добавляем данные в буфер

			if (num1 == num1_p) // все данные загружены в FIFO, выходим ожидаем передачу последнего байта
			{
				U1STAbits.UTXISEL1 = 0; // переключаем прерывание на опустошение сдвигового регистра
				U1STAbits.UTXISEL0 = 1;
			}
		}
	} else // передача завершена
	{
		if ((U1STAbits.TRMT == 0) && (U1STAbits.UTXISEL1 == 1))// для посылки меньше 5 байт, проверяем на наличие данных в сдвиговом регистре
		{
			U1STAbits.UTXISEL1 = 0;
			U1STAbits.UTXISEL0 = 1; // переключаем прерывание на опустошение сдвигового регистра
			IFS0bits.U1TXIF = 0; // сброс прерываний по передаче
			return;
		}

		IEC0bits.U1TXIE = 0; // отключаем прерывание по передаче

		statusTR.time = 0;
		statusTR.tr_on = 0;
		PR2 = time_free_line; // время ожидания отпускания линии
		T2CONbits.TON = 1; // включаем таймер для ожидания ответа
		U1STAbits.OERR = 0; // сброс ошибок буфера приема
		while (U1STAbits.URXDA == 1)
		{
			statusRX.temp_buffer = U1RXREG;
		}
		statusRX.b4rx = 0; // очищаем состояние приемника
		statusRX.numr = 0;
		statusRX.numkr = 0;

		IFS0bits.U1RXIF = 0;
		IEC0bits.U1RXIE = 1;
	}

	IFS0bits.U1TXIF = 0; // сброс прерываний по передаче
}
//----------------

void __attribute__((interrupt, auto_psv)) _T2Interrupt(void)// rs-485
{
	switch (PR2)
	{
		case TIME_TR:
			if (statusTR.turn == 1) // передача данных разрешена
			{
				T2CONbits.TON = 0; // выключаем таймер для отправки сообщения
				statusTR.tr_on = 1;
				statusTR.turn = 0; // сброс очереди
				RS485_1_TR(); // передача данных
			}
			break;
		case TIME_RX: // помежуток между посылками
			statusRX.timer = 1;
			U1STAbits.OERR = 0;
			break;
		case timerx_ob:// обрыв приема (отключить при тестированиее с PC)

			U1STAbits.OERR = 0;
			while (U1STAbits.URXDA == 1)
			{
				statusRX.temp_buffer = U1RXREG;
			}
			U1STAbits.URXISEL = 0;
			statusRX.status = 0;
			statusRX.numr = 0;
			TMR2 = 0;
			PR2 = TIME_RX;
			statusRX.eror = 1;

			break;
		case time_free_line:
			PR2 = TIME_RX;
			R_T1 = 0; // переводим max485 в режим приема
			break;
	}
	IFS0bits.T2IF = 0;
}

void RS485_initial(void)// инициализация порта uart
{
	RPOR3bits.RP6R = 3; // TX1
	RPINR18 = 8; // RX1 RP8 uart1
	U1BRG = BRGVAL; // настройка битрейта
	U1MODE = 0b1010000000000000;
	U1STA = 0b1000010000000000;

	IPC2bits.U1RXIP = 7; // приоритет прерываний на прием и передачу
	IPC3bits.U1TXIP = 7;
	IFS0bits.U1RXIF = 0; // обнуление сработки прерываний
	IFS0bits.U1TXIF = 0;
	IEC0bits.U1RXIE = 1; // включение прерываний
	PR2 = TIME_RX;
}

//------------------------------------------------------------------------------

void RS485_1_TR(void)// передача по uart1
{
	R_T1 = 1; //Перевести MAX485CSA в режим передачи
	num1_p = num1; //Количество передаваемых байт

	//IEC0bits.U1TXIE = 1; // Включить UART TX прерывания

	for (num1 = 0; num1 < num1_p;)
	{
		U1TXREG = buff_tr[num1++];

		if (U1STAbits.UTXBF == 1)
		{
			break;
		}

	}
	U1STAbits.UTXISEL0 = 1; // переключаем прерывание в режим опустошения буффера
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

void RS485_TR_buff(uint8_t *str)
{
	unsigned char str_len = 0;
	R_T1 = 1;
	while (str[str_len] != 0 && str_len < 100)
	{
		while (U1STAbits.UTXBF == 1)
		{
		}
		U1TXREG = str[str_len];
		str_len++;
	}
}

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
uint8_t Crc8(uint8_t *pcBlock, uint8_t len)// контрольная сумма
{
	unsigned char crc = 0xFF;
	unsigned char i;

	while (len--)
	{
		crc ^= *pcBlock++;

		for (i = 0; i < 8; i++)
			crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
	}

	return crc;
}

//------------------------------------------------------------------------------

void turn_buffer1(unsigned char j)// отправка буффера данных
{
	num1 = j; // устанавливаем количество байт для передачи
	IEC0bits.U1RXIE = 0;
	switch (statusTR.time) // проверка на минимальное время между посылками
	{
		case 0: // время меньше минимально допустимого
			statusTR.turn = 1; //есть данные для отправки, данные поставлены в очередь
			statusTR.reply_ok = 0;
			break;
		case 1: // время больше минимально допустимого
			statusTR.turn = 0; // сброс очереди на отправку
			statusTR.tr_on = 1; // передатчик занят
			statusTR.reply_ok = 0;
			RS485_1_TR(); // отправляем данные
			break;
	}
}

//------------------------------------------------------------------------------

void status(void)
{
	uint32_t totalMemory, freeMemory, freeClusters, sectorsize;

	sectorsize = FSectorSize(); //Делёный на 256

	totalMemory = (sectorsize * FTotalClusters()) >> 2; //Общий объём флеш в Kb (делёный на 4)

	//Общее деление на 1024

	freeClusters = FgetSetFreeCluster(); //Чтение из переменной обновляемой 1 раз в сек.
	if (freeClusters == 0xffffffff) freeClusters = 0; //Ошибка
	freeMemory = (sectorsize * freeClusters) >> 2; //Общий объём свободной памяти флеш в Kb

	buff_tr[0] = ADDRESS_Master; //0xFF
	buff_tr[1] = ADDRESS_Ustroistva; //0x76
	buff_tr[2] = 0x09; //Количество байт 

	getSDFlags(&flags_flash);
	buff_tr[3] = flags_flash.char_flags; //Stat

	//Общее количество в Kb флеш
	buff_tr[4] = totalMemory >> 24; //Со старшего байта
	buff_tr[5] = totalMemory >> 16;
	buff_tr[6] = totalMemory >> 8;
	buff_tr[7] = totalMemory;

	//Свободное количество в Kb флеш
	buff_tr[8] = freeMemory >> 24; //Со старшего байта
	buff_tr[9] = freeMemory >> 16;
	buff_tr[10] = freeMemory >> 8;
	buff_tr[11] = freeMemory;

	buff_tr[12] = Crc8(buff_tr, 12); // контрольная сумма

	int test = 0;
	if (buff_tr[12] != 0xE9)
		test = 1;

	turn_buffer1(13);
}

//------------------------------------------------------------------------------

void status_err_read(void)//Ошибка при чтении
{
	buff_tr[0] = ADDRESS_Master; //0xFF
	buff_tr[1] = ADDRESS_Ustroistva; //0x76
	buff_tr[2] = 1; //Количество байт
	buff_tr[3] = flags_flash.char_flags; //Stat
	buff_tr[4] = Crc8(buff_tr, 4); // контрольная сумма

	turn_buffer1(5); //Отослать данные
}

//------------------------------------------------------------------------------

void readedMessage(unsigned char *str, unsigned char len, uint32_t time)
{
	uint8_t i = 0;

	buff_tr[0] = ADDRESS_Master; //0xFF
	buff_tr[1] = ADDRESS_Ustroistva; //0x76
	buff_tr[2] = 1 + 4 + len; //Байт статуса, время и количество байт строки
	buff_tr[3] = FDatas.fread ? flags_flash.char_flags : (flags_flash.char_flags & 0x7f); //Stat (для отображения действительного флага записи (flags_flash.start_write))
	buff_tr[4] = time >> 24; //Время (со старшего)
	buff_tr[5] = time >> 16;
	buff_tr[6] = time >> 8;
	buff_tr[7] = time;

	for (i = 0; i <= len; i++) buff_tr[8 + i] = str[i];
	buff_tr[8 + len] = Crc8(buff_tr, (8 + len)); // контрольная сумма

	turn_buffer1(9 + len); //Отослать данные
}

//------------------------------------------------------------------------------

uint16_t CRC16(uint8_t *item, uint8_t lngth)//Для проверки эмуляции датчика уровня/температуры и давления
{
	uint16_t crc = 0xFFFF;
	uint8_t i;

	while (lngth--)
	{
		crc ^= *item++;

		for (i = 8; i > 0; i--)
			if (crc & 1)
			{
				crc >>= 1;
				crc ^= 0xA001;
			} else
			{
				crc >>= 1;
			}
	}

	return crc;
}