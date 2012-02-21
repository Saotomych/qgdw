/*
 * Copyright (C) 2012 by Grygorii Musiiaka
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _TS_PRINTF_H_
#define _TS_PRINTF_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


#define TS_PRINT_BUFF_SIZE	512

int ts_printf(int desc, const char *fmt, ...);
int ts_sprintf(char *dest, const char *ftm, ...);


#ifdef __cplusplus
}
#endif

#endif //_TS_PRINTF_H_
