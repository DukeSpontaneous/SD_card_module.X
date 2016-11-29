#include <string.h>

#include "SD_protocol.h"

#include "pins.h"
#include "SPI_protocol.h"
#include "MathHelper.h"
#include "myFAT32.h"

SD_CONFIG SDConfig = {CRC_ENABLE, SD_CARD_DEFAULT_TYPE};

// ******************************************************************
//�������: �������� ������ ���������� ����� 0xFE �� ���������� SPI.
//���������: void
//return: bool (���� ������ �������� ������� ������ ������)
// ******************************************************************

static bool SD_waitDataToken()
{
	SD_CS_ASSERT;
	uint16_t waiting = UINT16_MAX;
	while (SPI1_rw(0xFF) != SD_START_DATA_TOKEN) // ������� ����� ���������� ����� 0xFE (������ ������ ������ 0b11111110)
		if (waiting-- == 0) // ���� ��������� ������������ ����� ������� ������������
		{
			SD_CS_DEASSERT;
			return false;
		} // ��������� ����� �� ���������� ������ ��������� �������

	SD_CS_DEASSERT;
	return true;
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ��������� CMD ������� SD �����
//���������: SD_CMD (8-������ �������)
// & unsigned long (32-������ �������� �������)
// & unsigned char * (16-������ ��������� �� ������ ��� ����� ���� ����������)
//return: bool (���� ������; ���������� false, ���� response[0] == 0xFF)
// ******************************************************************

static bool SD_sendCMD(SD_CMD cmd, uint32_t arg, uint8_t response[])
{
	// ������: "�� ������, ��� ���� CRC � ������ SPI ������������ ��� ���� ������, ����� CMD0 � CMD8."
	// MY: ������ ���� � ���, ��� ��� ����� ������������ ��� �������...
	// http://we.easyelectronics.ru/AVR/mmcsd-i-avr-chast-2-rabota-s-kartoy.html

	uint8_t i;

	uint8_t message[5];

	SD_CS_ASSERT;
	// ����� ��������, ��� ����� �������� �������, �� ���������� ���� ������ ������ 0xFF.
	// ��� �������� �������� datasheet��,
	// �.�. ����������� �������� ����� ��������� ������ ������ �� ���������� �������
	// � ������ ������ ��������� ������ ������ �� ����� 8 ������ �� SCLK.	
	SPI1_rw(0xFF);

	// ������ �������:
	//	�����_�������	��������_�������		CRC7
	// {{0x40 + CMDx}, {0x??, 0x??, 0x??, 0x??}, {0x??}}
	SPI1_rw(message[0] = cmd); // ���������� �������, ������� ��� ���� ������ ����� '01'
	SPI1_rw(message[1] = arg >> 24);
	SPI1_rw(message[2] = arg >> 16);
	SPI1_rw(message[3] = arg >> 8);
	SPI1_rw(message[4] = arg);

	// ���� ����������� ����� �������� ����������� �����,
	// �� ��������� CRC7,
	// ����� ��������� �RC7 ��� CMD0 (0x95),
	// ������� ��������� � ����� �� � ����� ������.	
	if (SDConfig.SD_crcEnabled || cmd == SD_CMD_GO_IDLE_STATE || cmd == SD_CMD_SEND_IF_COND)
		SPI1_rw(MATH_CRC7_SD(message, 5) | 1); // [7:1] CRC7 + [0] end bit 1	

	// TODO: ������ � ����� ������ ������� 5 ���� ������,
	// �� ������ ���� ������ ����� ����� �������������
	response[0] = 0xFF;
	for (i = 0; i < 255 && response[0] == 0xFF; i++)
		response[0] = SPI1_rw(0xFF); // ������� �����

	if (response[0] == 0xFF)
		return false;

	for (i = 1; i < 5; i++)
		response[i] = SPI1_rw(0xFF);

	SPI1_rw(0xFF); //extra 8 CLK
	SD_CS_DEASSERT;

	return true; // ������� ���������� �������
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ��������� ACMD ������� (application specific command) SD �����
//���������: SD_ACMD (8-������ ACMD-�������)
// & unsigned long (32-������ �������� �������)
// & unsigned char * (16-������ ��������� �� ������ ��� ����� ���� ����������)
//return: bool (���� ������; ���������� false, ���� response[0] == 0xFF)
// ******************************************************************

static bool SD_sendACMD(SD_ACMD acmd, uint32_t arg, uint8_t response[])
{
	// CMD55 (APP_CMD): defines to the card that the next command is
	// an application specific command rather than a standard command.
	// Response: R1 	

	if (!SD_sendCMD(SD_CMD_APP_CMD, NULL, response))
		return false;

	SD_CS_ASSERT;
	SPI1_rw(0xFF); // � ������ ��������� �� ����	
	SD_CS_DEASSERT;

	// TODO: ���-�� �� ����� �������, ����� ������ ���� �����

	if (!SD_sendCMD(acmd, arg, response))
		return false;

	SD_CS_ASSERT;
	SPI1_rw(0xFF); // � ������ ��������� �� ����
	SD_CS_DEASSERT;

	return true; // ������� ���������� �������
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: ������������� ������ SPI � SD �����, ������������ �� SPI,
//�������, ������ �������, ���������� � ���.
//���������: void
//return: bool (���� ������ ������������� SD ����� � ������ SPI)
// ******************************************************************

SD_TYPE SD_initialiseModeSPI()
{
	uint8_t i;

	uint8_t response[5];

	SD_CS_DEASSERT;

	// SPI �������� ��� ���������� (� ����� ��� ����� ��� ������) � ����� _CS ����� ������ ��������� � ���������� �1�.
	// ����� ����� ��� ���������� ������ ������� 74 �������� �������� �� ����� SCLK SPI.
	// �������� ��� ��� �� ���������� ���������� �0� �� ����� _CS ����� ������ � �������� ������� CMD0.

	uint8_t retryGoToIDLE = UINT8_MAX;
	// CMD0: SEND_IF_COND; R1
	do
	{
		SD_CS_ASSERT;
		for (i = 0; i > 10; i++)
			SPI1_rw(0xFF);
		SD_CS_DEASSERT;

		if (retryGoToIDLE-- == 0)
			switch (response[0])
			{
				case 0x00:
					return SD_TYPE_DONT_IDLE;
				default:
					return SD_TYPE_UNKNOWN;
			}

		SD_sendCMD(SD_CMD_GO_IDLE_STATE, 0, response); // ������� 'reset & ������� � IDLE'

		// ������� �������� �� ����� � ������ �������� �� ��,
		// ��� ��� ������ �������� � ���� ������ ������ R1,
		// 7-� ��� �������� ������ �������� 0.
		//
		// R1[0, 7] Response Format:
		// R1[0] b00000001: in idle state
		// R1[1] b00000010: erase reset
		// R1[2] b00000100: illegal command
		// R1[3] b00001000: com crc error
		// R1[4] b00010000: erase sequence error
		// R1[5] b00100000: address error
		// R1[6] b01000000: parameter error
		// R1[7] b*0000000: ������������ 0 ��� R1
	} while (response[0] != 0x01); // ���� ����� ��� �� � IDLE state

	// TODO: ��������, ����� ��� ���� � ������ ��������� (���� ��������������)
	SD_CS_ASSERT;
	for (i = 0; i < 2; i++)
		SPI1_rw(0xFF);
	SD_CS_DEASSERT;


	// CMD8: SEND_IF_COND; R7

	// [39:20] reserved bits
	//		0x00000
	// [19:16] voltage supplied (VHS)
	// 0001b ( 2.7-3.6V )
	//		0x000001
	// 0010b ( Reserved for Low Voltage Range )
	//		0x000002
	// [15:8] �check pattern� is set to any 8-bit pattern	
	// It is recommended to use �10101010b� for the �check pattern�.
	//		0x000000AA

	// �������� ������� CMD8, ������� ��������� ����� �������������� �� ���������� ������� ��� ��
	// � ���������� � ��������� �����, ����� �� ��� �������� � ������ ��������� ����������,
	// ���������� ������ R7.
	// SEND_IF_COND

	// ����� ������ ��������� MMC � SD ������ 1.� ��� ������� �� ������������ �,
	// ��������������, � ����� ������ ����� ��������� ��� �illegal command� (2).
	// ���� ��������� ����� �����, �� ������������� ����� ���� MMC, ���� SD ������ 1.�.

	// The card checks whether it can operate on the host�s supply voltage. The card that accepted the
	// supplied voltage returns R7 response. In the response, the card echoes back both the voltage range
	// and check pattern set in the argument. If the card does not support the host supply voltage, it shall not
	// return response and stays in Idle state.

	if (!SD_sendCMD(SD_CMD_SEND_IF_COND, 0x000001AA, response))
		return SD_TYPE_UNKNOWN;

	SD_CS_ASSERT;
	SPI1_rw(0xFF);
	SD_CS_DEASSERT;

	// R1[2] b00000100: illegal command
	if (response[0] & 0b00000100) // MY: c���� �������� ����� ���� � 0x05
	{
		// ����� ������ ��������� MMC, ��� SD ������ 1.�

		// ������ ������ ����� ����������, ����� ������ �� ���� ���� ����� ���� ��������� � �������������.
		// ��� ����� �������� ����� ������ ������� ACMD41, ������� ���������� ������� ������������� �����.
		// ��� ������� ���������� � ����� ���� ��� �� ���������� ��������� ������,
		// �� �������� ����������� ����� �� ��� �������.

		// ACMD41: SD_SEND_OP_COND; R1		
		if (!SD_sendACMD(SD_ACMD_SD_SEND_OP_COND, 0, response))
			return SD_TYPE_UNKNOWN;

		// R1[2] b00000100: illegal command
		if (response[0] & 0b00000100)
		{
			// � ����� ������, ����� MMC �� ������������ ACMD41 � ������ �illegal command� � ����� ������.
			// � ����� ������ ����������� ����� ���� MMC � ��� �� ������������� ����������� ������� CMD1
			// (��� �� ���������� � �����, ���� ����� �� ��� �� ����� ����� 0).
			// ������� ����� �� CMD1 ������ 0�00 ����� MMC ������ � ������.

			response[0] = 0xFF;

			// ACMD1: SEND_OP_COND; R1
			while (response[0] != 0x00) // MY: MMC ����� � 0x01	
				if (!SD_sendCMD(SD_CMD_SEND_OP_COND, 0, response))
					return SD_TYPE_UNKNOWN;

			return SD_TYPE_MMC;

		} else if (response[0] == 0x01) // MY: c���� �������� ����� ���� � 0x01 o_0 ...
		{
			// ���� ����� �� ACMD41 �� �������� ������� ������������� ����� (�.�. ����� 0�00),
			// �� ����� SD ������ 1.� � ��� ������ � ������.
			return SD_TYPE_SD1;
		} else
		{
			return SD_TYPE_UNKNOWN;
		}
	} else if (response[0] == 0x01) // ���� ����� R7 �� CMD8: SEND_IF_COND ���������� �...
	{
		// ����� ������ ��������� SDC ������ 2+

		// ACMD41: SD_SEND_OP_COND; R1
		// � ��� ����� ������ ������� SD ������ 2.� ����������� �������(SDSC ������ 2.�) ��� SDHC.
		// ��������� ����� � ����� ������ ���� �������� ������� ACMD41 � ����������,
		// ����������� ����� ������, ������������ �� ���� ���������� ����� ������ SDHC.
		// ��� ����������� �� ����, ���� ��������� SDHC ��� �� ���,
		// �� ���������� ���������� ��� ������� ����� �� ��� ���, ���� ��� (�����) �� �������� ������� �������������.

		response[0] = 0xFF;
		while (response[0] != 0x00)
			if (!SD_sendACMD(SD_ACMD_SD_SEND_OP_COND, 0x40000000, response)) // ׸���� ����� ������ ���� � ������������ 0x01
				return SD_TYPE_UNKNOWN;

		// ����� ����� �� ACMD41 ����� ����� 0�00, ����� ������ ������������������� � ������ � ������.
		// �� ��� ����, ����� ������, ����� � �� �����, �� �������� �� ������� CMD58.

		// ACMD58: READ_OCR; R3
		if (!SD_sendCMD(SD_CMD_READ_OCR, 0, response))
			return SD_TYPE_UNKNOWN;

		SD_CS_ASSERT;
		SPI1_rw(0xFF);
		SD_CS_DEASSERT;

		// ��������������� OCR �� ��������� ���� CSS ����� ���������� ��� �����:
		if (response[1] & 0x40) // CCS == 1 � ����� SDHC ��� SDXC,
			return SD_TYPE_SD2HC;
		else // CCS == 0 � ����� SDSC.
			return SD_TYPE_SD2;
	} else
	{
		return SD_TYPE_UNKNOWN;
	}
}
//------------------------------------------------------------------------------

bool SD_setConfig(uint16_t blocklen, bool crcEnabled)
{
	// ���� ����� ���������������� �������, �������� ������ ����:	
	uint8_t response[5];

	// CMD16 (set block size to 512)
	SD_sendCMD(SD_CMD_SET_BLOCKLEN, blocklen, response);
	if (response[0] != 0x00)
		return false;

	// CMD59 (deafault - CRC disabled in SPI mode)
	SD_sendCMD(SD_CMD_CRC_ON_OFF, crcEnabled, response);
	if (response[0] != 0x00)
		return false;

	return true;
}

// ******************************************************************
//�������: ������� 512-���� ������ � SD �����
//���������: unsigned char * (16-������ ��������� �� 512-������� �����-�������)
// & unsigned long (32-������ ����� �������� ����������� ����� �� SD �����)
//return: bool (���� ������ �������� ������ 512-����� ������)
// ******************************************************************

bool SD_readSingle512Block(uint8_t destination[], uint32_t addrOfPhisicalSector)
{			
	uint16_t i;

	uint8_t response[5];	
	
	// CMD17 (READ_SINGLE_BLOCK): Reads a block of the size selected by the SET_BLOCKLEN command
	// Argument: [31:0] data address
	// The unit of �data address� in argument is byte
	// for Standard Capacity SD Memory Card and block (512 bytes)
	// for High Capacity SD Memory Card.
	// Response: R1
	
	// MY: sdStatus.sdType == SD2HC ? startBlock : startBlock << 9
	// ���� ��� �� High Capacity �����, �� ����� ����� ���������� �� 512 (����� �������� �����, ���������������� ������ �������� 512-�����).
	if (!SD_sendCMD(SD_CMD_READ_SINGLE_BLOCK, SDConfig.SD_type == SD_TYPE_SD2HC ? addrOfPhisicalSector : addrOfPhisicalSector << 9, response))
		return false;	
	if (response[0] != 0x00) // ��������� SD �������: 0x00 - OK
		return false;	
	
	if (!SD_waitDataToken())
		return false;	
	
	SD_CS_ASSERT;
	for (i = 0; i < 512; i++) // ������ 512 ����
		destination[i] = SPI1_rw(0xFF);		
	
	// ���� ��������� CRC16 ��������� 512-����� ������ (16 ���)
	uint16_t crc16Incoming = (SPI1_rw(0xFF) << 8);
	crc16Incoming += SPI1_rw(0xFF);
	
	SPI1_rw(0xFF); //extra 8 clock pulses
	SD_CS_DEASSERT;
	
	if (SDConfig.SD_crcEnabled)
		if (MATH_CRC16_SD(destination, 512) != crc16Incoming)
			return false;	
		
	return true;
}
//------------------------------------------------------------------------------

// ******************************************************************
//�������: �������� 512-���� ������ � SD �����
//���������: unsigned char * (16-������ ��������� �� 512-������� �����-��������)
// & unsigned long (32-������ ����� �������� ����������� ����� �� SD �����)
//return: bool (���� ������ �������� ������ 512-����� ������)
// ******************************************************************

bool SD_writeSingle512Block(uint8_t source[], uint32_t addrOfPhisicalSector)
{	
	uint16_t i;

	uint8_t response[5];
	
	// CMD24 (WRITE_BLOCK): Writes a block of the size selected by the SET_BLOCKLEN command.
	// Response: R1
	if (!SD_sendCMD(SD_CMD_WRITE_BLOCK, SDConfig.SD_type == SD_TYPE_SD2HC ? addrOfPhisicalSector : addrOfPhisicalSector << 9, response)) //write a Block command
		return false;	

	if (response[0] != 0x00) // ��������� SD ������: 0x00 - OK
		return false;

	SD_CS_ASSERT;
	SPI1_rw(0xFE); // ���������� ����� ���������� ����� 0xFE (0b11111110)

	for (i = 0; i < BYTE_PER_SECTOR; i++) // ���������� 512 ������ ������
		SPI1_rw(source[i]);
	
	uint16_t crc16 = 0xFFFF;
	if (SDConfig.SD_crcEnabled)
		crc16 = MATH_CRC16_SD(source, BYTE_PER_SECTOR);

	SPI1_rw(crc16 >> 8); // ��������� dummy CRC (16 ���)
	SPI1_rw(crc16 & 0xFF);

	response[0] = SPI1_rw(0xFF);

	//response = 0bXXX0AAA1; AAA='0b010' - data accepted
	if ((response[0] & 0x1F) != 0x05)
	{
		// *****010* - data accepted
		// *****101* - data rejected due to CRC error
		// *****110* - data rejected due to write error
		SD_CS_DEASSERT;
		return false;
	}

	SD_CS_DEASSERT;
	SPI1_rw(0xFF); //just spend 8 clock cycle delay before reasserting the CS line
	SD_CS_ASSERT; //re-asserting the CS line to verify if card is still busy
	
	uint32_t waiting = UINT32_MAX;
	while (!SPI1_rw(0xFF)) //wait for SD card to complete writing and get idle
		if (waiting-- == 0x00000000) // ���� ��������� ������������ ����� ������� ����������� � ������� ���������
		{
			// TODO: ��-����� ������ ����� �� ������ ����� ������������� ����� ������,
			// �������� ����� ������� ��� �������� ���������.
			// ����� SD2HC (4 ��) ��������� ���� 0xFFFE o_0
			
			SD_CS_DEASSERT;
			return false;
		}	

	SD_CS_DEASSERT;
	
	return true;
}
//------------------------------------------------------------------------------