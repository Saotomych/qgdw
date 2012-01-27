/*
 * varcontrol.c
 *
 *  Created on: 27.01.2012
 *      Author: Alex AVAlon
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nano-X.h"
#include "nanowm.h"
#include <signal.h>
#include <string.h>
#include "menu.h"
#include "hmi.h"
#include "varcontrol.h"

#include <sys/types.h>
#include <sys/stat.h>

vartable defvt[] = {
		{"mac", NULL, NULL, 0},
		{"ip", NULL, NULL, 0},
};

vartable *actvt[20];	// Max 20 variables may be book from applications

// Set callback function for variable changing events
void vc_setcallback(){

}

// Make memory allocation for all variables, read values and set pointers
void vc_init(pvartable vt, int len){

	// Def Table init
		// Parse full config files

		// Bring to conformity with all internal variables and config variables
		// It's equal constant booking

	// Multififo init


}

// To book concrete variable by synonym
int vc_book(char *varsynonim){

	return 0;
}

// To delete booking of concrete variable by synonym
int vc_unbook(char *varsynonim){

	return 0;
}
