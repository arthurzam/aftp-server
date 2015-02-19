#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <new>
#include <thread>
#include <mutex>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "fileControl.h"
#include "UserList.h"
#include "User.h"
#include "LoginDB.h"

#include "messages.h"
#include "server_lookup.h"

SOCKET sock;

extern bool needExit;
extern uint16_t port;

struct __attribute__((packed)) client_msg_plain_t{
    msgCode_t msgCode;
    union __attribute__((packed)){
        char data[BUFFER_SERVER_SIZE - sizeof(msgCode)];
        struct __attribute__((packed)){
            uint8_t md5Password[MD5_DIGEST_LENGTH];
            char username[sizeof(data) - MD5_DIGEST_LENGTH];
        } login;
        struct __attribute__((packed)){
            uint32_t start;
            uint32_t end;
        } block_range;
        uint32_t block;
        struct __attribute__((packed)){
            uint32_t blocksCount;
            char path[sizeof(data) - sizeof(blocksCount)];
        } upload;
    } u;
    char nullTerminate[8] = {0};
};

enum EXIT_STATUS {
    DONT_MANAGE = 0,
    BAD,
    GOOD,
};

constexpr char* INFO_STRING = (char*)"AFTP Server v2.1 made by Arthur Zamarin, 2015";
constexpr size_t INFO_STRING_LEN = strlen(INFO_STRING);

static bool initServer()
{
    struct sockaddr_in server;

#ifdef WIN32
    WSADATA wsaData;
    int retval;
    if ((retval = WSAStartup(0x202, &wsaData)) != 0)
    {
        fprintf(stderr,"[Server: WSAStartup() failed with error %d]\n", retval);
        return (false);
    }
#endif

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    sock = socket(AF_INET, SOCK_DGRAM, 0);

#ifdef WIN32
    if (sock == INVALID_SOCKET)
    {
        fprintf(stderr,"[Server: socket() failed with error %d]\n", WSAGetLastError());
        return (false);
    }
#else
    if (sock < 0)
    {
        fprintf(stderr,"[Server: socket() failed]\n");
        return (false);
    }
#endif

#ifdef WIN32
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        fprintf(stderr,"[Server: bind() failed with error %d]\n", WSAGetLastError());
        return (false);
    }
#else
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        fprintf(stderr,"[Server: bind() failed]\n");
        return (false);
    }
#endif
    return true;
}

static void userControl(UserList* listUsers)
{
    static constexpr unsigned WAIT_SECONDS = 50;
    int i;
    while (!needExit)
    {
        for(i = WAIT_SECONDS; !needExit && i != 0; --i)
            SLEEP(1);
        if(!needExit)
            listUsers->userControl();
    }
}

static int decryptAES(const AES_KEY* decryptKey, const void* src, size_t src_len, void* dst)
{
    uint8_t iv[16] = {0};
    uint8_t lastBlockLen = *((uint8_t*)src);
    uint8_t* dPtr = (uint8_t*)dst;
    const uint8_t* sPtr = (const uint8_t*)src + 1;

    --src_len;

    while(src_len >= 256)
    {
        AES_cbc_encrypt(sPtr, dPtr, 256, decryptKey, iv, AES_DECRYPT);
        sPtr += 256;
        dPtr += 256;
        src_len -= 256;
    }
    if(src_len != 0)
    {
        AES_cbc_encrypt(sPtr, dPtr, src_len, decryptKey, iv, AES_DECRYPT);
        dPtr += lastBlockLen;
    }
    return (dPtr - (uint8_t*)dst);
}

static bool decryptAESkeyRSA(const RSA* rsa, const void* src, int src_len, uint8_t dst[AES_KEY_LENGTH / 8])
{
    return (RSA_private_decrypt(src_len, (uint8_t*)src, (uint8_t*)dst, const_cast<RSA*>(rsa), RSA_PKCS1_OAEP_PADDING) != -1);
}

