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

#include "lcdfuncs.h"

static unsigned char info[3];
static unsigned char *video;
static unsigned char videolen;

void st7529init(){

}

unsigned int st7529exit(){

	return 0;
}

void st7529writecmd(unsigned char cmd){
	// Write command to hardware driver
}

void st7529writedat(unsigned char *buf, unsigned int len){
	// Write data buffer to hardware driver
}

unsigned int st7529readinfo(){
	// Return pointer to info buffer

	return info;
}

unsigned int readdata(unsigned char *addr){
unsigned int len = videolen;
	// Return length & pointer to data buffer
	*addr = video;
	return len;
}

AMLCDFUNC st7529func = {
	init = st7529init,
	exit = st7529exit,
	writecmd = st7529writecmd,
	writedat = st7529writedat,
	readinfo = st7529readinfo,
	readdata = st7529readdata
};

static AMLCDFUNC* st7529_connect(){
	return st7529func;
}

