#include <string.h>

#include "SD_protocol.h"

#include "pins.h"
#include "SPI_protocol.h"
#include "MathHelper.h"
#include "myFAT32.h"

SD_CONFIG SDConfig = {CRC_ENABLE, SD_CARD_DEFAULT_TYPE};

// ******************************************************************
//Функция: ожидание токена стартового блока 0xFE по интерфейсу SPI.
//Аргументы: void
//return: bool (флаг успеха ожидания маркера начала данных)
// ******************************************************************

static bool SD_waitDataToken()
{
	SD_CS_ASSERT;
	uint16_t waiting = UINT16_MAX;
	while (SPI1_rw(0xFF) != SD_START_DATA_TOKEN) // Ожидаем токен стартового блока 0xFE (маркер начала данных 0b11111110)
		if (waiting-- == 0) // Если первышено максимальное число попыток переотправки
		{
			SD_CS_DEASSERT;
			return false;
		} // Аварийный выход по превышению лимита повторных попыток

	SD_CS_DEASSERT;
	return true;
}
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: отправить CMD команду SD карте
//Аргументы: SD_CMD (8-битная команда)
// & unsigned long (32-битный аргумент команды)
// & unsigned char * (16-битный указатель на массив для приёма кода завершения)
//return: bool (флаг успеха; возвращает false, если response[0] == 0xFF)
// ******************************************************************

static bool SD_sendCMD(SD_CMD cmd, uint32_t arg, uint8_t response[])
{
	// Цитата: "Мы помним, что поле CRC в режиме SPI игнорируется для всех команд, кроме CMD0 и CMD8."
	// MY: видимо речь о том, что его МОЖНО игнорировать при желании...
	// http://we.easyelectronics.ru/AVR/mmcsd-i-avr-chast-2-rabota-s-kartoy.html

	uint8_t i;

	uint8_t message[5];

	SD_CS_ASSERT;
	// Стоит заметить, что перед отсылкой команды, мы отправляем байт данных равный 0xFF.
	// Это делается согласно datasheet’а,
	// т.к. минимальная задержка между последним байтом ответа от предыдущей команды
	// и первым байтом следующей должно пройти не менее 8 клоков по SCLK.	
	SPI1_rw(0xFF);

	// Формат команды:
	//	Номер_команды	Аргумент_команды		CRC7
	// {{0x40 + CMDx}, {0x??, 0x??, 0x??, 0x??}, {0x??}}
	SPI1_rw(message[0] = cmd); // Отправляем команду, старшие два бита всегда равны '01'
	SPI1_rw(message[1] = arg >> 24);
	SPI1_rw(message[2] = arg >> 16);
	SPI1_rw(message[3] = arg >> 8);
	SPI1_rw(message[4] = arg);

	// Если активирован режим проверки контрольной суммы,
	// то отправить CRC7,
	// иначе отправить СRC7 для CMD0 (0x95),
	// который постоянен и нужен ей в любом случае.	
	if (SDConfig.SD_crcEnabled || cmd == SD_CMD_GO_IDLE_STATE || cmd == SD_CMD_SEND_IF_COND)
		SPI1_rw(MATH_CRC7_SD(message, 5) | 1); // [7:1] CRC7 + [0] end bit 1	

	// TODO: сейчас в любом случае ожидает 5 байт ответа,
	// но вообще этот момент нужно потом перепроверить
	response[0] = 0xFF;
	for (i = 0; i < 255 && response[0] == 0xFF; i++)
		response[0] = SPI1_rw(0xFF); // Ожидаем ответ

	if (response[0] == 0xFF)
		return false;

	for (i = 1; i < 5; i++)
		response[i] = SPI1_rw(0xFF);

	SPI1_rw(0xFF); //extra 8 CLK
	SD_CS_DEASSERT;

	return true; // Команда отправлена успешно
}
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: отправить ACMD команду (application specific command) SD карте
//Аргументы: SD_ACMD (8-битная ACMD-команда)
// & unsigned long (32-битный аргумент команды)
// & unsigned char * (16-битный указатель на массив для приёма кода завершения)
//return: bool (флаг успеха; возвращает false, если response[0] == 0xFF)
// ******************************************************************

