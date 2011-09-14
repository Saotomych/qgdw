/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _LPDU_FRAME_H_
#define _LPDU_FRAME_H_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */
/* LPDU constants */
#define LPDU_FT_11				1		/* FT 1.1 format */
#define LPDU_FT_12				2		/* FT 1.2 format */
#define LPDU_FT_2				3		/* FT 2 format */
#define LPDU_FT_3				4		/* FT 3 format */

#define LPDU_LEN_MIN			1		/* minimum LPDU length (single character confirmation) */

#define LPDU_ADR_LEN			1		/* default link address length (the same as common object (ASDU) address length) */

#define LPDU_FT_VAR_LEN			1		/* frame with variable length */
#define LPDU_FT_FIX_LEN			2		/* frame with fixed length */
#define LPDU_FT_SC_LEN			3		/* frame with single character */


/* Function codes of control field in messages sent from primary (PRM = 1) */
#define PRM_RESET_LINK			0x00	/* reset remote link */
#define PRM_RESET_PROCCESS		0x01	/* reset of user process */
#define PRM_TEST_LINK			0x02	/* test function for link */
#define PRM_DATA_WAIT			0x03	/* user data, confirmation expected */
#define PRM_DATA_NO_WAIT		0x04	/* user data, no confirmation/replay expected */
#define PRM_REQ_ACD				0x08	/* request access demand, expected response specifies access demand */
#define PRM_REQ_LINK_STATUS		0x09	/* request status of link */
#define PRM_REQ_DATA_CLASS_1	0x0A	/* request user data class 1 */
#define PRM_REQ_DATA_CLASS_2	0x0B	/* request user data class 2 */

/* Function codes of control field in messages sent from secondary (PRM = 0) */
#define SEC_CONFIRM_ACK			0x00	/* positive acknowledgment */
#define SEC_CONFIRM_NACK		0x01	/* negative acknowledgment: message not accepted, link busy */
#define SEC_DATA				0x08	/* user data */
#define SEC_DATA_NACK			0x09	/* requested data not available */
#define SEC_RESP_LINK_ACD		0x0B	/* status of link or access demand */
#define SEC_LINK_NO_FNC			0x0E	/* link service not functioning */
#define SEC_LINK_NO_IMPL		0x0F	/* link service not implemented */


/*
 *
 * Structures
 *
 */

/* Frame structure */
typedef struct lpdu_frame {
	/* Header */
	uint8_t 		type;		/* frame type */
	uint8_t			fnc;		/* function */
	uint8_t			fcv;		/* FCB valid(1)/invalid (0) */
	uint8_t			fcb;		/* frame count bit */
	uint8_t			dfc;		/* data flow control - overflow(1)/accept(0) */
	uint8_t			acd;		/* access demand(1)/no access demand(0) */
	uint8_t			prm;		/* primary(1)/secondary(0) message */
	uint8_t			dir;		/* from controlling station(1)/from controlled station(0) */

	uint8_t			sc;			/* single char frame */
	uint8_t 		res1;		/* padding */
	uint16_t 		res2;		/* padding */

	uint16_t 		adr;		/* link address */

	/* Data */
	uint16_t		data_len;	/* length of the data */
	unsigned char	*data;		/* data of variable length frame */
} lpdu_frame;


/*
 *
 *  Functions
 *
 */

/* create lpdu_frame structure and initialize insides */
lpdu_frame *lpdu_frame_create();

/* cleanup frame structure insides and destroy it */
void lpdu_frame_destroy(lpdu_frame **frame);

/* parse input buffer to the frame structure */
uint16_t lpdu_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, lpdu_frame *frame, uint8_t format, uint8_t adr_len);

/* build buffer from given frame structure */
uint16_t lpdu_frame_buff_build(unsigned char **buff, uint32_t *buff_len, lpdu_frame *frame, uint8_t format, uint8_t adr_len);


#ifdef __cplusplus
}
#endif

#endif //_LPDU_FRAME_H_
