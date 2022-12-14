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

#define MAXEP		128
#define MAXCH		32

#define FDSETPOS	10

// ================= External API =========================================
// this structure added as one parameter for  mf_toendpoint and mf_readbuffer
typedef struct _transactinfo{
	char *buf;
	int len;
	int addr;
	int direct;
	int ep_index;
} TRANSACTINFO;

// Delete semaphore file
extern void mf_semadelete(char *pathinit, char *a_name);
// Initialisation
// in: pathinit - pointer to path to pipes
// in: devicename - pointer to devicename from config
extern pid_t mf_init(char *pathinit, char *a_name, void *func_rcvdata);
//
//extern void mf_set_cb_rcvclose(void *func_rcvclose);
// Exit from inotify waiting
extern void mf_exit(void);
// Create new endpoint in downlink application only
extern int mf_newendpoint (u32 addr, char* chld_name, char *pathinit, u32 ep_num);
// Send data to endpoint in any direction (up-down)
extern int mftai_toendpoint(TRANSACTINFO *tai);
extern int mf_toendpoint_by_index(char *buf, int len, int index, int direct);
extern int mf_toendpoint(char *buf, int len, int addr, int direct);

// Read data from ring buffer (for use in callback function)
extern int mftai_readbuffer(TRANSACTINFO *tai);
extern int mf_readbuffer_by_index(char *buf, int len, int *index, int *direct);
extern int mf_readbuffer(char *buf, int len, int *addr, int *direct);
extern void mf_addctrlheader(int addfd);
extern int mf_waitevent(char *buf, int len, int ms_delay);
extern int mf_testrunningapp(char *name);

#endif /* MULTIFIFO_H_ */
