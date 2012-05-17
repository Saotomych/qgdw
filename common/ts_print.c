/*
 * Copyright (C) 2012 by Grygorii Musiiaka
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "ts_print.h"

static int ts_mode = TS_INFO + TS_VERBOSE;

void ts_setmode(int mode){

	ts_mode |= mode;

}

int ts_printf(int desc, int mode, const char *fmt, ...)
{
int res = 0;
	if (mode & ts_mode){

		char buf[TS_PRINT_BUFF_SIZE] = {0};
		va_list ap;

		va_start(ap, fmt);
		res = vsnprintf(buf, TS_PRINT_BUFF_SIZE, fmt, ap);
		va_end(ap);

		if(res != -1)
		{
			write(desc, buf, TS_PRINT_BUFF_SIZE);
		}
	}
	return res;
}

int ts_sprintf(char *dest, const char *fmt, ...)
{
	va_list ap;
	int res;

	va_start(ap, fmt);
	res = vsnprintf(dest, TS_PRINT_BUFF_SIZE, fmt, ap);
	va_end(ap);

	return res;
}
