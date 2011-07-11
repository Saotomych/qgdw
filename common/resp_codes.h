/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _RESP_CODES_H_
#define _RESP_CODES_H_


/* Response codes */
#define RES_SUCCESS			0x0000	/* no error(s) */
#define RES_INCORRECT		0x0001	/* incorrect */
#define RES_UNKNOWN			0x0002	/* unknown */
#define RES_UNSUPPORTED		0x0004	/* unsupported */
#define RES_MEM_ALLOC		0x0008	/* memory allocation error */
#define RES_FCS_INCORRECT	0x0010	/* checksum incorrect */
#define RES_LEN_INVALID		0x0020	/* length invalid */
#define RES_NOT_FOUND		0x0040	/* not found */
#define RES_ACCESS_DND		0x0080	/* access denied */



#endif //_RESP_CODES_H_
