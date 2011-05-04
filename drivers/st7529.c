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
static unsigned char *io_cmd, *io_data;								// virtual i/o indicator addresses

static void st7529init(void){
	writeb(0x30, io_cmd);		//ext = 0
	writeb(0x94, io_cmd);		//sleep out
	writeb(0xd1, io_cmd);		//osc on
	writeb(0x20, io_cmd);		// power c set
	writeb(8,io_data);			// booster first
	mdelay(1);
	writeb(0x20, io_cmd);		// power c set
	writeb(0xb, io_data);		// booster regulator follower on

	writeb(0x81, io_cmd);		//electornic ctl
	writeb(4, io_data);
	writeb(4, io_data);

	writeb(0xca, io_cmd);		// dispaly ctl
	writeb(0, io_data);
	writeb(0x27, io_data);
	writeb(0, io_data);

	writeb(0xa6, io_cmd);
	writeb(0xbb, io_cmd);
	writeb(0, io_data);

	writeb(0xbc, io_cmd);		// datascan direct
	writeb(0, io_data);
	writeb(0, io_data);
	writeb(1, io_data);

	writeb(0x75, io_cmd);
	writeb(0, io_data);
	writeb(0x9f, io_data);

	writeb(0x15, io_cmd);
	writeb(0, io_data);
	writeb(0x54, io_data);

	writeb(0x31, io_cmd);		//ext = 1
	writeb(0x32, io_cmd);		// analog circuit
	writeb(0, io_data);
	writeb(1, io_data);
	writeb(0, io_data);

	writeb(0x34, io_cmd);		// initial

	writeb(0x30, io_cmd);		// ext =0
	writeb(0xaf, io_cmd);		// display on

	writeb(0xa7, io_cmd);		// display on

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
	for (i=0; i<len; i++) writeb(buf[i], io_data);
	mdelay(10);
	st7529writecmd(0x5d);
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

