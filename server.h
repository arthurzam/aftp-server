/*
 * server.h
 *
 *  Created on: 23 ???? 2013
 *      Author: Arthur Zamarin
 */

#ifndef SERVER_H_
#define SERVER_H_

#include "defenitions.h"

#define BUFFER_SERVER_SIZE 0x400 // = 1024
#define DEFAULT_PORT 7777

threadReturnValue startServer(void*);
int sendMessage(const struct sockaddr_in* to, uint16_t msgCode, const void* data, int datalen);


#endif /* SERVER_H_ */