static bool SD_sendACMD(SD_ACMD acmd, uint32_t arg, uint8_t response[])
{
	// CMD55 (APP_CMD): defines to the card that the next command is
	// an application specific command rather than a standard command.
	// Response: R1 	

	if (!SD_sendCMD(SD_CMD_APP_CMD, NULL, response))
		return false;

	SD_CS_ASSERT;
	SPI1_rw(0xFF); // В старой программе не было	
	SD_CS_DEASSERT;

	// TODO: что-то не очень понятно, какой должен быть ответ

	if (!SD_sendCMD(acmd, arg, response))
		return false;

	SD_CS_ASSERT;
	SPI1_rw(0xFF); // В старой программе не было
	SD_CS_DEASSERT;

	return true; // Команда отправлена успешно
}
//------------------------------------------------------------------------------

// ******************************************************************
//Функция: инициализация режима SPI в SD карте, подключенной по SPI,
//которая, помимо прочего, определяет её тип.
//Аргументы: void
//return: bool (флаг успеха инициализации SD карты в режиме SPI)
// ******************************************************************

SD_TYPE SD_initialiseModeSPI()
{
	uint8_t i;

	uint8_t response[5];

	SD_CS_DEASSERT;

	// SPI настроен как полагается (я думаю все умеют это делать) и вывод _CS карты памяти выставлен в логическую “1”.
	// После этого нам необходимо подать минимум 74 тактовых импульса на линию SCLK SPI.
	// Выполнив все это мы выставляем логический “0” на вывод _CS карты памяти и отсылаем команду CMD0.

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

		SD_sendCMD(SD_CMD_GO_IDLE_STATE, 0, response); // Команда 'reset & перейти в IDLE'

		// Немного отступлю от мысли и обращу внимание на то,
		// что все ответы содержат в себе первым байтом R1,
		// 7-й бит которого всегда является 0.
		//
		// R1[0, 7] Response Format:
		// R1[0] b00000001: in idle state
		// R1[1] b00000010: erase reset
		// R1[2] b00000100: illegal command
		// R1[3] b00001000: com crc error
		// R1[4] b00010000: erase sequence error
		// R1[5] b00100000: address error
		// R1[6] b01000000: parameter error
		// R1[7] b*0000000: обязательный 0 для R1
	} while (response[0] != 0x01); // Если карта ещё не в IDLE state

	// TODO: выяснить, зачем так было в старой программе (было деактивировано)
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
	// [15:8] ‘check pattern’ is set to any 8-bit pattern	
	// It is recommended to use ‘10101010b’ for the ‘check pattern’.
	//		0x000000AA

	// Посылаем команду CMD8, которая указывает карте поддерживаемые МК напряжения питания для неё
	// и спрашивает у выбранной карты, может ли она работать в данном диапазоне напряжений,
	// дожидаемся ответа R7.
	// SEND_IF_COND

	// Карты памяти стандарта MMC и SD версии 1.х эту команду не поддерживают и,
	// соответственно, в своем ответе будут содержать бит «illegal command» (2).
	// Если сказанное ранее верно, то установленная карта либо MMC, либо SD версии 1.х.

	// The card checks whether it can operate on the host’s supply voltage. The card that accepted the
	// supplied voltage returns R7 response. In the response, the card echoes back both the voltage range
	// and check pattern set in the argument. If the card does not support the host supply voltage, it shall not
	// return response and stays in Idle state.

	if (!SD_sendCMD(SD_CMD_SEND_IF_COND, 0x000001AA, response))
		return SD_TYPE_UNKNOWN;

	SD_CS_ASSERT;
	SPI1_rw(0xFF);
	SD_CS_DEASSERT;

	// R1[2] b00000100: illegal command
	if (response[0] & 0b00000100) // MY: cиняя карточка зашла сюда с 0x05
	{
		// Карты памяти стандарта MMC, или SD версии 1.х

		// Теперь пришло время распознать, какая именно из этих двух типов карт вставлена в картоприемник.
		// Для этого отправим карте памяти команду ACMD41, которая инициирует процесс инициализации карты.
		// Эта команда посылается в цикле либо для ее выполнения взводится таймер,
		// по которому проверяется ответ на эту команду.

		// ACMD41: SD_SEND_OP_COND; R1		
		if (!SD_sendACMD(SD_ACMD_SD_SEND_OP_COND, 0, response))
			return SD_TYPE_UNKNOWN;

		// R1[2] b00000100: illegal command
		if (response[0] & 0b00000100)
		{
			// В любом случае, карта MMC не поддерживает ACMD41 и вернет «illegal command» в своем ответе.
			// В таком случае вставленная карта есть MMC и для ее инициализации потребуется команда CMD1
			// (так же посылается в цикле, пока ответ на нее не будет равен 0).
			// Получив ответ на CMD1 равный 0х00 карта MMC готова к работе.

			response[0] = 0xFF;

			// ACMD1: SEND_OP_COND; R1
			while (response[0] != 0x00) // MY: MMC зашла с 0x01	
				if (!SD_sendCMD(SD_CMD_SEND_OP_COND, 0, response))
					return SD_TYPE_UNKNOWN;

			return SD_TYPE_MMC;

		} else if (response[0] == 0x01) // MY: cиняя карточка зашла сюда с 0x01 o_0 ...
		{
			// Если ответ на ACMD41 не содержит никаких установленных битов (т.е. равен 0х00),
			// то карта SD версии 1.х и она готова к работе.
			return SD_TYPE_SD1;
		} else
		{
			return SD_TYPE_UNKNOWN;
		}
	} else if (response[0] == 0x01) // если ответ R7 на CMD8: SEND_IF_COND начинается с...
	{
		// Карты памяти стандарта SDC версии 2+

		// ACMD41: SD_SEND_OP_COND; R1
		// У нас карта памяти формата SD версии 2.х стандартной емкости(SDSC версии 2.х) или SDHC.
		// Следующим шагом в таком случае есть отправка команды ACMD41 с параметром,
		// указывающим карте памяти, поддерживает ли наше устройство карты памяти SDHC.
		// Вне зависимости от того, есть поддержка SDHC или ее нет,
		// мы циклически отправляем эту команду карте то тех пор, пока она (карта) не закончит процесс инициализации.

		response[0] = 0xFF;
		while (response[0] != 0x00)
			if (!SD_sendACMD(SD_ACMD_SD_SEND_OP_COND, 0x40000000, response)) // Чёрная карта входит сюда с возвращением 0x01
				return SD_TYPE_UNKNOWN;

		// Когда ответ от ACMD41 будет равен 0х00, карта памяти проинициализирована и готова к работе.
		// Но для того, чтобы узнать, какая у на карта, мы отправим ей команду CMD58.

		// ACMD58: READ_OCR; R3
		if (!SD_sendCMD(SD_CMD_READ_OCR, 0, response))
			return SD_TYPE_UNKNOWN;

		SD_CS_ASSERT;
		SPI1_rw(0xFF);
		SD_CS_DEASSERT;

		// Проанализировав OCR на установку бита CSS можно определить тип карты:
		if (response[1] & 0x40) // CCS == 1 – карта SDHC или SDXC,
			return SD_TYPE_SD2HC;
		else // CCS == 0 – карта SDSC.
			return SD_TYPE_SD2;
	} else
	{
		return SD_TYPE_UNKNOWN;
	}
}
//------------------------------------------------------------------------------

