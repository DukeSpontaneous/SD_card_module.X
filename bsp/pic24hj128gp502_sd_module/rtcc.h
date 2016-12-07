#include <stdint.h>
#include <stdbool.h>

#ifndef _RTCC_H
#define _RTCC_H

typedef struct
{
    bool bcdFormat;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} RTCC_DATETIME;

void RTCC_Initialize (RTCC_DATETIME * value);
void RTCC_TimeGet (RTCC_DATETIME * value);
void RTCC_BuildTimeGet( RTCC_DATETIME *dateTime);

/* Deprecated API.  Will be removed in future release. */
#define BSP_RTCC_DATETIME RTCC_DATETIME
void __attribute__((deprecated)) BSP_RTCC_Initialize (BSP_RTCC_DATETIME * value);
void __attribute__((deprecated)) BSP_RTCC_TimeGet (BSP_RTCC_DATETIME * value);

#endif