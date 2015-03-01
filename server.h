#ifndef SERVER_H_
#define SERVER_H_

#include "defenitions.h"
#include <openssl/aes.h>
#include <openssl/rsa.h>

class LoginDB;
class UserList;

struct rsa_control_t {
    RSA* privateKey;
    char* publicKey;
    size_t publicKeyLength;
};

extern "C" void startServer(LoginDB* usersDB, UserList* listUsers, const rsa_control_t& rsaControl);
extern "C" int sendMessage(const struct sockaddr_in* to, msgCode_t msgCode, const void* data, size_t datalen, const AES_KEY* encryptKey);

#endif /* SERVER_H_ */
