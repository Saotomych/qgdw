/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _DLT_FRAME_H_
#define _DLT_FRAME_H_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */
/* Function codes of control field */
#define FNC_TIME_SYNC			0x08	/* time synchronization broadcast */
#define FNC_READ_DATA			0x11	/* read data */
#define FNC_READ_SSEQ_DATA		0x12	/* read subsequent data */
#define FNC_READ_ADDRESS		0x13	/* read slave station address */
#define FNC_WRITE_DATA			0x14	/* write data */
#define FNC_WRITE_ADDRESS		0x15	/* write slave station address */
#define FNC_FREEZE_DATA			0x16	/* freeze data */
#define FNC_SET_BAUD_RATE		0x17	/* set/change communication baud rate */
#define FNC_SET_PASSWORD		0x18	/* set/change slave station access password */
#define FNC_CLEAR_MAX_DEMAND	0x19	/* current maximum demand and time of occurrence  data clear */
#define FNC_CLEAR_DATA			0x1A	/* clear all accumulated data on the slave station */
#define FNC_CLEAR_EVENT_LOG		0x1B	/* clear event log data */

/* Direction */
#define DIR_REQUEST				0		/* request from controlling station */
#define DIR_RESPONSE			1		/* response from controlled station */


/*
 *
 * Structures
 *
 */

/* Frame structure */
typedef struct dlt_frame {
	/* Header */
	uint8_t			fnc;		/* function */
	uint8_t			sseq;		/* subsequent data frame(1)/no subsequent data frame(0) */
	uint8_t			asyn;		/* asynchronous slave response(1)/synchronous slave response(0) */
	uint8_t			dir;		/* from controlled station(1)/from controlling station(0) */

	uint16_t 		adr;		/* link address */
	uint64_t 		adr_hex;	/* link address in BCD format or address mask or broadcast address */

	/* Data */
	uint8_t			data_len;	/* length of the data */
	unsigned char	*data;		/* data of variable length frame */
} dlt_frame;


/*
 *
 *  Functions
 *
 */

/* create dlt_frame structure and initialize insides */
dlt_frame *dlt_frame_create();

/* cleanup frame structure insides and destroy it */
void dlt_frame_destroy(dlt_frame **frame);

/* parse input buffer to the frame structure */
uint16_t dlt_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, dlt_frame *frame);

/* build buffer from given frame structure */
uint16_t dlt_frame_buff_build(unsigned char **buff, uint32_t *buff_len, dlt_frame *frame);


#ifdef __cplusplus
}
#endif

#endif //_DLT_FRAME_H_
