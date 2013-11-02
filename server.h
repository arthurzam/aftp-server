/*
 * server.h
 *
 *  Created on: 23 ???? 2013
 *      Author: Arthur Zamarin
 */

#ifndef SERVER_H_
#define SERVER_H_

#include "defenitions.h"

#define DEFAULT_PORT 7777

THREAD_RETURN_VALUE startServer(void*);
int sendMessage(struct sockaddr_in* to, short msgCode, char* data, int datalen);


#endif /* SERVER_H_ */