bool SD_setConfig(uint16_t blocklen, bool crcEnabled)
{
	// Если карта инициализирована успешно, согласно своему типу:	
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
//Функция: считать 512-блок данных с SD карты
//Аргументы: unsigned char * (16-битный указатель на 512-байтный буфер-приёмник)
// & unsigned long (32-битный номер целевого физического блока на SD карте)
//return: bool (флаг успеха операции чтения 512-блока данных)
// ******************************************************************

bool SD_readSingle512Block(uint8_t destination[], uint32_t addrOfPhisicalSector)
{			
	uint16_t i;

	uint8_t response[5];	
	
	// CMD17 (READ_SINGLE_BLOCK): Reads a block of the size selected by the SET_BLOCKLEN command
	// Argument: [31:0] data address
	// The unit of “data address” in argument is byte
	// for Standard Capacity SD Memory Card and block (512 bytes)
	// for High Capacity SD Memory Card.
	// Response: R1
	
	// MY: sdStatus.sdType == SD2HC ? startBlock : startBlock << 9
	// если это не High Capacity карта, то номер блока умножается на 512 (выбор целевого байта, соответствующего началу искомого 512-блока).
	if (!SD_sendCMD(SD_CMD_READ_SINGLE_BLOCK, SDConfig.SD_type == SD_TYPE_SD2HC ? addrOfPhisicalSector : addrOfPhisicalSector << 9, response))
		return false;	
	if (response[0] != 0x00) // Проверяем SD стуатус: 0x00 - OK
		return false;	
	
	if (!SD_waitDataToken())
		return false;	
	
	SD_CS_ASSERT;
	for (i = 0; i < 512; i++) // Читаем 512 байт
		destination[i] = SPI1_rw(0xFF);		
	
	// Приём входящего CRC16 принятого 512-блока данных (16 бит)
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
//Функция: записать 512-блок данных с SD карты
//Аргументы: unsigned char * (16-битный указатель на 512-байтный буфер-источник)
// & unsigned long (32-битный номер целевого физического блока на SD карте)
//return: bool (флаг успеха операции записи 512-блока данных)
// ******************************************************************

bool SD_writeSingle512Block(uint8_t source[], uint32_t addrOfPhisicalSector)
{	
	uint16_t i;

	uint8_t response[5];
	
	// CMD24 (WRITE_BLOCK): Writes a block of the size selected by the SET_BLOCKLEN command.
	// Response: R1
	if (!SD_sendCMD(SD_CMD_WRITE_BLOCK, SDConfig.SD_type == SD_TYPE_SD2HC ? addrOfPhisicalSector : addrOfPhisicalSector << 9, response)) //write a Block command
		return false;	

	if (response[0] != 0x00) // Проверяем SD статус: 0x00 - OK
		return false;

	SD_CS_ASSERT;
	SPI1_rw(0xFE); // Отправляем токен стартового блока 0xFE (0b11111110)

	for (i = 0; i < BYTE_PER_SECTOR; i++) // Отправляет 512 байтов данных
		SPI1_rw(source[i]);
	
	uint16_t crc16 = 0xFFFF;
	if (SDConfig.SD_crcEnabled)
		crc16 = MATH_CRC16_SD(source, BYTE_PER_SECTOR);

	SPI1_rw(crc16 >> 8); // Отправить dummy CRC (16 бит)
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
		if (waiting-- == 0x00000000) // Если первышено максимальное число попыток возвращения в штатное состояние
		{
			// TODO: по-моему именно здесь он дольше всего задерживается после записи,
			// возможно стоит изучить эту ситуацию подробнее.
			// Одной SD2HC (4 ГБ) оказалось мало 0xFFFE o_0
			
			SD_CS_DEASSERT;
			return false;
		}	

	SD_CS_DEASSERT;
	
	return true;
}
//------------------------------------------------------------------------------