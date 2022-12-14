/*
 * uc1698.c
 *
 *  Created on: 28.04.2011
 *      Author: alex AAV
 *
 *      This module provide functions for low level working with controller uc1698
 *      init
 *      exit
 *      writecmd
 *      writedat
 *      readinfo
 *      readdata
 *
 *      Bitmap
 *      0 - NC
 *      1 - NC
 *      2 - NC
 *      3 - четные точки с нулевой
 *      4 - NC
 *      5 - NC
 *      6 - NC
 *      7 - нечетные точки с первой
 *
 *		Screen map - rgb mode, 4 bit
 * 		7bit(a)..3bit(b) | 7bit(c)..3bit(d) | 7bit(e)..3bit(f) | 7bit(g)..3bit(h)
 *
 *		adr		0			79
 * 		0ln 		margin	- 80 byte
 * 		adr		80			159
 * 		1ln		abdcfehg	- 80 byte
 * 		adr		160			239
 * 		2ln					- 80 byte
 *
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/backlight.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

#include "lcdfuncs.h"

#define 	off					0
#define 	on					1
// UC1698 Command set
#define 	SETCOLADDR_L		0x00
#define 	SETCOLADDR_H		0x10
#define 	SETTEMPCOMP			0x24
#define		SETPWRCTL			0x28
#define 	SETPRGCTL_2B		0x30
#define		SETSCRLN_L			0x40
#define 	SETSCRLN_H			0x50
#define		SETROWADDR_L		0x60
#define		SETROWADDR_H		0x70
#define		SETV_2B				0x81
#define		SETPARTCTL			0x84
#define		SETRAMCTL			0x88
#define		SETFIXLN_2B			0x90
#define		SETLNRATE			0xA0
#define		SETALLPXON			0xA4
#define 	SETALLPXINV			0xA6
#define		SETDISPEN			0xA8
#define		SETLCDMAP			0xC0
#define		SETNLNINV_2B		0xC8
#define		SETCOLPAT			0xD0
#define		SETCOLMOD			0xD4
#define		SETCOMSCAN			0xD8
#define		RESET				0xE2
#define		NOP					0xE3
#define		SETTESTCTL_2B		0xE4
#define		SETLCDBIASRT		0xE8
#define		SETCOMEND_2B		0xF1
#define		SETPARTSTART_2B		0xF2
#define		SETPARTEND_2B		0xF3
#define		SETWINCOLSTART_2B	0xF4
#define		SETWINROWSTART_2B	0xF5
#define		SETWINCOLEND_2B		0xF6
#define		SETWINROWEND_2B		0xF7
#define		WINPRGMOD			0xF8
#define		SETMTPOPS_2B		0xB8
#define		SETMTPWRMASK_3B		0xB9
#define		SETV1_2B			0xB4
#define		SETV2_2B			0xB5
#define		SETMTPWRTMR_2B		0xF6
#define		SETMTPRDTMR_2B		0xF7

static unsigned char info[3];
static unsigned char *video;
static unsigned char videolen = 0;
static unsigned char *io_cmd, *io_data;								// virtual i/o indicator addresses
static unsigned int endy;

static inline void inlinecmd(unsigned char cmd){
	// Write command to hardware driver
	writeb(cmd, io_cmd);
}

// Set initial state of hardware
static void uc1698init(void){

        inlinecmd(RESET);						// Reset
        mdelay(100);

        // default rgb mode, default 64k color mod

    	inlinecmd(SETLCDBIASRT | 1);			//Bias Ratio:1/10 bias
    	inlinecmd(SETPWRCTL | 3);				//power control set as internal power
    	inlinecmd(SETTEMPCOMP | off);			//set temperate compensation as 0%
    	inlinecmd(SETV_2B);					//electronic potentiometer
    	inlinecmd(0xc6);						// potentiometer value

    	inlinecmd(SETLCDMAP | 4);					//18:partial display and MX disable,MY enable
    	inlinecmd(SETLNRATE | 3);				//line rate 15.2klps
    	inlinecmd(SETPARTCTL | off);			//12:partial display control disable
    	inlinecmd(SETRAMCTL | 1);

    	inlinecmd(SETNLNINV_2B);
    	inlinecmd(off);						// disable NIV

    	/*com scan fuction*/
    	inlinecmd(SETCOMSCAN | 4);				//enable FRC,PWM,LRM sequence
    	inlinecmd(SETCOMEND_2B);				//com end
    	inlinecmd(159);						//160

    	/* color functions */
    	inlinecmd(SETCOLPAT | 1);					// rgb mode
		inlinecmd(SETCOLMOD | 1);				// 4bit mode

    	/*window*/
    	inlinecmd(SETWINCOLSTART_2B);
    	inlinecmd(0x0C);
    	inlinecmd(SETWINCOLEND_2B);
    	inlinecmd(0x5a);							// 80 byte = 160 b/w pixel

    	inlinecmd(SETWINROWSTART_2B);
    	inlinecmd(0);
    	inlinecmd(SETWINROWEND_2B);
    	inlinecmd(0x9f);
    	inlinecmd(WINPRGMOD);					//inside mode

    	/*scroll line*/
    	inlinecmd(SETSCRLN_L);
    	inlinecmd(SETSCRLN_H);
    	inlinecmd(SETFIXLN_2B);			//14:FLT,FLB set
    	inlinecmd(0);

    	inlinecmd(SETALLPXON | 0);			// all pixel off
    	inlinecmd(SETALLPXINV | on);		// inversed

    	inlinecmd(SETDISPEN | 5);			//display on,select on/off mode.Green Enhance mode disable

    	/*partial display*/
