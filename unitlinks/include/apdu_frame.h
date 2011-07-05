/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _APDU_FRAME_H_
#define _APDU_FRAME_H_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */
/* Response codes */
#define RES_APDU_SUCCESS		0x00	/* no error(s) */
#define RES_APDU_INCORRECT		0x01	/* frame or/and buffer are incorrect */
#define RES_APDU_UNKNOWN		0x02	/* unknown frame type */
#define RES_APDU_UNSUPPORTED	0x04	/* unsupported frame type */
#define RES_APDU_MEM_ALLOC		0x08	/* memory allocation error */
#define RES_APDU_FCS_INCORRECT	0x10	/* frame checksum incorrect */
#define RES_APDU_LEN_INVALID	0x20	/* frame length invalid */


/* APDU constants */
#define APDU_LEN_MIN			6		/* minimum LPDU length */

/* ACPI control masks/flags */
/* Control field flags */
#define APCI_TYPE_I				0x00	/* I-Format frame */
#define APCI_TYPE_S				0x01	/* S-Format frame */
#define APCI_TYPE_U				0x03	/* U-Format frame */

#define APCI_LINK_ON			1
#define APCI_LINK_OFF			0


/* U-Format flags */
#define APCI_U_STARTDT_ACT		0x04	/* start data transfer activation request */
#define APCI_U_STARTDT_CON		0x08	/* start data transfer confirmation */
#define APCI_U_STOPDT_ACT		0x10	/* stop data transfer activation request */
#define APCI_U_STOPDT_CON		0x20	/* stop data transfer confirmation */
#define APCI_U_TESTFR_ACT		0x40	/* test frame activation */
#define APCI_U_TESTFR_CON		0x80	/* test frame confirmation */


/*
 *
 * Structures
 *
 */

/* Frame structure */
typedef struct apdu_frame {
	/* Header */
	uint8_t 		type;		/* frame type */
	uint16_t		send_num;	/* send sequence number */
	uint16_t		recv_num;	/* receive sequence number */
	uint8_t			u_cmd;		/* U-Format command code */

	/* Data */
	uint8_t			data_len;	/* length of the data */
	unsigned char	*data;		/* data of I-Format frame */
} apdu_frame;


/*
 *
 *  Functions
 *
 */

/* create apdu_frame structure */
apdu_frame *apdu_frame_create();

/* cleanup frame structure insides and destroy it */
void apdu_frame_destroy(apdu_frame **frame);

/* parse input buffer to the frame structure */
uint8_t apdu_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, apdu_frame *frame);

/* build buffer from given frame structure */
uint8_t apdu_frame_buff_build(unsigned char **buff, uint32_t *buff_len, apdu_frame *frame);


#ifdef __cplusplus
}
#endif

#endif //_APDU_FRAME_H_
