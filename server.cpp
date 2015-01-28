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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#endif

#include "server.h"
#include "fileControl.h"
#include "UserList.h"
#include "User.h"
#include "LoginDB.h"

#include "messages.h"
#include "server_lookup.h"

SOCKET sock;

extern bool needExit;
extern uint16_t port;

void userControl(UserList* listUsers);

void startServer(LoginDB* usersDB, UserList* listUsers)
{
    char Buffer[BUFFER_SERVER_SIZE];
    int retval;
    User* user;
    struct sockaddr_in server;
    struct sockaddr_in from;
    uint16_t msgCode;
    socklen_t fromlen = sizeof(from);
    std::thread userControlThread;
    IOThreadPool ioThreadPool;

    union {
        struct {
            char* src;
            char* dst;
            uint8_t src_len;
            uint8_t dst_len;
        } src_dst;
        char* path;
        const Login* loginClass;
        int i;
        uint64_t l;
        uint8_t md5[MD5_DIGEST_LENGTH];
        struct {
            char* username;
            uint8_t* md5Password;
        } login;
    } tempData;


#ifdef WIN32
    WSADATA wsaData;
    if ((retval = WSAStartup(0x202, &wsaData)) != 0)
    {
        fprintf(stderr,"[Server: WSAStartup() failed with error %d]\n", retval);
        goto _errorExit;
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
        goto _errorExit;
    }
#else
    if (sock < 0)
    {
        fprintf(stderr,"[Server: socket() failed]\n");
        goto _errorExit;
    }
#endif

#ifdef WIN32
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        fprintf(stderr,"[Server: bind() failed with error %d]\n", WSAGetLastError());
        goto _errorExit;
    }
#else
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        fprintf(stderr,"[Server: bind() failed]\n");
        goto _errorExit;
    }
