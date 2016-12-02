//#include <libpic30.h> // ���������� ��������
#include <xc.h>

#include "pins.h"
#include "MathHelper.h"
#include "RS-485_protocol.h"

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void) // ������� ���������� �� ������ UART1
{
	TMR2 = 0; // �������� ������

	if (statusRX.timer == 1)//���������� ����� ��������� ������ ���������� �����������
	{
		if (statusRX.address_agree == 0)//����� ���������� �� ������ ��� �� ������
		{
			buff_rx[0] = U1RXREG; // ���������� ������ ����
			if (buff_rx[0] == 0)// ���������� �����
			{
				IFS0bits.U1RXIF = 0;
				return; // ������� ���������� ������� 4 �������
			}
			if (buff_rx[0] == statusRX.address)//���������� �����(������)
			{
				T4CONbits.TON = 0; // ��������� ����������� ������� ���������� �����
				T2CONbits.TON = 1;
				statusRX.reseivedCount++;
				while (U1STAbits.URXDA == 1)// ���������� ������ �� ������
				{
					buff_rx[statusRX.reseivedCount++] = U1RXREG;
				}
				statusRX.address_agree = 1;
				PR2 = timerx_ob; // ������������ ������ �������
				statusRX.command = buff_rx[1]; //���������� ����
				statusRX.waitForReseiveCount = buff_rx[2] + 4; //���������� ������ (��� ������ ������� ����������� ������� crc � ���������)
				if (statusRX.waitForReseiveCount == 4)// ���� ���������� ������ 0, ��� ������ ������� 
				{
					goto raschet_crc;
				} else//���� ������ ��� ������
				{
					if (statusRX.waitForReseiveCount > sizerxbuff)// �������� ���������� ����, ��������� ��������
					{
						statusRX.status = 0;
						statusRX.reseivedCount = 0;
						statusRX.error = 1;
						PR2 = timerx;
					}
					if (statusRX.waitForReseiveCount < statusRX.reseivedCount + 4)// ���������� ������� ������ ��������� ������ 4-�
					{
						U1STAbits.URXISEL1 = 0; // ���������� ����� ������
					}
				}
			} else//�� ������
			{
				statusRX.timer = 0; // ����� �������� ������� (������� ������ ������� ����������)
			}
		} else//����� ������
		{
			while (U1STAbits.URXDA == 1)//����� ������
			{
				buff_rx[statusRX.reseivedCount++] = U1RXREG;
			}
			if (statusRX.reseivedCount < statusRX.waitForReseiveCount)
			{
				if (statusRX.waitForReseiveCount < statusRX.reseivedCount + 4)
				{
					U1STAbits.URXISEL1 = 0; // ���������� ����� ������
				}
			} else//����� ��������
			{
raschet_crc:
				U1STAbits.URXISEL1 = 1; // ���������� �� 4 �����
				statusRX.crc = MATH_CRC8_UART(buff_rx, statusRX.reseivedCount - 1); // ��������� ����������� �����
				if (statusRX.crc == buff_rx[statusRX.reseivedCount - 1])// �������
				{
					PR2 = timetr; // �������� ���������� ��������
					statusRX.status = 0; //����� ���������� ������
					statusRX.reseivedCount = 0;
					//��������� ���������� ������
					switch (statusRX.command)
					{

					}
				} else// crc �� �������
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
		U1STAbits.OERR = 0; // ����� ������ ������ ������
		while (U1STAbits.URXDA == 1)//����� ������
		{
			buff_rx[0] = U1RXREG;
		}
	}
	IFS0bits.U1RXIF = 0;
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _U1TXInterrupt(void) // ������� ���������� �� ���������� �������� UART1
{
	if (statusTR.sendedCount < statusTR.waitForSendCount) // ��� ������ ��������� � FIFO ?
	{
		while (statusTR.sendedCount < statusTR.waitForSendCount) // ��������� � ����� FIFO
		{
			if (U1STAbits.UTXBF == 1)// ����� ����� ?
			{
				break;
			}
			U1TXREG = buff_tr[statusTR.sendedCount++]; // ��������� ������ � �����
			if (statusTR.sendedCount == statusTR.waitForSendCount) // ��� ������ ��������� � FIFO, ������� ������� �������� ���������� �����
			{
				U1STAbits.UTXISEL1 = 0; // ����������� ���������� �� ����������� ���������� ��������
				U1STAbits.UTXISEL0 = 1;
			}
		}
	} else // �������� ���������
	{
		IEC0bits.U1TXIE = 0; // ��������� ���������� �� ��������
		U1STAbits.UTXISEL1 = 1; // ����������� ���������� �� ����������� FIFO
		U1STAbits.UTXISEL0 = 0;
		statusTR.status = 0;
		PR2 = time_free_line; // ����� �������� ���������� �����
		T2CONbits.TON = 1; // �������� ������ ��� ������������ � ����� ������
		U1STAbits.OERR = 0; // ����� ������ ������ ������
		while (U1STAbits.URXDA == 1)
		{
			statusRX.temp_buffer = U1RXREG;
		}
		statusRX.reseivedCount = 0;
		statusRX.waitForReseiveCount = 0;

		IFS0bits.U1RXIF = 0; // �������� ���������� ������
		IEC0bits.U1RXIE = 1;
	}
	IFS0bits.U1TXIF = 0; // ����� ���������� �� ��������
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _T2Interrupt(void)// rs-485
{
	switch (PR2)
	{
		case timetr:
			if (statusTR.turn == 1) // �������� ������ ���������, ������� ��� �������� ��������
			{
				T2CONbits.TON = 0; // ��������� ������ ��� �������� ���������
				statusTR.tr_on = 1;
				statusTR.turn = 0; // ����� �������
				RS485_1_TR(); // �������� ������
			}
			break;
		case timerx://���������� ����� ������� ������ ������������
			statusRX.timer = 1;
			U1STAbits.OERR = 0;
			TMR4 = 0;
			TMR5HLD = 0;
			T4CONbits.TON = 1; // ������ ����������� ������� ���������� �����
			T2CONbits.TON = 0;
			break;
		case timerx_ob:// ����� ������, ������ �� ��� ������
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
			R_T = 0; // ��������� max485 � ����� ������
			break;
	}
	IFS0bits.T2IF = 0;
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _T1Interrupt(void) // ������� ���������� �� T1, �������������� ���������
{
	//    static uint8_t counter, counter1, counter10;
	//    if (statusRX.eror == 1)
	//    {
	//        counter++;
	//    }
	//    //dtu2();//��� �������� �������� ������� ������/����������� � ��������
	//
	//    if (counter == 4)//250ms*4=1s
	//    {
	//
	//        FDatas.CountGetFreeClasters++; //������� ��� ����������� ���������� ��������� ��������� � ������
	//        counter = 0;
	//        flash_params.sec_from_1980++;
	//        if (!flags_flash.flash_initialized)//flash is not initialized
	//            RED_LED = ~RED_LED;
	//        else
	//            RED_LED = 0;
	//        if (flags_flash.vdd_on_off)//���� ������� �������
	//        {
	//            BLUE_LED = ~BLUE_LED;
	//            if (!statusRX1.eror)//��� ������ �����
	//                WHITE_LED = 1; //�����
	//            else//���� ������ �����
	//                WHITE_LED = ~WHITE_LED; //������� ��� ���
	//        }
	//        else//������� �� ����������
	//        {
	//            WHITE_LED = 0; //off
	//            BLUE_LED = 1; //off
	//        }
	//
	//        //on_off  - ������ ��������� � ���������(0)
	//        //wp_on_off - ������ �� ������ �����(0)
	//        //micro_in_off - ������ �������(0)
	//        //vdd_on_off - ������� �������(1)/����������(0)
	//        //flash_initialized - ������ ��������(1)
	//        //flash_init_answer - ����� � ������� ������������(0)
	//        //delay_before_init - ���� ����� �� ������� ������(1)
	//        //start_write - ��� ������� ������(1)
	//        //����� ������ �� ���� ������ 10��� (��� ��������� ������)
	//        if (!flags_flash.on_off && !flags_flash.micro_in_off && flags_flash.vdd_on_off && flags_flash.delay_before_init)
	//        {
	//            counter10++;
	//            if (counter10 == TimeForFlushPeriodSec)//����� ������ �� ���� ������ 60���
	//            {
	//                counter10 = 0;
	//
	//                FDatas.NeedWriteFlushBufLog = 1; //��������� �������� ����� �� ����
	//
	//            }
	//        }
	IFS0bits.T1IF = 0;
	//    }
}
//------------------------------------------------------------------------------

void __attribute__((interrupt, auto_psv)) _T5Interrupt(void) // ������� ���������� �� T1, ���������� ����� � �������
{
	T4CONbits.TON = 0;
	statusRX.error = 1;
	IFS1bits.T5IF = 0;
}
//--------------------------------------------------------------------------
