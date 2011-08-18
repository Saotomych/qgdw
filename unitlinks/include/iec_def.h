/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _IEC_DEF_H_
#define _IEC_DEF_H_

/* Common IEC constants */
#define IEC_HOST_MASTER			1		/* master (controlling) station */
#define IEC_HOST_SLAVE			2		/* slave (controlled) station */

/* IEC-101 */
#define IEC101_POINT_TO_POINT	1		/* point to point network */
#define IEC101_MULTI_TO_POINT	2		/* multiple-point to point network */
#define IEC101_PARTY_LINE		3		/* party line network */
#define IEC101_REDUNDANT_LINE	4		/* redundant line network */


#define IEC101_BALANCED			1		/* balanced mode */
#define IEC101_UNBALANCED		2		/* unbalanced mode */

/* default constants values */
#define IEC101_COT_LEN			1		/* cause of transmission default length */
#define IEC101_COA_LEN			1		/* common object (ASDU) address default length */
#define IEC101_IOA_LEN			2		/* information object address default length */

#define IEC101_LPDU_LEN_MAX		255		/* default maximum LPDU length for IEC-101 */


/* IEC-104 */
/* default constants values */
#define IEC104_COT_LEN			2		/* cause of transmission length */
#define IEC104_COA_LEN			2		/* common object (ASDU) address length */
#define IEC104_IOA_LEN			3		/* information object address length */

#define IEC104_T_T0				30		/* timeout of connection establishment */
#define IEC104_T_T1				15		/* timeout of send or test APDUs */
#define IEC104_T_T2				10		/* timeout for acknowledges in case of no data messages (t2<t1) */
#define IEC104_T_T3				20		/* timeout for sending test frames in case of long idle state */

#define IEC104_K				12		/* latest acknowledgment after sending I-Format frame default value */
#define IEC104_W				8		/* latest acknowledgment after receiving I-Format frame default value  */

#define IEC104_APDU_LEN_MAX		253		/* default maximum APDU length for IEC-104 */



#endif //_IEC_DEF_H_
