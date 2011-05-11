/*
 * st7529.c
 *
 *  Created on: 28.04.2011
 *      Author: alex AAV
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
static unsigned char *io_cmd, *io_data;								// virtual i/o indicator addresses

static inline void inlinecmd(unsigned char cmd){
	// Write command to hardware driver
	writeb(cmd, io_cmd);
}

static inline void inlinedat(unsigned char cmd){
	// Write command to hardware driver
	writeb(cmd, io_data);
}

static void st7529init(void){
	inlinecmd(0x30);		//ext = 0
	inlinecmd(0x94);		//sleep out
	inlinecmd(0xd1);		//osc on
	inlinecmd(0x20);		// power c set
	inlinedat(8);			// booster first
	mdelay(1);
	inlinecmd(0x20);		// power c set
	inlinedat(0xb);		// booster regulator follower on

	inlinecmd(0x81);		//electornic ctl
	inlinedat(4);
	inlinedat(4);

	inlinecmd(0xca);		// dispaly ctl
	inlinedat(0);
	inlinedat(0x27);
	inlinedat(0);

	inlinecmd(0xa6);
	inlinecmd(0xbb);
	inlinedat(0);

	inlinecmd(0xbc);		// datascan direct
	inlinedat(0);
	inlinedat(0);
	inlinedat(1);

	inlinecmd(0x75);
	inlinedat(0);
	inlinedat(0x9f);

	inlinecmd(0x15);
	inlinedat(0);
	inlinedat(0x54);

	inlinecmd(0x31);		//ext = 1
	inlinecmd(0x32);		// analog circuit
	inlinedat(0);
	inlinedat(1);
	inlinedat(0);

	inlinecmd(0x34);		// initial

	inlinecmd(0x30);		// ext =0
	inlinecmd(0xaf);		// display on

	inlinecmd(0xa7);		// display on

}

static unsigned int st7529exit(void){

	return 0;
}

static void st7529writecmd(unsigned char cmd){
	// Write command to hardware driver
	inlinecmd(cmd);
}

static void st7529writedat(unsigned char *buf, unsigned int len){
	// Write data buffer to hardware driver
unsigned int i;
	inlinecmd(0x5c);
	for (i=0; i<len; i++) inlinedat(buf[i]);
	mdelay(10);
	inlinecmd(0x5d);
	for (i=0; i<len; i++) buf[i] = readb(io_data);
}

static unsigned int st7529readinfo(void){
	// Return pointer to info buffer

	info[0] = readb(io_cmd);

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

