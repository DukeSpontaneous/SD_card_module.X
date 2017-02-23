#include "rs-485.h"

StatusRX statusRX;
StatusTR statusTR;

unsigned char buff_rx[sizerxbuff];
unsigned char buff_tr[30];

void RS485_Initialize(void)// Инициализация порта UART
{
	RPOR3bits.RP6R = 3; // TX1
	RPINR18 = 8; // RX1 RP8 uart1
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
	statusRX.adres = MODULE_ADDRESS_SD;
	T4initial();
	T2initial();
}
//------------------------------------------------------------------------------

unsigned char Crc8(unsigned char *pcBlock, unsigned char len)// Контрольная сумма CRC8
{
	unsigned char crc = 0xFF;
	unsigned char i, a = 0;

	while (len--)
	{
		crc ^= pcBlock[a++];

		for (i = 0; i < 8; i++)
			crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
	}

	return crc;
}
//------------------------------------------------------------------------------

void RS485_1_TR(void)// Передача по UART1
{
	if (statusTR.tr_on == 1)
	{
		for (statusTR.k_vo_peredano = 0; statusTR.k_vo_peredano < statusTR.k_vo_na_peredachu;)
		{
			U1TXREG = buff_tr[statusTR.k_vo_peredano++];

			if (U1STAbits.UTXBF == 1)
			{
				break;
			}
		}
	}
	
	IEC0bits.U1TXIE = 1;
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void) // Функция прерываний по приёму UART1
{
	TMR2 = 0; // Обнулить таймер

	if (statusRX.timer == 1)// Промежуток между посылками больше минимально допустимого
	{
		if (statusRX.adres_agree == 0)// Адрес устройства не совпал, или не принят
		{
			buff_rx[0] = U1RXREG; // Вычитываем первый байт
			statusRX.k_vo_priniatix++;
			if (buff_rx[0] == 0)// Вычитываем мусор
			{
				statusRX.k_vo_priniatix = 0;
				IFS0bits.U1RXIF = 0;
				return; // Ожидаем заполнения буффера 4-мя байтами
			}
			while (U1STAbits.URXDA == 1)// Приём данных
			{
				buff_rx[statusRX.k_vo_priniatix++] = U1RXREG;
			}
			if (buff_rx[0] == statusRX.adres)// Сравниваем адрес (совпал)
			{
				T4CONbits.TON = 0; // Остановка глобального таймера отсутствия связи
				T2CONbits.TON = 1;
				while (U1STAbits.URXDA == 1)// Вычитываем данные из буфера
				{
					buff_rx[statusRX.k_vo_priniatix++] = U1RXREG;
				}
				statusRX.adres_agree = 1;
				PR2 = timerx_ob; // Отслеживание обрыва посылки
				statusRX.command = buff_rx[1]; // Коммандный байт
				statusRX.k_vo_dolzno_priniatsy = buff_rx[2]*256 + buff_rx[3] + 5; // Количество данных (все данные которые принимаются включая crc и служебные)
				if (statusRX.k_vo_dolzno_priniatsy == 4)// Если количество данных 0, все данные приняты 
				{
					goto raschet_crc;
				} else// Есть данные для приёма
				{
					if (statusRX.k_vo_dolzno_priniatsy > sizerxbuff)// Неверное количество байт, ошибочная передача
					{
						statusRX.status = 0;
						statusRX.k_vo_priniatix = 0;
						statusRX.eror = 1;
						PR2 = timerx;
					}
					if (statusRX.k_vo_dolzno_priniatsy < statusRX.k_vo_priniatix + 4)// Количество, которые должны приняться, меньше 4-х
					{
						U1STAbits.URXISEL1 = 0; // Побайтовый режим приёма
					}
				}
			} else// Не совпал
			{
				statusRX.k_vo_priniatix = 0;
				statusRX.timer = 0; // Сброс сработки таймера (посылка данных другому устройству)
			}
		} else// Приём данных
		{
			while (U1STAbits.URXDA == 1)// Приём данных
			{
				buff_rx[statusRX.k_vo_priniatix++] = U1RXREG;
			}
			if (statusRX.k_vo_priniatix < statusRX.k_vo_dolzno_priniatsy)
			{
				if (statusRX.k_vo_dolzno_priniatsy < statusRX.k_vo_priniatix + 4)
				{
					U1STAbits.URXISEL1 = 0; // Побайтовый режим приёма
				}
			} else// Приём завершен
			{
raschet_crc:
				U1STAbits.URXISEL1 = 1; // Прерывание по 4 байта
				statusRX.crc = Crc8(buff_rx, statusRX.k_vo_priniatix - 1); // Вычисляем CRC
				if (statusRX.crc == buff_rx[statusRX.k_vo_priniatix - 1]) // Совпал
				{
					PR2 = timetr; // Ожидание разрешения передачи
					statusRX.status = 0; // Приём завершился удачно
					statusRX.k_vo_priniatix = 0;
					
					SLAVE_SD_MasterQueryProcessing(buff_rx, buff_tr);
					send_answer_status();				
					
				} else// CRC не совпал
				{
					statusRX.status = 0;
					statusRX.k_vo_priniatix = 0;
					statusRX.eror = 1;
					PR2 = timerx;
				}
			}
		}
	} else
	{
		U1STAbits.OERR = 0; // Сброс ошибок буфера приёма
		while (U1STAbits.URXDA == 1) // Приём данных
		{
			buff_rx[0] = U1RXREG;
		}
	}
	IFS0bits.U1RXIF = 0;
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _U1TXInterrupt(void) // Функция прерываний по завершению передачи UART1
{
	if (statusTR.k_vo_peredano < statusTR.k_vo_na_peredachu) // все данные загружены в FIFO ?
	{
		while (statusTR.k_vo_peredano < statusTR.k_vo_na_peredachu) // добавляем в буфер FIFO
		{
			if (U1STAbits.UTXBF == 1)// буфер полон ?
			{
				break;
			}
			U1TXREG = buff_tr[statusTR.k_vo_peredano++]; // добавляем данные в буфер
			if (statusTR.k_vo_peredano == statusTR.k_vo_na_peredachu) // все данные загружены в FIFO, выходим ожидаем передачу последнего байта
			{
				U1STAbits.UTXISEL1 = 0; // переключаем прерывание на опустошение сдвигового регистра
				U1STAbits.UTXISEL0 = 1;
			}
		}
	} else // передача завершена
	{
		IEC0bits.U1TXIE = 0; // отключаем прерывание по передаче
		U1STAbits.UTXISEL1 = 1; // переключаем прерывание на опустошение FIFO
		U1STAbits.UTXISEL0 = 0;
		statusTR.status = 0;
		PR2 = time_free_line; // время ожидания отпускания линии
		T2CONbits.TON = 1; // включаем таймер для переключения в режим приема
		U1STAbits.OERR = 0; // сброс ошибок буфера приема
		while (U1STAbits.URXDA == 1)
		{
			statusRX.temp_buffer = U1RXREG;
		}
		statusRX.k_vo_priniatix = 0;
		statusRX.k_vo_dolzno_priniatsy = 0;

		IFS0bits.U1RXIF = 0; // включаем прерывание приема
		IEC0bits.U1RXIE = 1;
	}
	IFS0bits.U1TXIF = 0; // сброс прерываний по передаче
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _T2Interrupt(void) // RS-485
{
	switch (PR2)
	{
		case timetr:
			if (statusTR.turn == 1) // передача данных разрешена, тацминг для передачи превышен
			{
				T2CONbits.TON = 0; // выключаем таймер для отправки сообщения
				statusTR.tr_on = 1;
				statusTR.turn = 0; // сброс очереди
				RS485_1_TR(); // передача данных
			}
			break;
		case timerx://промежуток между приемом больше минимального
			statusRX.timer = 1;
			U1STAbits.OERR = 0;
			TMR4 = 0;
			TMR5HLD = 0;
			T4CONbits.TON = 1; // запуск глобального таймера отсутствия связи
			//        T2CONbits.TON = 0;
			break;
		case timerx_ob:// обрыв приема, пришли не все данные
			U1STAbits.OERR = 0;
			while (U1STAbits.URXDA == 1)
			{
				statusRX.temp_buffer = U1RXREG;
			}
			//        U1STAbits.URXISEL = 0;
			statusRX.status = 0;
			statusRX.k_vo_priniatix = 0;
			TMR2 = 0;
			PR2 = timerx;
			statusRX.eror = 1;
			break;
		case time_free_line:
			PR2 = timerx;
			R_T = 0; // переводим max485 в режим приема
			break;
	}
	IFS0bits.T2IF = 0;
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _T5Interrupt(void) // Функция прерываний по T1, отсутствие связи с модулем
{
	T4CONbits.TON = 0;
	statusRX.eror = 1;
	IFS1bits.T5IF = 0;
}
//------------------------------------------------------------------------------

void turn_tr_buffer(uint8_t j) // Отправка буффера данных
{
	statusTR.k_vo_na_peredachu = j; // Устанавливаем количество байт для передачи
	IEC0bits.U1RXIE = 0;
	switch (statusTR.time) // Проверка на минимальное время между посылками
	{
		case 0: // Время меньше минимально допустимого
			statusTR.turn = 1; // Есть данные для отправки, данные поставлены в очередь
			break;
		case 1: // Время больше минимально допустимого
			statusTR.turn = 0; // Сброс очереди на отправку
			statusTR.tr_on = 1; // Передатчик занят
			RS485_1_TR(); // Отправляем данные
			break;
	}
}
//------------------------------------------------------------------------------

void send_answer_status()
{
	struct_flags_flash flags_flash;
	getSDFlags(&flags_flash);

	uint32_t totalMemory, freeMemory, freeClusters, sectorsize;

	sectorsize = FSectorSize(); // Делёный на 256

	totalMemory = (sectorsize * FTotalClusters()) >> 2; // Общий объём флеш в Kb (делёный на 4)

	// Общее деление на 1024

	// TODO: не очень понятно, зачем об этом знать мастер-модулю,
	// в принципе этот параметр нужно будет убрать,
	// а пока, видимо, нужно поставить заглушку.
	freeClusters = FgetSetFreeCluster(); // Чтение из переменной обновляемой 1 раз в сек.
	if (freeClusters == 0xffffffff)
		freeClusters = 0; // Ошибка
	freeMemory = (sectorsize * freeClusters) >> 2; // Общий объём свободной памяти флеш в Kb

	buff_tr[0] = MODULE_ADDRESS_MASTER; // 0xFF
	buff_tr[1] = MODULE_ADDRESS_SD; // 0x76
	buff_tr[2] = 0x09; // Количество байт 
	buff_tr[3] = 0x0C;//flags_flash.char_flags; // Ожидается 0x58

	// Общее количество в Kb флеш
	buff_tr[4] = totalMemory >> 24; // Со старшего байта
	buff_tr[5] = totalMemory >> 16;
	buff_tr[6] = totalMemory >> 8;
	buff_tr[7] = totalMemory;

	// Свободное количество в Kb флеш
	buff_tr[8] = freeMemory >> 24; // Со старшего байта
	buff_tr[9] = freeMemory >> 16;
	buff_tr[10] = freeMemory >> 8;
	buff_tr[11] = freeMemory;

	buff_tr[12] = Crc8(buff_tr, 12); // Контрольная сумма

	turn_tr_buffer(13);
}

//------------------------------------------------------------------------------

void send_answer_status_err_read(void)// Ошибка при чтении
{
	struct_flags_flash flags_flash;
	getSDFlags(&flags_flash);
	buff_tr[0] = MODULE_ADDRESS_MASTER; //0xFF
	buff_tr[1] = MODULE_ADDRESS_SD; //0x76
	buff_tr[2] = 1; //Количество байт
	buff_tr[3] = flags_flash.char_flags; //Stat
	buff_tr[4] = Crc8(buff_tr, 4); // контрольная сумма

	turn_tr_buffer(5); //Отослать данные
}

//------------------------------------------------------------------------------