extern "C" void startServer(LoginDB* usersDB, UserList* listUsers, const rsa_control_t& rsaControl)
{
    struct __attribute__((packed)){
        uint16_t msgCode;
        uint8_t padLength;
        char data[BUFFER_SERVER_SIZE - sizeof(msgCode) - sizeof(padLength)];
    } encrypted_msg;
    client_msg_plain_t decryptedBuffer;

    const uint16_t exit_msg_table[] = {0, SERVER_MSG::ERROR_OCCURED, SERVER_MSG::ACTION_COMPLETED};

    register client_msg_plain_t* Buffer;

    static_assert(sizeof(decryptedBuffer) - sizeof(Buffer->nullTerminate) == BUFFER_SERVER_SIZE, "bad buffer size");

    unsigned retval;
    User* user;
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    union {
        const Login* loginClass;
        int i;
        uint64_t l;
        uint8_t aes_key[AES_KEY_LENGTH / 8];
    } tempData;

    if(!initServer())
    {
#ifdef WIN32
        WSACleanup();
#endif
        return;
    }

    IOThreadPool ioThreadPool;
    std::thread userControlThread(userControl, listUsers);
    while(!needExit)
    {
        EXIT_STATUS status = EXIT_STATUS::DONT_MANAGE;
        retval = recvfrom(sock, (char*)&encrypted_msg, BUFFER_SERVER_SIZE, 0, (struct sockaddr *)&from, &fromlen);
        if (retval < sizeof(msgCode_t)) continue;
        ((char*)&encrypted_msg)[retval] = '\0';
        encrypted_msg.msgCode = ntohs(encrypted_msg.msgCode);

        if((user = listUsers->findUser(from)))
            user->resetTime();
        else
            user = listUsers->addUser(from);

        if(user->isEncryption())
        {
            retval = sizeof(msgCode_t) + decryptAES(user->getDecryptKey(), &encrypted_msg, retval - 2, decryptedBuffer.u.data); //FIXME: check
            Buffer = &decryptedBuffer;
            decryptedBuffer.msgCode = encrypted_msg.msgCode;
        }
        else
        {
            Buffer = (client_msg_plain_t*)&encrypted_msg;
        }

        if(table_msgs[Buffer->msgCode].min_ret > retval) // too short data
        {
            user->sendData(SERVER_MSG::ERROR_OCCURED);
            continue;
        }
        if(Buffer->msgCode == CLIENT_MSG::SET_AES) // set aes encryption
        {
            if(decryptAESkeyRSA(rsaControl.privateKey, Buffer->u.data, retval - sizeof(msgCode_t), tempData.aes_key))
            {
                user->setAesKey(tempData.aes_key);
                status = EXIT_STATUS::GOOD;
            }
            else
            {
                status = EXIT_STATUS::BAD;
            }
            continue;
        }
        if(Buffer->msgCode != CLIENT_MSG::LOGIN && !user->isLoged()) // not logged in
        {
            user->sendData(SERVER_MSG::LOGIN_REQUIRED);
            continue;
        }
        if(Buffer->msgCode > CLIENT_MSG_COUNT) // bad msgCode
        {
            user->sendData(SERVER_MSG::UNKNOWN_COMMAND);
            continue;
        }
        if(table_msgs[Buffer->msgCode].num != 0) // IO path function
        {
            ioThreadPool.addPathFunction(table_msgs[Buffer->msgCode].num, Buffer->u.data, user, table_msgs[Buffer->msgCode].function);
            continue;
        }
        switch(Buffer->msgCode)
        {
            case CLIENT_MSG::LOGIN:
                if((tempData.loginClass = usersDB->check(Buffer->u.login.username, Buffer->u.login.md5Password)))
                {
                    user->logIn(tempData.loginClass);
                    status = EXIT_STATUS::GOOD;
                }
                else
                    user->sendData(SERVER_MSG::LOGIN_BAD);
                break;
            case CLIENT_MSG::LOGOUT:
                listUsers->removeUser(user);
                status = EXIT_STATUS::GOOD;
                break;
            case CLIENT_MSG::EMPTY_MESSAGE:
                status = EXIT_STATUS::GOOD;
                break;
            case CLIENT_MSG::FILE_BLOCK:
                if(!user->fileTransfer)
                    status = EXIT_STATUS::BAD;
                else
                    user->fileTransfer.recieveBlock(Buffer->u.data, retval - sizeof(msgCode_t));
                break;
            case CLIENT_MSG::ASK_BLOCK_RANGE:
                if(!user->fileTransfer)
                    status = EXIT_STATUS::BAD;
                else
                    user->fileTransfer.askForBlocksRange(ntohl(Buffer->u.block_range.start), ntohl(Buffer->u.block_range.end));
                break;
            case CLIENT_MSG::ASK_BLOCK:
                if(!user->fileTransfer)
                    status = EXIT_STATUS::BAD;
                else
                    user->fileTransfer.askForBlock(ntohl(Buffer->u.block));
                break;
            case CLIENT_MSG::END_FILE_TRANSFER:
                if(!user->fileTransfer)
                    status = EXIT_STATUS::BAD;
                else
                {
                    user->fileTransfer.close();
                    status = EXIT_STATUS::GOOD;
                }
                break;
            case CLIENT_MSG::CLIENT_INFO:
            case CLIENT_MSG::SERVER_INFO:
                user->sendData(SERVER_MSG::INFO_SERVER, INFO_STRING, INFO_STRING_LEN);
                break;
            case CLIENT_MSG::FILE_UPLOAD:
                status = (EXIT_STATUS)(1 + user->fileTransfer.reset(Buffer->u.upload.path, user, ntohl(Buffer->u.upload.blocksCount)));
                break;
            case CLIENT_MSG::FILE_DOWNLOAD:
                if(user->fileTransfer.reset(Buffer->u.data, user))
                {
                    tempData.i = htonl(user->fileTransfer.getBlocksCount());
                    user->sendData(SERVER_MSG::ACTION_COMPLETED, &tempData.i, 4);
                }
                else
                    status = EXIT_STATUS::BAD;
                break;
            case CLIENT_MSG::DIR_CD:
                status = (EXIT_STATUS)(1 + user->moveFolder(Buffer->u.data));
                break;
            case CLIENT_MSG::DIR_PWD:
                if(user->folderPath()[0])
                    user->sendData(SERVER_MSG::ACTION_COMPLETED, user->folderPath(), strlen(user->folderPath()));
                else
                    user->sendData(SERVER_MSG::ACTION_COMPLETED, (char*)"/", 1);
                break;
            case CLIENT_MSG::ASK_RSA:
                user->sendData(SERVER_MSG::SERVER_RSA_KEY, rsaControl.publicKey, rsaControl.publicKeyLength);
                break;
            case CLIENT_MSG::STOP_ENCRYPTION:
                user->stopEncryption();
                status = EXIT_STATUS::GOOD;
        }
        if(status != EXIT_STATUS::DONT_MANAGE)
        {
            user->sendData(exit_msg_table[status]);
        }
    }
#ifdef WIN32
    WSACleanup();
#endif
    userControlThread.join();
}

