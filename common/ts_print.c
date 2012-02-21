/*
 * Copyright (C) 2012 by Grygorii Musiiaka
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "ts_print.h"

int ts_printf(int desc, const char *fmt, ...)
{
	char buf[TS_PRINT_BUFF_SIZE] = {0};
	va_list ap;
	int res;

	va_start(ap, fmt);
	res = vsnprintf(buf, TS_PRINT_BUFF_SIZE, fmt, ap);
	va_end(ap);

	if(res != -1)
	{
		write(desc, buf, TS_PRINT_BUFF_SIZE);
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
