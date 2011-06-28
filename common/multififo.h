/*
 * multififo.h
 *
 *  Created on: 10.06.2011
 *      Author: Alex AVAlon
 */

#ifndef MULTIFIFO_H_
#define MULTIFIFO_H_

#define DIRDN		0x57
#define DIRUP		0x58

// ================= External API =========================================
// Initialisation
// in: pathinit - pointer to path to pipes
// in: devicename - pointer to devicename from config
//extern int mf_init(char *pathinit, char *a_name);
extern pid_t mf_init(char *pathinit, char *a_name, void *func_rcvdata, void *func_rcvinit);
// Exit from inotify waiting
extern void mf_exit(void);
// Create new endpoint in downlink application only
extern int mf_newendpoint (struct config_device *cd, char *pathinit, u32 ep_num);
// Send data to endpoint in any direction (up-down)
extern int mf_toendpoint(char *buf, int len, int addr, int direct);
// Read data from ring buffer (for use in callback function)
extern int mf_readbuffer(char *buf, int len);

#endif /* MULTIFIFO_H_ */