static int encryptAES(const AES_KEY* encryptKey, const void* src, size_t src_len, void* dst)
{
    uint8_t iv[16] = {0};
    uint8_t* dstLen = (uint8_t*)dst;
    uint8_t* dPtr = (uint8_t*)dst + 1;
    const uint8_t* sPtr = (const uint8_t*)src;

    while(src_len >= 256)
    {
        AES_cbc_encrypt(sPtr, dPtr, 256, encryptKey, iv, AES_ENCRYPT);
        sPtr += 256;
        dPtr += 256;
        src_len -= 256;
    }
    if(src_len != 0)
    {
        AES_cbc_encrypt(sPtr, dPtr, src_len, encryptKey, iv, AES_ENCRYPT);
        dPtr += ((src_len & 0xFFFFFFF0) + ((src_len & 0x0F) ? 16 : 0)); // calculates the pad size
    }
    *dstLen = (uint8_t)src_len;
    return (dPtr - (uint8_t*)dst);
}

extern "C" int sendMessage(const struct sockaddr_in* to, uint16_t msgCode, const void* data, size_t datalen, const AES_KEY* encryptKey)
{
    static std::mutex lockSend;

    struct __attribute__((packed)){
        msgCode_t msgCode;
        char data[BUFFER_SERVER_SIZE - sizeof(msgCode)];
        char nullTerminate[8];
    } buffer;

    buffer.msgCode = htons(msgCode);
    if((data != nullptr) & (datalen != 0))
    {
        if(datalen > sizeof(buffer.data))
            datalen = sizeof(buffer.data);

        if(encryptKey)
            datalen = encryptAES(encryptKey, data, datalen, buffer.data);
        else
            memcpy(buffer.data, data, datalen);
    }
    else
        datalen = 0;

    lockSend.lock();
    int retVal = sendto(sock, (char*)&buffer, datalen + sizeof(msgCode), 0, (struct sockaddr *)to, sizeof(struct sockaddr_in));
    lockSend.unlock();
    return (retVal);
}
