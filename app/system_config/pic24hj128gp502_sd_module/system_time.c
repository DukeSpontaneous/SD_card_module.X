#include "system_time.h"

uint32_t globalSecondsTime1980;

void SYSTEM_TIME_Initialize()
{
	globalSecondsTime1980 = 315532800;
}

void SYSTEM_TIME_Update(uint32_t time_1980)
{
	globalSecondsTime1980 = time_1980;
}

void GetTimestamp(FILEIO_TIMESTAMP *timeStamp)
{
	struct tm * timeinfo;
	// Перевод секунд от 1980 к секундам от 1970
	// (10 лет и два високосных дня)
	time_t rawtime = globalSecondsTime1980 - 315532800;

	timeinfo = localtime(&rawtime);

	timeStamp->timeMs = 0;
	timeStamp->time.bitfield.hours = timeinfo->tm_hour;
	timeStamp->time.bitfield.minutes = timeinfo->tm_min;
	timeStamp->time.bitfield.secondsDiv2 = timeinfo->tm_sec / 2;

	timeStamp->date.bitfield.day = timeinfo->tm_mday;
	timeStamp->date.bitfield.month = timeinfo->tm_mon;
	// Years in the FAT file system go from 1980-2108.
	timeStamp->date.bitfield.year = timeinfo->tm_year - 80;
}
//------------------------------------------------------------------------------