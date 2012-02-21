/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */

#include <stdio.h>
#include <string.h>
#include "../include/p_num.h"


uint32_t buff_get_le_uint(unsigned char *buff, uint32_t offset, uint8_t size)
{
	if(!buff) return 0;

	switch(size)
	{
	case 4:
		return buff_get_le_uint32(buff, offset);
		break;
	case 3:
		return buff_get_le_uint24(buff, offset);
		break;
	case 2:
		return buff_get_le_uint16(buff, offset);
		break;
	case 1:
		return  buff_get_le_uint8(buff, offset);
		break;
	}

	return 0;
}


uint8_t  buff_get_le_uint8(unsigned char *buff, uint32_t offset)
{
	if(!buff) return 0;

	const uint8_t* ptr = buff + offset;

	return *ptr;
}


uint16_t buff_get_le_uint16(unsigned char *buff, uint32_t offset)
{
	if(!buff) return 0;

	const uint8_t* ptr = buff + offset;

	return pletoint16(ptr);
}


uint32_t buff_get_le_uint24(unsigned char *buff, uint32_t offset)
{
	if(!buff) return 0;

	const uint8_t* ptr = buff + offset;

	return pletoint24(ptr);
}


uint32_t buff_get_le_uint32(unsigned char *buff, uint32_t offset)
{
	if(!buff) return 0;

	const uint8_t* ptr = buff + offset;

	return pletoint32(ptr);
}


uint64_t buff_get_le_uint48(unsigned char *buff, uint32_t offset)
{
	if(!buff) return 0;

	const uint8_t* ptr = buff + offset;

	return pletoint48(ptr);
}


uint64_t buff_get_le_uint64(unsigned char *buff, uint32_t offset)
{
	if(!buff) return 0;

	const uint8_t* ptr = buff + offset;

	return pletoint64(ptr);
}


float buff_get_le_ieee_float(unsigned char *buff, uint32_t offset)
{
	if(!buff) return 0;

	const uint8_t* ptr = buff + offset;

	union {
		float f;
		uint32_t i;
	} ieee_fp;

	ieee_fp.i = pletoint32(ptr);

	return ieee_fp.f;
}


void buff_put_le_uint8(unsigned char *buff, uint32_t offset, uint32_t num)
{
	if(!buff) return;

	uint8_t* const ptr = buff + offset;

	*ptr = num;
}


void buff_put_le_uint16(unsigned char *buff, uint32_t offset, uint32_t num)
{
	if(!buff) return;

	uint8_t* const ptr = buff + offset;

	int16tople(ptr, num);
}


void buff_put_le_uint24(unsigned char *buff, uint32_t offset, uint32_t num)
{
	if(!buff) return;

	uint8_t* const ptr = buff + offset;

	int24tople(ptr, num);
}


void buff_put_le_uint32(unsigned char *buff, uint32_t offset, uint32_t num)
{
	if(!buff) return;

	uint8_t* const ptr = buff + offset;

	int32tople(ptr, num);
}


void buff_put_le_uint48(unsigned char *buff, uint32_t offset, uint64_t num)
{
	if(!buff) return;

	uint8_t* const ptr = buff + offset;

	int48tople(ptr, num);
}


void buff_put_le_uint64(unsigned char *buff, uint32_t offset, uint64_t num)
{
	if(!buff) return;

	uint8_t* const ptr = buff + offset;

	int64tople(ptr, num);
}


void buff_put_le_ieee_float(unsigned char *buff, uint32_t offset, float num)
{
	if(!buff) return;

	uint8_t* const ptr = buff + offset;

	union {
		float f;
		uint32_t i;
	} ieee_fp;

	ieee_fp.f = num;

	int32tople(ptr, ieee_fp.i);
}


float pow_x(float x, uint8_t pow)
{
	float num = 1.0;

	while(pow > 0)
	{
		if (pow % 2 == 0)
		{
			pow /= 2;
			x *= x;
		}
		else
		{
			pow--;
			num *= x;
		}
	}

	return num;
}


uint8_t buff_bcd_check(unsigned char *buff, uint32_t offset, uint8_t len)
{
	if(!buff) return 0;

	uint8_t* const ptr = buff + offset;

	uint8_t i;

	for(i=0; i<len; i++)
	{
	    if((ptr[i] & 0x0F) >= 0x0A || (ptr[i] & 0xF0) >= 0xA0) return 0;
	}

	return 1;
}


uint64_t buff_bcd_get_le_uint(unsigned char *buff, uint32_t offset, uint8_t len)
{
	if(!buff || len > 8 || !buff_bcd_check(buff, offset, len)) return 0;

	const uint8_t* ptr = buff + offset;

	uint64_t num = 0;
	uint8_t i;

	for(i=0; i<len; i++)
	{
		num *= 100;

		num += (ptr[len-i-1] & 0x0F) + ((ptr[len-i-1] & 0xF0) >> 4) * 10;
	}

	return num;
}


float buff_bcd_get_le_flt(unsigned char *buff, int32_t offset, uint8_t len, uint8_t frac)
{
	float num = (float) buff_bcd_get_le_uint(buff, offset, len);

	// move float point over the fraction
	num = num / pow_x(10, frac);

	return num;
}


void buff_bcd_put_le_uint(unsigned char *buff, uint32_t offset, uint64_t num, uint8_t len)
{
	if(!buff || len > 8) return;

	uint8_t* const ptr = buff + offset;

	uint8_t i;

	for(i=0; i<len; i++)
	{
		ptr[i] = (num % 10) | (((num / 10) % 10) << 4);

		num /= 100;
	}
}


void buff_bcd_put_le_flt(unsigned char *buff, uint32_t offset, float num, uint8_t len, uint8_t frac)
{
	if(num < 0 || num >= pow_x(10, len * 2 - frac)) return;

	// move float point over the fraction
	uint64_t uint_num = (uint64_t) (num * pow_x(10, frac));

	buff_bcd_put_le_uint(buff, offset, uint_num, len);

	return;
}


int hex2ascii(unsigned char *h_buff, char *c_buff, int len)
{
	char tmp[4];
	int i;

	c_buff[0]=0;

	for(i=0; i<len; i++)
	{
		snprintf(tmp, 4, "%02X", h_buff[i]);
		strcat(c_buff, tmp);
	}

	return len;
}


