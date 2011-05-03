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
static unsigned char *io_cmd, *io_data, *io_cmdwr, *io_datawr;								// virtual i/o indicator addresses

static void uc1698init(void){
    // Хардварная инициализация индикатора

        writeb(RESET, io_cmd);						// Reset, Дальнейшие операции регистрации фреймбуфера дадут задержку до операций с индикатором
        mdelay(10);

        // Хардварная инициализация индикатора
        // default gbr mode, default 64k color mod

    	writeb(SETLCDBIASRT | 1, io_cmd);			//Bias Ratio:1/10 bias
    	writeb(SETPWRCTL | 3, io_cmd);				//power control set as internal power
    	writeb(SETTEMPCOMP | off, io_cmd);			//set temperate compensation as 0%
    	writeb(SETV_2B, io_cmd);					//electronic potentiometer
    	writeb(0xc6, io_cmd);						// potentiometer value
    	writeb(SETALLPXON | 0, io_cmd);				// all pixel on
    	writeb(SETALLPXINV | on, io_cmd);			// not inversed

    	writeb(SETLCDMAP, io_cmd);					//19:partial display and MX disable,MY enable
    	writeb(SETLNRATE | 3, io_cmd);				//line rate 15.2klps
    	writeb(SETPARTCTL | off, io_cmd);				//12:partial display control disable

    	writeb(SETNLNINV_2B, io_cmd);
    	writeb(off, io_cmd);							// disable NIV

    	/*com scan fuction*/
    	writeb(SETCOMSCAN | 4, io_cmd);				//enable FRC,PWM,LRM sequence

    	/*window*/
    	writeb(SETWINCOLSTART_2B, io_cmd);
    	writeb(0, io_cmd);
    	writeb(SETWINCOLEND_2B, io_cmd);
    	writeb(0x35, io_cmd);							// 53 fullcolor pixel = 160 b/w pixel

    	writeb(SETWINROWSTART_2B, io_cmd);
    	writeb(0, io_cmd);
    	writeb(SETWINROWEND_2B, io_cmd);
    	writeb(0x9f, io_cmd);

    	writeb(WINPRGMOD, io_cmd);					//inside mode

    	writeb(SETRAMCTL | 1, io_cmd);

    	writeb(SETDISPEN | 5, io_cmd);			//display on,select on/off mode.Green Enhance mode disable

    	/*scroll line*/
    	writeb(SETSCRLN_L, io_cmd);
    	writeb(SETSCRLN_H, io_cmd);
    	writeb(SETLCDMAP | 4, io_cmd);			//19,enable FLT and FLB
    	writeb(SETFIXLN_2B, io_cmd);				//14:FLT,FLB set
    	writeb(0x00, io_cmd);

    	/*partial display*/
    	writeb(SETPARTCTL, io_cmd);				//12,set partial display control:off
    	writeb(SETCOMEND_2B, io_cmd);				//com end
    	writeb(159, io_cmd);						//160
    	writeb(SETPARTSTART_2B, io_cmd);			//display start
    	writeb(0, io_cmd);						//0
    	writeb(SETPARTEND_2B, io_cmd);			//display end
    	writeb(159, io_cmd);			//160

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
	// Write data buffer to hardware driver
	unsigned int i;
		for(i=0; i<len; i++) writeb(buf[i], io_data);
		mdelay(10);
		for (i=0; i<len; i++) buf[i] = readb(io_data);
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

PAMLCDFUNC uc1698_connect(unsigned char *io_c, unsigned char *io_d, unsigned char *io_cw, unsigned char *io_dw){
	io_data = io_d;
	io_cmd = io_c;
	io_datawr = io_dw;
	io_cmdwr = io_cw;

	return &uc1698func;
}
