//#include <libpic30.h> // библиотека задержек
#include <xc.h>

#include "pins.h"
#include "MathHelper.h"
#include "RS-485_protocol.h"

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void) // функция прерываний по приему UART1
{
	TMR2 = 0; // обнулить таймер

	if (statusRX.timer == 1)//промежуток между посылками больше минимально допустимого
	{
		if (statusRX.address_agree == 0)//адрес устройства не совпал или не принят
		{
			buff_rx[0] = U1RXREG; // вычитываем первый байт
			if (buff_rx[0] == 0)// вычитываем мусор
			{
				IFS0bits.U1RXIF = 0;
				return; // ожидаем заполнения буффера 4 байтами
			}
			if (buff_rx[0] == statusRX.address)//сравниваем адрес(совпал)
			{
				T4CONbits.TON = 0; // остановка глобального таймера отсутствия связи
				T2CONbits.TON = 1;
				statusRX.reseivedCount++;
				while (U1STAbits.URXDA == 1)// вычитываем данные из буфера
				{
					buff_rx[statusRX.reseivedCount++] = U1RXREG;
				}
				statusRX.address_agree = 1;
				PR2 = timerx_ob; // отслеживание обрыва посылки
				statusRX.command = buff_rx[1]; //коммандный байт
				statusRX.waitForReseiveCount = buff_rx[2] + 4; //количество данных (все данные которые принимаются включая crc и служебные)
				if (statusRX.waitForReseiveCount == 4)// если количество данных 0, все данные приняты 
				{
					goto raschet_crc;
				} else//есть данные для приема
				{
					if (statusRX.waitForReseiveCount > sizerxbuff)// неверное количество байт, ошибочная передача
					{
						statusRX.status = 0;
						statusRX.reseivedCount = 0;
						statusRX.error = 1;
						PR2 = timerx;
					}
					if (statusRX.waitForReseiveCount < statusRX.reseivedCount + 4)// количество которые должны приняться меньше 4-х
					{
						U1STAbits.URXISEL1 = 0; // побайтовый режим приема
					}
				}
			} else//не совпал
			{
				statusRX.timer = 0; // сброс сработки таймера (посылка данных другому устройству)
			}
		} else//прием данных
		{
			while (U1STAbits.URXDA == 1)//прием данных
			{
				buff_rx[statusRX.reseivedCount++] = U1RXREG;
			}
			if (statusRX.reseivedCount < statusRX.waitForReseiveCount)
			{
				if (statusRX.waitForReseiveCount < statusRX.reseivedCount + 4)
				{
					U1STAbits.URXISEL1 = 0; // побайтовый режим приема
				}
			} else//прием завершен
			{
raschet_crc:
				U1STAbits.URXISEL1 = 1; // прерывание по 4 байта
				statusRX.crc = MATH_CRC8_UART(buff_rx, statusRX.reseivedCount - 1); // вычисляем контрольную сумму
				if (statusRX.crc == buff_rx[statusRX.reseivedCount - 1])// совпала
				{
					PR2 = timetr; // ожидание разрешения передачи
					statusRX.status = 0; //прием завершился удачно
					statusRX.reseivedCount = 0;
					//обработка полученных данных
					switch (statusRX.command)
					{

					}
				} else// crc не совпала
				{
					statusRX.status = 0;
					statusRX.reseivedCount = 0;
					statusRX.error = 1;
					PR2 = timerx;
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
	if (statusTR.sendedCount < statusTR.waitForSendCount) // все данные загружены в FIFO ?
	{
		while (statusTR.sendedCount < statusTR.waitForSendCount) // добавляем в буфер FIFO
		{
			if (U1STAbits.UTXBF == 1)// буфер полон ?
			{
				break;
			}
			U1TXREG = buff_tr[statusTR.sendedCount++]; // добавляем данные в буфер
			if (statusTR.sendedCount == statusTR.waitForSendCount) // все данные загружены в FIFO, выходим ожидаем передачу последнего байта
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
		statusRX.reseivedCount = 0;
		statusRX.waitForReseiveCount = 0;

		IFS0bits.U1RXIF = 0; // включаем прерывание приема
		IEC0bits.U1RXIE = 1;
	}
	IFS0bits.U1TXIF = 0; // сброс прерываний по передаче
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _T2Interrupt(void)// rs-485
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
			T2CONbits.TON = 0;
			break;
		case timerx_ob:// обрыв приема, пришли не все данные
			U1STAbits.OERR = 0;
			while (U1STAbits.URXDA == 1)
			{
				statusRX.temp_buffer = U1RXREG;
			}
			U1STAbits.URXISEL = 0;
			statusRX.status = 0;
			statusRX.reseivedCount = 0;
			TMR2 = 0;
			PR2 = timerx;
			statusRX.error = 1;
			break;
		case time_free_line:
			PR2 = timerx;
			R_T = 0; // переводим max485 в режим приема
			break;
	}
	IFS0bits.T2IF = 0;
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _T1Interrupt(void) // функция прерываний по T1, информационный светодиод
{
	//    static uint8_t counter, counter1, counter10;
	//    if (statusRX.eror == 1)
	//    {
	//        counter++;
	//    }
	//    //dtu2();//Для проверки эмуляции датчика уровня/температуры и давления
	//
	//    if (counter == 4)//250ms*4=1s
	//    {
	//
	//        FDatas.CountGetFreeClasters++; //Счётчик для вычитывания количества свободных кластеров с флешки
	//        counter = 0;
	//        flash_params.sec_from_1980++;
	//        if (!flags_flash.flash_initialized)//flash is not initialized
	//            RED_LED = ~RED_LED;
	//        else
	//            RED_LED = 0;
	//        if (flags_flash.vdd_on_off)//есть внешнее питание
	//        {
	//            BLUE_LED = ~BLUE_LED;
	//            if (!statusRX1.eror)//нет ошибок связи
	//                WHITE_LED = 1; //горит
	//            else//есть ошибка связи
	//                WHITE_LED = ~WHITE_LED; //моргаем раз сек
	//        }
	//        else//питание от ионисторов
	//        {
	//            WHITE_LED = 0; //off
	//            BLUE_LED = 1; //off
	//        }
	//
	//        //on_off  - флешка вставлена в держатель(0)
	//        //wp_on_off - защита от записи снята(0)
	//        //micro_in_off - микрик замкнут(0)
	//        //vdd_on_off - питание внешнее(1)/внутреннее(0)
	//        //flash_initialized - флешка запущена(1)
	//        //flash_init_answer - связь с флешкой присутствует(0)
	//        //delay_before_init - есть пауза до запуска флешки(1)
	//        //start_write - идёт процесс записи(1)
	//        //Сброс данных на флеш каждые 10сек (при отсутсвии ошибок)
	//        if (!flags_flash.on_off && !flags_flash.micro_in_off && flags_flash.vdd_on_off && flags_flash.delay_before_init)
	//        {
	//            counter10++;
	//            if (counter10 == TimeForFlushPeriodSec)//Сброс данных на флеш каждые 60сек
	//            {
	//                counter10 = 0;
	//
	//                FDatas.NeedWriteFlushBufLog = 1; //Требуется сбросить буфер на флеш
	//
	//            }
	//        }
	IFS0bits.T1IF = 0;
	//    }
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _T5Interrupt(void) // функция прерываний по T1, отсутствие связи с модулем
{
	T4CONbits.TON = 0;
	statusRX.error = 1;
	IFS1bits.T5IF = 0;
}
//--------------------------------------------------------------------------
