#ifndef SERVER_H_
#define SERVER_H_

#include "defenitions.h"

constexpr unsigned BUFFER_SERVER_SIZE = 0x400; // = 1024

int sendMessage(const struct sockaddr_in* to, uint16_t msgCode, const void* data, size_t datalen);

#endif /* SERVER_H_ */
