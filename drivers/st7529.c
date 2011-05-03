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
static unsigned char *io_cmd, *io_data;

static void st7529init(void){

}

static unsigned int st7529exit(void){

	return 0;
}

static void st7529writecmd(unsigned char cmd){
	// Write command to hardware driver
	writeb(cmd, io_cmd);
}

static void st7529writedat(unsigned char *buf, unsigned int len){
	// Write data buffer to hardware driver
unsigned int i;
	st7529writecmd(0x5c);
	for (i=0; i<len; i++) writeb((i&1 ? 0 : 0xff), io_data);
}

static unsigned int st7529readinfo(void){
	// Return pointer to info buffer

	return info;
}

static unsigned char* st7529readdata(unsigned char *addr, unsigned int len){
	// Return length & pointer to data buffer
	return video;
}

static AMLCDFUNC st7529func = {
	st7529init,
	st7529writecmd,
	st7529writedat,
	st7529readinfo,
	st7529readdata,
	st7529exit,
};

PAMLCDFUNC st7529_connect(unsigned char *io_c, unsigned char *io_d){
	io_data = io_d;
	io_cmd = io_c;
    printk(KERN_INFO "set i/o sram: %lX; %lX\n", io_cmd, io_data);
	return &st7529func;
}

