/*
 * multififo.h
 *
 *  Created on: 10.06.2011
 *      Author: Alex AVAlon
 */

#ifndef MULTIFIFO_H_
#define MULTIFIFO_H_

// ================= External API =========================================
// Initialisation
// in: pathinit - pointer to path to pipes
// in: devicename - pointer to devicename from config
//extern int mf_init(char *pathinit, char *a_name);
extern pid_t mf_init(char *pathinit, char *a_name, void *func_rcvdata, void *func_rcvinit);
// Exit from inotify waiting
extern void mf_exit(void);
// Create new endpoint in downlink application only
extern int mf_newendpoint (struct config_device *cd, char *pathinit);
// Send data to endpoint in any direction (up-down)
extern int mf_toendpoint(struct config_device *cd, char *buf, int len);
// Read data from ring buffer (for use in callback function)
extern int mf_readbuffer(char *buf, int len);

#endif /* MULTIFIFO_H_ */
