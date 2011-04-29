/*
 * st7529.c
 *
 *  Created on: 28.04.2011
 *      Author: alex
 *      This module provide functions for low level working with controller st7529
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

static unsigned char info[3];
static unsigned char *video;
static unsigned char videolen;

static void st7529init(){

}

static unsigned int st7529exit(){

	return 0;
}

static void st7529writecmd(unsigned char cmd){
	// Write command to hardware driver
}

static void st7529writedat(unsigned char *buf, unsigned int len){
	// Write data buffer to hardware driver
}

static unsigned int st7529readinfo(){
	// Return pointer to info buffer

	return info;
}

static unsigned int st7529readdata(unsigned char *addr){
unsigned int len = videolen;
	// Return length & pointer to data buffer
	*addr = video;
	return len;
}

static AMLCDFUNC st7529func = {
	st7529init,
	st7529exit,
	st7529writecmd,
	st7529writedat,
	st7529readinfo,
	st7529readdata,
	AT91_PIN_PC4,
	AT91_PIN_PC5
};

//PAMLCDFUNC st7529_connect(){
//	return &st7529func;
//}

