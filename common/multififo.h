/*
 * multififo.h
 *
 *  Created on: 10.06.2011
 *      Author: Alex AVAlon
 */

#ifndef MULTIFIFO_H_
#define MULTIFIFO_H_

#define DIRUP		0x58
#define DIRDN		0x59

#define MAXEP		64
#define MAXCH		64

// ================= External API =========================================
// this structure added as one parameter for  mf_toendpoint and mf_readbuffer
typedef struct _transactinfo{
	char *buf;
	int len;
	int addr;
	int direct;
	int ep_index;
} TRANSACTINFO;

extern volatile int hpp[2];

// Initialisation
// in: pathinit - pointer to path to pipes
// in: devicename - pointer to devicename from config
extern pid_t mf_init(char *pathinit, char *a_name, void *func_rcvdata, void *func_rcvinit);
//
extern void mf_set_cb_rcvclose(void *func_rcvclose);
// Exit from inotify waiting
extern void mf_exit(void);
// Create new endpoint in downlink application only
extern int mf_newendpoint (struct config_device *cd, char *pathinit, u32 ep_num);
// Send data to endpoint in any direction (up-down)
extern int mftai_toendpoint(TRANSACTINFO *tai);
extern int mf_toendpoint_by_index(char *buf, int len, int index, int direct);
extern int mf_toendpoint(char *buf, int len, int addr, int direct);

// Read data from ring buffer (for use in callback function)
extern int mftai_readbuffer(TRANSACTINFO *tai);
extern int mf_readbuffer_by_index(char *buf, int len, int *index, int *direct);
extern int mf_readbuffer(char *buf, int len, int *addr, int *direct);

#endif /* MULTIFIFO_H_ */
