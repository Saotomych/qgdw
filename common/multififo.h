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
extern int mf_init(char *pathinit, char *devicename);
// Exit from inotify waiting
extern void mf_exit(void);
// Create new endpoint in downlink application only
extern int mf_newendpoint(struct config_device *cd);
// Send data to endpoint in any direction (up-down)
extern int mf_toendpoint(struct config_device *cd, char *buf, int len);

// Data Receiving
// Signal registration in main program: signal (SIGPOLL, poll_handler)
// in: len - data length in buffer
extern void poll_handler(int len);

#endif /* MULTIFIFO_H_ */
