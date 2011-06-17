/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _IEC_TYPES_H_
#define _IEC_TYPES_H_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/*===============================Basic Types Start==================================*/
/* CP56Time2a */
typedef struct cp56time2a {
	uint16_t	msec;		/* milliseconds */
	uint8_t		min;		/* minutes */
	uint8_t		res1;		/* changed(1)/original(0) */
	uint8_t		iv;			/* invalid(1)/valid(0) */
	uint8_t		hour;		/* hours */
	uint8_t		su;			/* summer time(1)/standard time(0) */
	uint8_t		mday;		/* month's day(1-31) */
	uint8_t		wday;		/* week's day since Monday(1-7) */
	uint8_t		month;		/* month(1-12) */
	uint8_t		year;		/* year(since 2000) */
} cp56time2a;
/*===============================Basic Types End====================================*/


#ifdef __cplusplus
}
#endif

#endif //_IEC_TYPES_H_
