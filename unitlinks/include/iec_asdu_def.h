/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */
 
#ifndef _IEC_ASDU_DEF_H_
#define _IEC_ASDU_DEF_H_


/*
 *
 * Constants and byte flags, masks
 *
 */

/* ASDU header masks */
#define IEC_ASDU_HEAD_NUM			0x7F
#define IEC_ASDU_HEAD_SQ			0x80
#define IEC_ASDU_HEAD_CAUSE			0x3F
#define IEC_ASDU_HEAD_PN			0x40
#define IEC_ASDU_HEAD_TEST			0x80


/* Quality descriptor */
#define IEC_ASDU_QDS_OV				0x01		/* overflow(1)/no overflow(0) */
#define IEC_ASDU_QDP_EI				0x08		/* invalid time interval(1)/valid time interval(0) */

#define IEC_ASDU_QDS_BL				0x10 		/* blocked(1)/not blocked(0) */
#define IEC_ASDU_QDS_SB				0x20		/* substituted(1)/not substituted(0) */
#define IEC_ASDU_QDS_NT				0x40		/* not topical(1)/topical(0) */
#define IEC_ASDU_QDS_IV				0x80		/* invalid(1)/valid(0) */

/* Command qualifier */
#define IEC_ASDU_QOC_QU				0x7C		/* qualifier-value */
#define IEC_ASDU_QOC_SE				0x80		/* select(1)/execute(0) */

/* Set point qualifier */
#define IEC_ASDU_QOS_QL				0x7F		/* qualifier-value  */
#define IEC_ASDU_QOS_SE				0x80		/* select(1)/execute(0) */

/* Parameter qualifier */
#define IEC_ASDU_QPM_KPA			0x3F		/* parameter type */
#define IEC_ASDU_QPM_LPC			0x40		/* local parameter change - change(1)/no change(0) */
#define IEC_ASDU_QPM_POP			0x80		/* not in use(1)/in use(0) */

/* Single point information (M_SP_) */
#define IEC_ASDU_SIQ_SPI			0x01		/* single point information */
#define IEC_ASDU_SIQ_RES			0x0E		/* reserved */

/* Double point information (M_DP_) */
#define IEC_ASDU_DIQ_DPI			0x03		/* double point information */
#define IEC_ASDU_DIQ_RES			0x0C		/* reserved */

/* Transient state indication (M_ST_) */
#define IEC_ASDU_VTI_VAL			0x7F		/* transient state indication value */
#define IEC_ASDU_VTI_TS				0x80		/* in transient state(1)/not in transient state(0) */

/* Integrated totals (M_IT_) */
#define IEC_ASDU_BCR_SQ				0x1F		/* sequence number */
#define IEC_ASDU_BCR_CY				0x20		/* counter wrapped(1)/counter not wrapped (0) */
#define IEC_ASDU_BCR_CA				0x40		/* counter set(1)/counter not set(0) */
#define IEC_ASDU_BCR_IV				0x80		/* invalid(1)/valid(0) */

/* Single event of protection equipment (M_EP_) */
#define IEC_ASDU_SEP_ES				0x03		/* protection equipment state */
#define IEC_ASDU_SEP_RES			0x04		/* reserved */

/* Packed event information of starter protection equipment (M_EP_) */
#define IEC_ASDU_SPE_GS				0x01		/* common job started(1)/ common job not started(0) */
#define IEC_ASDU_SPE_SL1			0x02		/* phase A job started(1)/ phase A job not started(0) */
#define IEC_ASDU_SPE_SL2			0x04		/* phase B job started(1)/ phase B job not started(0) */
#define IEC_ASDU_SPE_SL3			0x08		/* phase C job started(1)/ phase C job not started(0) */
#define IEC_ASDU_SPE_SIE			0x10		/* IE (ground current) job started(1)/ IE (ground current) job not started(0) */
#define IEC_ASDU_SPE_SRD			0x20		/* reverse direction job started(1)/ reverse direction not started(0) */
#define IEC_ASDU_SPE_RES			0xC0		/* reserved */

/* Packed event information of protection equipment output circuits triggering (M_EP_) */
#define IEC_ASDU_OCI_GS				0x01		/* common command triggered(1)/ common command not triggered(0) */
#define IEC_ASDU_OCI_CL1			0x02		/* phase A command triggered(1)/ phase A command not triggered(0) */
#define IEC_ASDU_OCI_CL2			0x04		/* phase B command triggered(1)/ phase B command not triggered(0) */
#define IEC_ASDU_OCI_CL3			0x08		/* phase C command triggered(1)/ phase C command not triggered(0) */
#define IEC_ASDU_OCI_RES			0xF0		/* reserved */

/* Single point command (C_SC_) */
#define IEC_ASDU_SCO_SCS			0x01		/* single point command - on(1)/off(0) */

/* Double point command (C_DC_) */
#define IEC_ASDU_DCO_DCS			0x03		/* double point command - on(2)/off(1) */

/* Regulating step command (C_RC_) */
#define IEC_ASDU_RCO_RCS			0x03		/* regulating step command - step down(1)/step up(2) */


/* Counter interrogation command (C_CI_) */
#define IEC_ASDU_QCC_RQT			0x3F		/* request */
#define IEC_ASDU_QCC_FRZ			0xC0		/* freeze */

/* File ready (F_FR_) */
#define IEC_ASDU_FRQ_FR				0x7F		/* default (0) */
#define IEC_ASDU_FRQ_ACK			0x80		/* negative acknowledgment(1)/positive acknowledgment (0) */

/* Section ready (F_SR_) */
#define IEC_ASDU_SRQ_SR				0x7F		/* default (0) */
#define IEC_ASDU_SRQ_ACK			0x80		/* section not ready(1)/section ready (0) */

/* Call directory, select file, call file, call section (F_SC_) */
#define IEC_ASDU_SCQ_SC				0x0F		/* call, selection */
#define IEC_ASDU_SCQ_RSP			0xF0		/* response */

/* Acknowledgment file, acknowledgment section (F_AF_) */
#define IEC_ASDU_AFQ_ACK			0x0F		/* acknowledgment */
#define IEC_ASDU_AFQ_RSP			0xF0		/* response */

/* File state (F_DR_) */
#define IEC_ASDU_SOF_STS			0x1F		/* status */
#define IEC_ASDU_SOF_LFD			0x20		/* last file(1)/not last file(0) */
#define IEC_ASDU_SOF_FOR			0x40		/* directory(1)/file(0) */
#define IEC_ASDU_SOF_FA				0x80		/* file transferring(1)/waiting for transfer(0) */

/* Fixed test combination */
#define IEC_ASDU_FSP_CMB			0x55AA		/* fixed test combination */


#endif //_IEC_ASDU_DEF_H_
