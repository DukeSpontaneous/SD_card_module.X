#pragma once

#include <stdint.h>

#define FCY                 40000000
#define F (uint16_t)		(40 / 8)	// ���/�������� ��� �������� ������� ����� ��������� T2
#define timetr              200 * F		// ��������� ������� ��  us ��������
#define timerx              6000 * F	// ��������� ������� ��  us ��� ������������ ������� (���������� ����� ��������� + 4 ����� � ����)
#define timerx_ob           1500 * F	// ��� ������������ ����� ������� ������ 4 ����� �������, �������� ������ ��� 4-� ��� ������ �����
#define time_free_line      150 * F		// ����� ��������� ����� ����� �������� � ��������

#define BAUDRATE        57600
#define BRGVAL          ((FCY / BAUDRATE) / 16) - 1

typedef enum
{
	ADDRESS_MODULE_SD = 0x76, // ����� SD-������
	ADDRESS_MODULE_MASTER = 0xFF // ����� ������-������
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
			unsigned turn : 1; // ���� ������ ��� ��������
			unsigned time : 1; // ����� ����� ���������� ��������� �����������
			unsigned tr_on : 1; // �������� ��������
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
			unsigned error : 1; // ������ ������
			unsigned indication : 1; //��� ������ ������ �� ����� ����� ��� ���������
			unsigned timer : 1;
			unsigned address_agree : 1; // ����� ���������� ������
		};
	};
	uint16_t reseivedCount; // ���������� ��������  ����
	uint16_t waitForReseiveCount; // ���������� ���� ������� ����� ���������
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