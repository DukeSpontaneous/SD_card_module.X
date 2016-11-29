/*
 * File:   MathHelper.c
 * Author: ionash-an
 *
 * Created on 11 ноября 2016 г., 8:20
 */
#include "MathHelper.h"

// MY: какое вообще это имеет отношение к CRC16 CCIT?!
static const uint16_t crc16_sd_table_a[0x10] = // 1 колонка  16x16  CCIT x12+x5+1
{
	0x0000, 0x1231, 0x2462, 0x3653,
	0x48c4, 0x5af5, 0x6ca6, 0x7e97,
	0x9188, 0x83b9, 0xb5ea, 0xa7db,
	0xd94c, 0xcb7d, 0xfd2e, 0xef1f
};
static const uint16_t crc16_sd_table_b[0x10] = // 1 строка 16x16  CCIT x12+x5+1
{
	0x0000, 0x1021, 0x2042, 0x3063,
	0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b,
	0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
};

uint32_t MATH_DivisionCeiling(uint32_t divides, uint32_t divisor)
{
	return divides % divisor ?
			divides / divisor + 1 : divides / divisor;
}

uint16_t MATH_CRC7_SD(uint8_t *buff, uint16_t len)
{
	uint16_t a;
	uint16_t crc = 0x0000; //init CRC

	while (len--)
	{
		crc ^= *buff++;
		a = 0x08;
		do
		{
			crc <<= 1;
			if (crc & 0x100)
				crc ^= 0x12;
			
		} while (--a);
	}
	return (crc & 0xFE);
}
//------------------------------------------------------------------------------

uint16_t MATH_CRC16_SD(uint8_t *buff, uint16_t len)
{	
	uint8_t data;
	uint16_t crc = 0x0000; // init CRC

	while (len--)
	{
		data = *buff++ ^ (crc >> 8);

		uint16_t tmp = crc16_sd_table_a[(data & 0xF0) >> 4];
		tmp ^= crc16_sd_table_b[data & 0x0F];
		crc = tmp ^ (crc << 8);
	}

	return crc;
}
//------------------------------------------------------------------------------

uint8_t MATH_CRC8_UART(const uint8_t *pcBlock, uint8_t len)// контрольная сумма CRC8
{
	uint8_t i, a = 0;
	uint8_t crc = 0xFF;

	while (len--)
	{
		crc ^= pcBlock[a++];

		for (i = 0; i < 8; i++)
			crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
	}

	return crc;
}
//------------------------------------------------------------------------------