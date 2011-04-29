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

#include "lcdfuncs.h"

static unsigned char info[3];
static unsigned char *video;
static unsigned char videolen = 0;

void uc1698init(){
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
    	writeb(SETALLPXON | 1, io_cmd);				// all pixel on
    	writeb(SETALLPXINV | off, io_cmd);			// not inversed

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

unsigned int uc1698exit(){

	return 0;
}

void uc1698writecmd(unsigned char cmd){
	// Write command to hardware driver
}

void uc1698writedat(unsigned char *buf, unsigned int len){
	// Write data buffer to hardware driver
}

unsigned int uc1698readinfo(){
	// Return pointer to info buffer

    info[0] = readb(io_cmd);
    info[1] = readb(io_cmd);
    info[2] = readb(io_cmd);

	return info;
}

unsigned int readdata(unsigned char *addr){
unsigned int len = videolen;
	// Return length & pointer to data buffer
	addr = video;
	return len;
}

AMLCDFUNC uc1698func = {
	init = uc1698init,
	exit = uc1698exit,
	writecmd = uc1698writecmd,
	writedat = uc1698writedat,
	readinfo = uc1698readinfo,
	readdata = uc1698readdata,
	resetpin = AT91_PIN_PC4,
	lightpin = AT91_PIN_PC5
};

static AMLCDFUNC* uc1698_connect(){
	return uc1698func;
}
