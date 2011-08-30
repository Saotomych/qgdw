/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _M700_FRAME_H_
#define _M700_FRAME_H_


#include <stdint.h>
#include "../../common/asdu.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Constants and byte flags/masks */


/*
 *
 * Structures
 *
 */

/* Frame structure */
typedef struct m700_frame {
	/* Header */
	uint8_t			cmd;		/* function */
	uint8_t 		adr;		/* link address */

	/* Data */
	uint8_t			data_len;	/* length of the data */
	unsigned char	*data;		/* data of variable length frame */
} m700_frame;


/*
 *
 *  Functions
 *
 */

/* create m700_frame structure and initialize insides */
m700_frame *m700_frame_create();

/* cleanup frame structure insides and destroy it */
void m700_frame_destroy(m700_frame **frame);

/* parse input buffer to the frame structure */
uint16_t m700_frame_buff_parse(unsigned char *buff, uint32_t buff_len, uint32_t *offset, m700_frame *frame);

/* build buffer from given frame structure */
uint16_t m700_frame_buff_build(unsigned char **buff, uint32_t *buff_len, m700_frame *frame);


uint16_t m700_asdu_buff_parse(m700_frame *m_fr, asdu *m700_asdu);

#ifdef __cplusplus
}
#endif

#endif //_M700_FRAME_H_
