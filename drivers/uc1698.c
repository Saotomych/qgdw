/*
 * uc1698.c
 *
 *  Created on: 28.04.2011
 *      Author: alex
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
 * 			0			158
 * 1ln		bcdafghe		- 80 byte
 * 2ln
 *
 *
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

static void uc1698init(void){
    // Хардварная инициализация индикатора

        writeb(RESET, io_cmd);						// Reset
        mdelay(100);

        // Хардварная инициализация индикатора
        // default rgb mode, default 64k color mod

    	writeb(SETLCDBIASRT | 1, io_cmd);			//Bias Ratio:1/10 bias
    	writeb(SETPWRCTL | 3, io_cmd);				//power control set as internal power
    	writeb(SETTEMPCOMP | off, io_cmd);			//set temperate compensation as 0%
    	writeb(SETV_2B, io_cmd);					//electronic potentiometer
    	writeb(0xc6, io_cmd);						// potentiometer value

    	writeb(SETLCDMAP | 4, io_cmd);					//18:partial display and MX disable,MY enable
    	writeb(SETLNRATE | 3, io_cmd);				//line rate 15.2klps
    	writeb(SETPARTCTL | off, io_cmd);			//12:partial display control disable
    	writeb(SETRAMCTL | 1, io_cmd);

    	writeb(SETNLNINV_2B, io_cmd);
    	writeb(off, io_cmd);						// disable NIV

    	/*com scan fuction*/
    	writeb(SETCOMSCAN | 4, io_cmd);				//enable FRC,PWM,LRM sequence
    	writeb(SETCOMEND_2B, io_cmd);				//com end
    	writeb(159, io_cmd);						//160

    	/* color functions */
    	writeb(SETCOLPAT | 1, io_cmd);					// rgb mode
		writeb(SETCOLMOD | 1, io_cmd);				// 4bit mode

    	/*window*/
    	writeb(SETWINCOLSTART_2B, io_cmd);
    	writeb(0x0C, io_cmd);
    	writeb(SETWINCOLEND_2B, io_cmd);
    	writeb(0x5a, io_cmd);							// 80 byte = 160 b/w pixel

    	writeb(SETWINROWSTART_2B, io_cmd);
    	writeb(0, io_cmd);
    	writeb(SETWINROWEND_2B, io_cmd);
    	writeb(0x9f, io_cmd);
    	writeb(WINPRGMOD, io_cmd);					//inside mode

    	/*scroll line*/
    	writeb(SETSCRLN_L, io_cmd);
    	writeb(SETSCRLN_H, io_cmd);
    	writeb(SETFIXLN_2B, io_cmd);			//14:FLT,FLB set
    	writeb(0, io_cmd);

    	writeb(SETALLPXON | 0, io_cmd);			// all pixel off
    	writeb(SETALLPXINV | on, io_cmd);		// inversed

    	writeb(SETDISPEN | 5, io_cmd);			//display on,select on/off mode.Green Enhance mode disable

    	/*partial display*/
/*    	writeb(SETPARTCTL, io_cmd);				//12,set partial display control:off
    	writeb(SETPARTSTART_2B, io_cmd);			//display start
    	writeb(0, io_cmd);						//0
    	writeb(SETPARTEND_2B, io_cmd);			//display end
    	writeb(159, io_cmd);			//160*/

}

static unsigned int uc1698exit(void){
	iounmap(io_cmd);
	iounmap(io_data);
	return 0;
}

static void uc1698writecmd(unsigned char cmd){
	// Write command to hardware driver
	writeb(cmd, io_cmd);
}

static void uc1698writedat(unsigned char *buf, unsigned int len){
unsigned int x, y, sadr=0;
int endx=len%80, endy=len/80;

// Write data buffer to hardware driver
// Full rows
	for (y=0; y < endy; y++){
		uc1698writecmd(SETCOLADDR_L | 5);
		uc1698writecmd(SETCOLADDR_H | 2);
		uc1698writecmd(SETROWADDR_L | (y&0xF) );
		uc1698writecmd(SETROWADDR_H | (y>>4) );
		for(x=0; x < 80; x++){
			writeb(buf[sadr], io_data);
			sadr++;
		}
	}

// Last row
	uc1698writecmd(SETCOLADDR_L | 5);
	uc1698writecmd(SETCOLADDR_H | 2);
	uc1698writecmd(SETROWADDR_L | (y&0xF) );
	uc1698writecmd(SETROWADDR_H | (y>>4) );
	for(x=0; x < endx; x++){
		writeb(buf[sadr], io_data);
		sadr++;
	}

	printk(KERN_INFO "len(%d)\n",len);
//	mdelay(10);
//	for (i=0; i<len; i++) buf[i] = readb(io_data);
}

static unsigned int uc1698readinfo(void){
	// Return pointer to info buffer

    info[0] = readb(io_cmd);
    info[1] = readb(io_cmd);
    info[2] = readb(io_cmd);

	return info;
}

static unsigned char* uc1698readdata(unsigned int len){
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

PAMLCDFUNC uc1698_connect(unsigned char *io_c, unsigned char *io_d){
	io_data = io_d;
	io_cmd = io_c;

	return &uc1698func;
}