/*    	inlinecmd(SETPARTCTL, io_cmd);				//12,set partial display control:off
    	inlinecmd(SETPARTSTART_2B, io_cmd);			//display start
    	inlinecmd(0, io_cmd);						//0
    	inlinecmd(SETPARTEND_2B, io_cmd);			//display end
    	inlinecmd(159, io_cmd);			//160*/

}

// Unmap io_memory before exit
static unsigned int uc1698exit(void){
	iounmap(io_cmd);
	iounmap(io_data);
	return 0;
}

static void uc1698writecmd(unsigned char cmd){
	// Write command to hardware driver
	writeb(cmd, io_cmd);
}

// Convert to indicator's format with record to indi memory
static void uc1698writedat(unsigned char *buf){
unsigned int x, y;
unsigned char ibt, obt, i, mask, *pv = buf;

// Write data buffer to hardware driver
// Full rows
	for (y=0; y < endy; y++){
		inlinecmd(SETCOLADDR_L | 5);
		inlinecmd(SETCOLADDR_H | 2);
		inlinecmd(SETROWADDR_L | (y&0xF) );
		inlinecmd(SETROWADDR_H | (y>>4) );
		for(x=0; x < 20; x++){
			mask = 0x80;
			ibt = *pv;
			for (i=0; i<4; i++){
				obt = 0;
				if (ibt & mask) obt = 0x80;
				mask>>=1;
				if (ibt & mask) obt |= 8;
				mask>>=1;

				// Writing to indicator memory
				writeb(obt, io_data);
			}
			pv++;
		}
		writeb(0, io_data);
	}
}

// Read status from LCD
static unsigned long uc1698readinfo(void){
	// Return pointer to info buffer

    info[0] = readb(io_cmd);
    info[1] = readb(io_cmd);
    info[2] = readb(io_cmd);

	return (unsigned long) info;
}

// Read screen data from memory or hardware (for some chips)
static unsigned char* uc1698readdata(unsigned char *addr, unsigned int len){
	// Return length & pointer to data buffer
//	for (i=0; i<len; i++) video[i] = readb(io_data);
	return video;
}

static AMLCDFUNC uc1698func = {
	uc1698init,
	uc1698writecmd,
	uc1698writedat,
	uc1698readinfo,
	uc1698readdata,
	uc1698exit
};

// Set io memory addresses
PAMLCDFUNC uc1698_connect(unsigned char *io_c, unsigned char *io_d,  unsigned int len){
	io_data = io_d;
	io_cmd = io_c;
	endy = len/20;

	return &uc1698func;
}
