/*
 * Copyright (C) 2011 by Grygorii Musiiaka                            
 * grymster@gmail.com
 * LLC "Mediakon"
 */

#ifndef _ASDU_H_
#define _ASDU_H_

#include <stdint.h>
#include "resp_codes.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Constants and byte flags/masks
 *
 */

/* Protocols */
#define PROTO_UNKNOWN		0
#define PROTO_IEC101		1
#define PROTO_IEC104		2
#define PROTO_DLT645		3
#define PROTO_M700			4

/* Value types */
#define ASDU_VAL_NONE		0		/* no value (used in ASDU like time synchronization, broadcast request, etc.) */
#define ASDU_VAL_INT		1		/* integer 32-bit */
#define ASDU_VAL_UINT		2		/* unsigned integer 32-bit */
#define ASDU_VAL_FLT		3		/* float */
#define ASDU_VAL_BOOL		4		/* boolean */
#define ASDU_VAL_TIME		5		/* time */

#define PROTO_ID			0
#define BASE_ID				1

#define DEC_BASE			1
#define HEX_BASE			2

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


/*
 *
 * Structures
 *
 */

/* data unit structure for universal ASDU */
typedef struct data_unit {
	uint32_t		id;					/* device's internal identifier of data unit */
	char			name[DOBJ_NAMESIZE];/* name of variable (protocol IEC61850) */

	union {
		uint32_t	ui;					/* unsigned integer representation */
		int32_t		i;					/* integer representation */
		float		f;					/* float representation */
	} value;							/* transferring value (e.g. measured value, counter readings, etc.) */

	int32_t			time_tag;			/* time tag in time_t representation */
	uint8_t			value_type; 		/* e.g. integer, unsigned integer, float, boolean, etc. (need this for "on fly" protocol converting) */
	uint8_t			attr;				/* value's additional attributes (e.g. quality descriptor, command qualifier, etc.) */
	uint8_t			time_iv;			/* invalid(1)/valid(0) */
	uint8_t			reserved;			/* padding */
} data_unit;


/* universal ASDU structure */
typedef struct asdu {
	/* Header */
	uint16_t		proto;				/* protocol identifier */
	uint16_t		adr;				/* ASDU address */
	uint8_t			type;				/* protocol specific ASDU type */
	uint8_t			fnc;				/* protocol specific command/function/cause/etc. */
	uint8_t			attr;				/* additional protocol specific ASDU attributes */

	/* Data */
	uint8_t			size;				/* data units array size */
	data_unit		*data;				/* data units array */
} asdu;


/* Identifiers mapping list item */
typedef struct asdu_map asdu_map;

struct asdu_map {
	uint32_t	proto_id;			/* protocol specific identifier */
	uint32_t	base_id;			/* base identifier */

	char		name[DOBJ_NAMESIZE];/* name of variable (protocol IEC61850) */

	asdu_map	*prev;				/* previous item in the mapping list */
	asdu_map	*next;				/* next item in the mapping list */
};


/*
 *
 * Functions
 *
 */

/* create asdu structure and initialize insides */
asdu *asdu_create();

/* cleanup asdu structure insides and destroy it */
void asdu_destroy(asdu **unit);

/* cleanup data array of asdu structure insides */
void asdu_cleanup_data(asdu *unit);

/* copies byte array to asdu in a correct way */
uint16_t asdu_from_byte(unsigned char *buff, uint32_t buff_len, asdu **unit);

/* copies asdu to byte array in a correct way */
uint16_t asdu_to_byte(unsigned char **buff, uint32_t *buff_len, asdu *unit);

/* protocol specific data identifiers mapping functions */
uint16_t  asdu_map_read(asdu_map **m_list, const char *file_name, const char *app_name, uint8_t num_base);
uint16_t  asdu_add_map_item(asdu_map **m_list, uint32_t proto_id, uint32_t base_id, const char *name, const char *app_name, uint8_t num_base);
asdu_map *asdu_get_map_item(asdu_map **m_list, uint32_t id, uint8_t get_by);
void      asdu_map_ids(asdu_map **m_list, asdu *cur_asdu, const char *app_name, uint8_t num_base);



#ifdef __cplusplus
}
#endif

#endif //_ASDU_H_
