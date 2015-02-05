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

static void userControl(UserList* listUsers);

void startServer(LoginDB* usersDB, UserList* listUsers)
{
    struct __attribute__((packed)){
        uint16_t msgCode;
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
    } Buffer;

    static_assert(sizeof(Buffer) - sizeof(Buffer.nullTerminate) == BUFFER_SERVER_SIZE, "bad buffer size");

    int retval;
    User* user;
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    union {
        const Login* loginClass;
        int i;
        uint64_t l;
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
        retval = recvfrom(sock, (char*)&Buffer, BUFFER_SERVER_SIZE, 0, (struct sockaddr *)&from, &fromlen);
        if (retval < 2) continue;
        Buffer.u.data[retval - 2] = 0;
        Buffer.msgCode = ntohs(Buffer.msgCode);

        if((user = listUsers->findUser(from)))
            user->resetTime();
        else
            user = listUsers->addUser(from);

        if(table_msgs[Buffer.msgCode].min_ret > retval)
        {
            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
            continue;
        }
        if(Buffer.msgCode != CLIENT_MSG::LOGIN && !user->isLoged())
        {
            sendMessage(&from, SERVER_MSG::LOGIN_REQUIRED, NULL, 0);
            continue;
        }
        if(Buffer.msgCode > CLIENT_MSG_COUNT)
        {
            sendMessage(&from, SERVER_MSG::UNKNOWN_COMMAND, NULL, 0);
            continue;
        }
        switch(table_msgs[Buffer.msgCode].num)
        {
            case 0:
                switch(Buffer.msgCode)
                {
                    case CLIENT_MSG::LOGIN: // login
                        if((tempData.loginClass = usersDB->check(Buffer.u.login.username, Buffer.u.login.md5Password)))
                        {
                            user->logIn(tempData.loginClass);
                            sendMessage(&from, SERVER_MSG::LOGIN_SUCCESS, NULL, 0);
                        }
                        else
                            sendMessage(&from, SERVER_MSG::LOGIN_BAD, NULL, 0);
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
                            user->fileTransfer->recieveBlock(Buffer.u.data);
                        break;
                    case CLIENT_MSG::ASK_BLOCK_RANGE: // ask for block range
                        if(!user->fileTransfer)
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        else
                            user->fileTransfer->askForBlocksRange(Buffer.u.block_range.start, Buffer.u.block_range.end);
                        break;
                    case CLIENT_MSG::ASK_BLOCK: // ask for block
                        if(!user->fileTransfer)
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        else
                            user->fileTransfer->askForBlock(Buffer.u.block);
                        break;
                    case CLIENT_MSG::END_FILE_TRANSFER: // finish file transfer
                        if(!user->fileTransfer)
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        else
                        {
                            user->cleanFileTransfer();
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
                        user->fileTransfer = new (std::nothrow) FileTransfer(Buffer.u.upload.path, user, ntohl(Buffer.u.upload.blocksCount));
                        if(user->fileTransfer && user->fileTransfer->isLoaded())
                            sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, NULL, 0);
                        else
                        {
                            user->cleanFileTransfer();
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        }
                        break;
                    case CLIENT_MSG::FILE_DOWNLOAD: // download
                        user->cleanFileTransfer(new (std::nothrow) FileTransfer(Buffer.u.data, user));
                        if(user->fileTransfer && user->fileTransfer->isLoaded())
                        {
                            tempData.i = htonl(user->fileTransfer->getBlocksCount());
                            sendMessage(&from, SERVER_MSG::ACTION_COMPLETED, &tempData.i, 4);
                        }
                        else
                        {
                            user->cleanFileTransfer();
                            sendMessage(&from, SERVER_MSG::SOURCE_BAD, NULL, 0);
                        }
                        break;
                    case CLIENT_MSG::DIR_CD: // cd
                        if(user->moveFolder(Buffer.u.data))
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
            default:
                ioThreadPool.addPathFunction(table_msgs[Buffer.msgCode].num, Buffer.u.data, user, table_msgs[Buffer.msgCode].function);
                break;
        }
    }
#ifdef WIN32
    WSACleanup();
#endif
    userControlThread.join();
}

int sendMessage(const struct sockaddr_in* to, uint16_t msgCode, const void* data, size_t datalen)
{
    static std::mutex lockSend;

    struct __attribute__((packed)){
        uint16_t msgCode;
        char data[BUFFER_SERVER_SIZE - sizeof(msgCode)];
        char nullTerminate[8] = {0};
    } buffer;


    buffer.msgCode = htons(msgCode);
    if(datalen > BUFFER_SERVER_SIZE)
        datalen = BUFFER_SERVER_SIZE;
    if(data && datalen > 0)
        memcpy(buffer.data, data, datalen);

    lockSend.lock();
    int retVal = sendto(sock, (char*)&buffer, datalen + sizeof(msgCode), 0, (struct sockaddr *)to, sizeof(struct sockaddr_in));
    lockSend.unlock();
    return (retVal);
}

void userControl(UserList* listUsers)
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
