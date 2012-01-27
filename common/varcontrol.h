/*
 * varcontrol.h
 *
 *  Created on: 27.01.2012
 *      Author: alex
 */

#ifndef VARCONTROL_H_
#define VARCONTROL_H_

#define BOOKING		0x100
#define TRUEVALUE	0x080

typedef struct _VARTABLE{
	char syn[20];	// 'Synonym' of variable
	void *val;		// pointer to value
	void  *defval;	// pointer to default value (if exists)
	int typeid;		// type of variable
	int prop;		// properties: booking, true value.
	int time;		// time from last refresh value, sec
} vartable, *pvartable;

#endif /* VARCONTROL_H_ */
