#include "RS-485_protocol.h"
#include "pins.h"

StatusTR statusTR;
StatusRX statusRX;

uint8_t buff_tr[sizetrbuff], buff_rx[sizerxbuff];

void RS485_initial(void)// ������������� ����� uart
{
	RPOR3bits.RP6R = 3; // TX1
	RPINR18 = 8; // RX1 RP8 UART1
	U1BRG = BRGVAL; // ��������� ��������
	R_T = 0;
	PR2 = timerx;
	U1MODE = 0b1010000000000000;
	U1STA = 0b1000010011000000;

	IPC2bits.U1RXIP = 5; // ��������� ���������� �� ����� � ��������
	IPC3bits.U1TXIP = 5;

	IFS0bits.U1RXIF = 0; // ��������� �������� ����������
	IFS0bits.U1TXIF = 0;
	IEC0bits.U1RXIE = 1; // ��������� ����������
	statusRX.address = ADDRESS_MODULE_SD;
	T2initial();
}
//------------------------------------------------------------------------------

void RS485_1_TR(void)// �������� �� uart1
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

void turn_buffer(uint8_t j)// �������� ������� ������
{
	statusTR.waitForSendCount = j; // ������������� ���������� ���� ��� ��������
	IEC0bits.U1RXIE = 0;
	switch (statusTR.time) // �������� �� ����������� ����� ����� ���������
	{
		case 0: // ����� ������ ���������� �����������
			statusTR.turn = 1; //���� ������ ��� ��������, ������ ���������� � �������
			break;
		case 1: // ����� ������ ���������� �����������
			statusTR.turn = 0; // ����� ������� �� ��������
			statusTR.tr_on = 1; // ���������� �����
			RS485_1_TR(); // ���������� ������
			break;
	}
}
//------------------------------------------------------------------------------