#endif

    userControlThread = std::thread(userControl, listUsers);
    while(!needExit)
    {
        retval = recvfrom(sock, Buffer, sizeof(Buffer), 0, (struct sockaddr *)&from, &fromlen);
        if (retval < 2) continue;
        Buffer[retval] = 0;
        memcpy(&msgCode, Buffer, sizeof(msgCode));
        msgCode = ntohs(msgCode);

        if((user = listUsers->findUser(from)))
            user->resetTime();
        else
            user = listUsers->addUser(from);

        if(msgCode != CLIENT_MSG::LOGIN && !user->isLoged())
        {
            sendMessage(&from, SERVER_MSG::LOGIN_REQUIRED, NULL, 0);
            continue;
        }
        else if(msgCode > CLIENT_MSG_COUNT)
        {
            sendMessage(&from, SERVER_MSG::UNKNOWN_COMMAND, NULL, 0);
            continue;
        }
        switch(table_msgs[msgCode].num)
        {
            case 0:
                switch(msgCode)
                {
                    case CLIENT_MSG::LOGIN: // login
                        tempData.login.username = Buffer + 18;
                        tempData.login.md5Password = (uint8_t*)(Buffer + 2);
                        if((tempData.loginClass = usersDB->check(tempData.login.username, tempData.login.md5Password)))
                        {
                            user->logIn(tempData.loginClass);
                            sendMessage(&from, SERVER_MSG::LOGIN_SUCCESS, NULL, 0);
                        }
                        else
                        {
                            sendMessage(&from, SERVER_MSG::LOGIN_BAD, NULL, 0);
                        }
                        break;
                    case CLIENT_MSG::LOGOUT: // logout
                        sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, NULL, 0);
                        listUsers->removeUser(user);
                        break;
                    case CLIENT_MSG::EMPTY_MESSAGE:
                        sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, NULL, 0);
                        break;
                    case CLIENT_MSG::FILE_BLOCK: // block in File Upload
                        if(!user->fileTransfer)
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        else
                            user->fileTransfer->recieveBlock(Buffer + 2);
                        break;
                    case CLIENT_MSG::ASK_BLOCK_RANGE: // ask for block range
                        if(!user->fileTransfer)
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        else
                            user->fileTransfer->askForBlocksRange(*((uint32_t*)(Buffer + 2)), *((uint32_t*)(Buffer + 6)));
                        break;
                    case CLIENT_MSG::ASK_BLOCK: // ask for block
                        if(!user->fileTransfer)
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        else
                            user->fileTransfer->askForBlock(*((uint32_t*)(Buffer + 2)));
                        break;
                    case CLIENT_MSG::END_FILE_TRANSFER: // finish file transfer
                        if(!user->fileTransfer)
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        else
                        {
                            delete user->fileTransfer;
                            user->fileTransfer = NULL;
                            sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, NULL, 0);
                        }
                        break;
                    case CLIENT_MSG::CLIENT_INFO:
                    case CLIENT_MSG::SERVER_INFO: // info
                        sendMessage(&from, SERVER_MSG::INFO_SERVER, (char*)"AFTP Server v1.0 made by Arthur Zamarin, 2014", 46);
                        break;
                    case CLIENT_MSG::FILE_UPLOAD: // upload
                        if(user->fileTransfer)
                            delete user->fileTransfer;
                        user->fileTransfer = new (std::nothrow) FileTransfer(Buffer + 6, user, ntohl(*(uint32_t*)(Buffer + 2)));
                        if(user->fileTransfer && user->fileTransfer->isLoaded())
                            sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, NULL, 0);
                        else
                        {
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                            delete user->fileTransfer;
                            user->fileTransfer = NULL;
                        }
                        break;
                    case CLIENT_MSG::FILE_DOWNLOAD: // download
                        if(user->fileTransfer)
                            delete user->fileTransfer;
                        user->fileTransfer = new (std::nothrow) FileTransfer(Buffer + 2, user);
                        if(user->fileTransfer && user->fileTransfer->isLoaded())
                        {
                            tempData.i = htonl(user->fileTransfer->getBlocksCount());
                            sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, &tempData.i, 4);
                        }
                        else
                        {
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                            delete user->fileTransfer;
                            user->fileTransfer = NULL;
                        }
                        break;
                    case CLIENT_MSG::DIR_CD: // cd
                        if(user->moveFolder(Buffer + 2))
                            sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, NULL, 0);
                        else
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        break;
                    case CLIENT_MSG::DIR_PWD: // pwd
                        if(user->folderPath()[0])
                            sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, user->folderPath(), strlen(user->folderPath()));
                        else
                            sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, (char*)"/", 1);
                        break;
                }
                break;
            case 1:
                ioThreadPool.add1pathFunction(Buffer + 2, user, table_msgs[msgCode].function);
                break;
            case 2:
                ioThreadPool.add2pathFunction(Buffer + 2, user, table_msgs[msgCode].function);
                break;
        }

    }
_errorExit:
#ifdef WIN32
    WSACleanup();
#endif
    userControlThread.join();
}

int sendMessage(const struct sockaddr_in* to, uint16_t msgCode, const void* data, int datalen)
{
    static std::mutex lockSend;

    char buffer[BUFFER_SERVER_SIZE + 5];
    msgCode = htons(msgCode);
    memcpy(buffer, &msgCode, sizeof(msgCode));
    if(datalen > BUFFER_SERVER_SIZE)
        datalen = BUFFER_SERVER_SIZE;
    if(data && datalen > 0)
        memcpy(buffer + sizeof(msgCode), data, datalen);

    lockSend.lock();
    int retVal = sendto(sock, buffer, datalen + sizeof(msgCode), 0, (struct sockaddr *)to, sizeof(struct sockaddr_in));
    lockSend.unlock();
    return (retVal);
}

void userControl(UserList* listUsers)
{
    constexpr unsigned WAIT_SECONDS = 50;
	int i;
    while (!needExit)
    {
        for(i = WAIT_SECONDS; !needExit && i != 0; ++i)
#ifdef WIN32
            Sleep(1000); //mili-seconds
#else
            sleep(1);    // seconds
#endif
        if(!needExit)
        	listUsers->userControl();
    }
}
