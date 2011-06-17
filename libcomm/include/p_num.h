/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _P_NUM_H_
#define _P_NUM_H_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/* byte-swap of 16-bit, 32-bit and 64-bit numbers */
#define	BYTESWAP16(v)  ((((v)&0xFF00) >> 8) |  \
                        (((v)&0x00FF) << 8))

#define	BYTESWAP32(v)  ((((v)&0xFF000000) >> 24) |  \
                        (((v)&0x00FF0000) >> 8)  |  \
                        (((v)&0x0000FF00) << 8)  |  \
                        (((v)&0x000000FF) << 24))

#define	BYTESWAP64(v)  ((((v)&0xFF00000000000000) >> 56) |  \
                        (((v)&0x00FF000000000000) >> 40) |  \
                        (((v)&0x0000FF0000000000) >> 24) |  \
                        (((v)&0x000000FF00000000) >> 8)  |  \
                        (((v)&0x00000000FF000000) << 8)  |  \
                        (((v)&0x0000000000FF0000) << 24) |  \
                        (((v)&0x000000000000FF00) << 40) |  \
                        (((v)&0x00000000000000FF) << 56))

/*=========================little-endian2le representation============================ */
/* pointer to integer */
#define pletoint16(p)  ((uint16_t)									\
                       ((uint16_t)*((const uint8_t *)(p)+1)<<8  |	\
                        (uint16_t)*((const uint8_t *)(p)+0)<<0))

#define pletoint24(p)  ((uint32_t)*((const uint8_t *)(p)+2)<<16 |	\
                        (uint32_t)*((const uint8_t *)(p)+1)<<8  |	\
                        (uint32_t)*((const uint8_t *)(p)+0)<<0)

#define pletoint32(p)  ((uint32_t)*((const uint8_t *)(p)+3)<<24 |	\
                        (uint32_t)*((const uint8_t *)(p)+2)<<16 |	\
                        (uint32_t)*((const uint8_t *)(p)+1)<<8  |	\
                        (uint32_t)*((const uint8_t *)(p)+0)<<0)

#define pletoint48(p)  ((uint64_t)*((const uint8_t *)(p)+5)<<40 |	\
                        (uint64_t)*((const uint8_t *)(p)+4)<<32 |	\
                        (uint64_t)*((const uint8_t *)(p)+3)<<24 |	\
                        (uint64_t)*((const uint8_t *)(p)+2)<<16 |	\
                        (uint64_t)*((const uint8_t *)(p)+1)<<8  |	\
                        (uint64_t)*((const uint8_t *)(p)+0)<<0)

#define pletoint64(p)  ((uint64_t)*((const uint8_t *)(p)+7)<<56 |	\
                        (uint64_t)*((const uint8_t *)(p)+6)<<48 |	\
                        (uint64_t)*((const uint8_t *)(p)+5)<<40 |	\
                        (uint64_t)*((const uint8_t *)(p)+4)<<32 |	\
                        (uint64_t)*((const uint8_t *)(p)+3)<<24 |	\
                        (uint64_t)*((const uint8_t *)(p)+2)<<16 |	\
                        (uint64_t)*((const uint8_t *)(p)+1)<<8  |	\
                        (uint64_t)*((const uint8_t *)(p)+0)<<0)

/* integer to pointer */
#define int16tople(p, v)											\
                       {											\
	                    ((uint8_t*)(p))[1] = (uint8_t)((v) >> 8);	\
                        ((uint8_t*)(p))[0] = (uint8_t)((v) >> 0);	\
                       }

#define int24tople(p, v)											\
                       {											\
                        ((uint8_t*)(p))[2] = (uint8_t)((v) >> 16);	\
                        ((uint8_t*)(p))[1] = (uint8_t)((v) >> 8);	\
                        ((uint8_t*)(p))[0] = (uint8_t)((v) >> 0);	\
                       }

#define int32tople(p, v)											\
                       {											\
                        ((uint8_t*)(p))[3] = (uint8_t)((v) >> 24);	\
                        ((uint8_t*)(p))[2] = (uint8_t)((v) >> 16);	\
                        ((uint8_t*)(p))[1] = (uint8_t)((v) >> 8);	\
                        ((uint8_t*)(p))[0] = (uint8_t)((v) >> 0);	\
                       }

#define int48tople(p, v)											\
                       {											\
                        ((uint8_t*)(p))[5] = (uint8_t)((v) >> 40);	\
                        ((uint8_t*)(p))[4] = (uint8_t)((v) >> 32);	\
                        ((uint8_t*)(p))[3] = (uint8_t)((v) >> 24);	\
                        ((uint8_t*)(p))[2] = (uint8_t)((v) >> 16);	\
                        ((uint8_t*)(p))[1] = (uint8_t)((v) >> 8);	\
                        ((uint8_t*)(p))[0] = (uint8_t)((v) >> 0);	\
                       }

#define int64tople(p, v)											\
                       {											\
                        ((uint8_t*)(p))[7] = (uint8_t)((v) >> 56);	\
                        ((uint8_t*)(p))[6] = (uint8_t)((v) >> 48);	\
                        ((uint8_t*)(p))[5] = (uint8_t)((v) >> 40);	\
                        ((uint8_t*)(p))[4] = (uint8_t)((v) >> 32);	\
                        ((uint8_t*)(p))[3] = (uint8_t)((v) >> 24);	\
                        ((uint8_t*)(p))[2] = (uint8_t)((v) >> 16);	\
                        ((uint8_t*)(p))[1] = (uint8_t)((v) >> 8);	\
                        ((uint8_t*)(p))[0] = (uint8_t)((v) >> 0);	\
                       }

uint8_t  buff_get_le_uint8(unsigned char *buff, uint32_t offset);

uint16_t buff_get_le_uint16(unsigned char *buff, uint32_t offset);

uint32_t buff_get_le_uint24(unsigned char *buff, uint32_t offset);

uint32_t buff_get_le_uint32(unsigned char *buff, uint32_t offset);

uint64_t buff_get_le_uint48(unsigned char *buff, uint32_t offset);

uint64_t buff_get_le_uint64(unsigned char *buff, uint32_t offset);

float buff_get_le_ieee_float(unsigned char *buff, uint32_t offset);


void buff_put_le_uint8(unsigned char *buff, uint32_t offset, uint32_t num);

void buff_put_le_uint16(unsigned char *buff, uint32_t offset, uint32_t num);

void buff_put_le_uint24(unsigned char *buff, uint32_t offset, uint32_t num);

void buff_put_le_uint32(unsigned char *buff, uint32_t offset, uint32_t num);

void buff_put_le_uint48(unsigned char *buff, uint32_t offset, uint64_t num);

void buff_put_le_uint64(unsigned char *buff, uint32_t offset, uint64_t num);

void buff_put_le_ieee_float(unsigned char *buff, uint32_t offset, float num);


/*=========================little-endian2le BCD representation======================== */

uint8_t buff_bcd_check(unsigned char *buff, uint32_t offset, uint8_t len);

uint64_t buff_bcd_get_le_uint(unsigned char *buff, uint32_t offset, uint8_t len);

float buff_bcd_get_le_flt(unsigned char *buff, int32_t offset, uint8_t len, uint8_t frac);


void buff_bcd_put_le_uint(unsigned char *buff, uint32_t offset, uint64_t num, uint8_t len);

void buff_bcd_put_le_flt(unsigned char *buff, uint32_t offset, float num, uint8_t len, uint8_t frac);


#ifdef __cplusplus
}
#endif

#endif //_P_NUM_H_
