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

#define IEC104_K				12		/* maximum difference receive sequence number */
#define IEC104_W				8		/* latest acknowledge after receiving I-Format frame */

#define IEC104_APDU_LEN_MAX		253		/* default maximum APDU length for IEC-104 */



/* Identification Types */
#define M_SP_NA_1		1
#define M_SP_TA_1		2
#define M_DP_NA_1		3
#define M_DP_TA_1		4
#define M_ST_NA_1		5
#define M_ST_TA_1		6
#define M_BO_NA_1		7
#define M_BO_TA_1		8
#define M_ME_NA_1		9
#define M_ME_TA_1		10
#define M_ME_NB_1		11
#define M_ME_TB_1		12
#define M_ME_NC_1		13
#define M_ME_TC_1		14
#define M_IT_NA_1		15
#define M_IT_TA_1		16
#define M_EP_TA_1		17
#define M_EP_TB_1		18
#define M_EP_TC_1		19
#define M_PS_NA_1		20
#define M_ME_ND_1		21
#define M_SP_TB_1		30
#define M_DP_TB_1		31
#define M_ST_TB_1		32
#define M_BO_TB_1		33
#define M_ME_TD_1		34
#define M_ME_TE_1		35
#define M_ME_TF_1		36
#define M_IT_TB_1		37
#define M_EP_TD_1		38
#define M_EP_TE_1		39
#define M_EP_TF_1		40

#define C_SC_NA_1		45
#define C_DC_NA_1		46
#define C_RC_NA_1		47
#define C_SE_NA_1		48
#define C_SE_NB_1		49
#define C_SE_NC_1		50
#define C_BO_NA_1		51
#define C_SC_TA_1		58
#define C_DC_TA_1		59
#define C_RC_TA_1		60
#define C_SE_TA_1		61
#define C_SE_TB_1		62
#define C_SE_TC_1		63
#define C_BO_TA_1		64

#define M_IE_NA_1		70

#define C_IC_NA_1		100
#define C_CI_NA_1		101
#define C_RD_NA_1		102
#define C_CS_NA_1		103
#define C_TS_NA_1		104
#define C_RP_NA_1		105
#define C_CD_NA_1		106
#define C_TS_TA_1		107

#define P_ME_NA_1		110
#define P_ME_NB_1		111
#define P_ME_NC_1		112
#define P_AC_NA_1		113

#define F_FR_NA_1		120
#define F_SR_NA_1		121
#define F_SC_NA_1		122
#define F_LS_NA_1		123
#define F_AF_NA_1		124
#define F_DR_TA_1		125


/* Cause of Transmission*/
#define COT_Per_Cyc			1
#define COT_Back			2
#define COT_Spont			3
#define COT_Init			4
#define COT_Req				5

#define COT_Act				6
#define COT_ActCon			7
#define COT_Deact			8
#define COT_DeactCon		9
#define COT_ActTerm			10

#define COT_RetRem			11
#define COT_RetLoc			12

#define COT_File			13

#define COT_InroGen			20
#define COT_Inro1			21
#define COT_Inro2			22
#define COT_Inro3			23
#define COT_Inro4			24
#define COT_Inro5			25
#define COT_Inro6			26
#define COT_Inro7			27
#define COT_Inro8			28
#define COT_Inro9			29
#define COT_Inro10			30
#define COT_Inro11			31
#define COT_Inro12			32
#define COT_Inro13			33
#define COT_Inro14			34
#define COT_Inro15			35
#define COT_Inro16			36

#define COT_ReqCoGen		37
#define COT_ReqCo1			38
#define COT_ReqCo2			39
#define COT_ReqCo3			40
#define COT_ReqCo4			41

#define COT_UnkTypeId		44
#define COT_UnkCauseTx		45
#define COT_UnkComObjAdr	46
#define COT_UnkInfObjAdr	47


#endif //_IEC_DEF_H_
