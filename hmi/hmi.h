/*
 * hmi.h
 *
 *  Created on: 16.12.2011
 *      Author: dmitry
 */

#ifndef HMI_H_
#define HMI_H_

typedef struct _fact{
   char *action;
   void (*func)(void *arg);
}fact, *pfact;


#endif


