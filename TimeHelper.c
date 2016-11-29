/*
 * File:   TimeHelper.c
 * Author: ionash-an
 *
 * Created on 25 ноября 2016 г., 8:24
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "TimeHelper.h"

static const char* monthesMMM[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const uint8_t monthesDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int8_t TIME_GetIndexOfMMM(const char mmm[])
{
	int8_t i;
	for (i = 0; i < 12; ++i)
		if (memcmp(monthesMMM[i], mmm, 3) == 0)
			return i;

	return -1;
}

static bool TIME_IsLeapYear(uint8_t year)
{
	if (year % 400 == 0)
		return true;
	else if (year % 100 == 0)
		return false;
	else if (year % 4 == 0)
		return true;
	else
		return false;
}

static uint16_t TIME_CalculateDaysSinceJanuary1(const struct tm *date)
{
	uint16_t days = 0;

	uint8_t i;
	for (i = 0; i < date->tm_mon; ++i)
		days += monthesDays[i];

	days += date->tm_mday - 1;

	if (TIME_IsLeapYear(date->tm_year + 1900))
		++days;

	return days;
}

#define WEEK_OFFSET_MONTHES_CYCLE 12
#define WEEK_OFFSET_YEARS_CYCLE 28
static const uint8_t weekOffsetForMonth[] = {6, 2, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
static const uint8_t weekOffsetForYears[] = {6, 0, 1, 2, 4, 5, 6, 0, 2, 3, 4, 5, 0, 1, 2, 3, 5, 6, 0, 1, 3, 4, 5, 6, 1, 2, 3, 4};

static uint8_t TIME_CalculateDaysSinceSunday(const struct tm *date)
{
	uint8_t year = date->tm_year + 1900;

	uint8_t wday = (date->tm_mday + weekOffsetForMonth[date->tm_mon] + weekOffsetForYears[year % 28]);

	if (TIME_IsLeapYear(year))
		if (date->tm_mon == 0 || date->tm_mon == 1)
			--wday;

	return wday % 7;
}

bool TIME_GetCompileTime(struct tm *output)
{
	/*НАЧАЛО: Компоненты, содержащиеся в явном виде*/
	// "Feb 12 1996"
	char cDate[] = __DATE__;
	cDate[3] = cDate[6] = 0;

	output->tm_mon = TIME_GetIndexOfMMM(cDate + 0);
	if (output->tm_mon == -1)
		return false;
	output->tm_mday = atoi(cDate + 4);

	uint16_t year = atoi(cDate + 7);
	output->tm_year = year - 1900;

	// "23:59:01"
	char cTime[] = __TIME__;
	cTime[2] = cTime[5] = 0;

	output->tm_hour = atoi(cTime + 0);
	output->tm_min = atoi(cTime + 3);
	output->tm_sec = atoi(cTime + 6);
	/*КОНЕЦ: Компоненты, содержащиеся в явном виде*/

	output->tm_isdst = 0; // Переход на летнее время не действует	
	output->tm_yday = TIME_CalculateDaysSinceJanuary1(output);
	output->tm_wday = TIME_CalculateDaysSinceSunday(output);

	return true;
}