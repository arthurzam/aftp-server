#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <thread>
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
#include "LoginDB.h"

SOCKET sock;

extern bool needExit;
extern bool canExit;
extern unsigned short port;

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

        if((user = listUsers->findUser(from)))
            user->resetTime();
        else
            user = listUsers->addUser(from);

        if(msgCode != 100 && !user->isLoged())
        {
            sendMessage(&from, 100, NULL, 0);
            continue;
        }
        switch(msgCode)
        {
        case 100: // login
            tempData.login.username = Buffer + 18;
            tempData.login.md5Password = (uint8_t*)(Buffer + 2);
            if((tempData.loginClass = usersDB->check(tempData.login.username, tempData.login.md5Password)))
            {
                user->logIn(tempData.loginClass);
                sendMessage(&from, 101, NULL, 0);
            }
            else
            {
                sendMessage(&from, 110, NULL, 0);
            }
            break;
        case 105: // logout
            sendMessage(&from, 200, NULL, 0);
            listUsers->removeUser(user);
            break;
        case 200:
            sendMessage(&from, 200, NULL, 0);
            break;
        case 210: // block in File Upload
            if(!user->fileTransfer)
                sendMessage(&from, 300, NULL, 0);
            else
                user->fileTransfer->recieveBlock(Buffer + 2);
            break;
        case 211: // ask for block range
            if(!user->fileTransfer)
                sendMessage(&from, 300, NULL, 0);
            else
                user->fileTransfer->askForBlocksRange(*((uint32_t*)(Buffer + 2)), *((uint32_t*)(Buffer + 6)));
            break;
        case 212: // ask for block
            if(!user->fileTransfer)
                sendMessage(&from, 300, NULL, 0);
            else
                user->fileTransfer->askForBlock(*((uint32_t*)(Buffer + 2)));
            break;
        case 213: // finish file transfer
            if(!user->fileTransfer)
                sendMessage(&from, 300, NULL, 0);
            else
            {
                delete user->fileTransfer;
                user->fileTransfer = NULL;
                sendMessage(&from, 200, NULL, 0);
            }
            break;
        case 500: // info
            sendMessage(&from, 400, (char*)"AFTP Server made by Arthur Zamarin, 2014", 41);
            break;
        case 510: // upload
            if(user->fileTransfer)
                delete user->fileTransfer;
            user->fileTransfer = new FileTransfer(Buffer + 6, user, *(uint32_t*)(Buffer + 2));
            if(user->fileTransfer->isLoaded())
                sendMessage(&from, 200, NULL, 0);
            else
            {
                sendMessage(&from, 300, NULL, 0);
                delete user->fileTransfer;
                user->fileTransfer = NULL;
            }
            break;
        case 511: // download
            if(user->fileTransfer)
                delete user->fileTransfer;
            user->fileTransfer = new FileTransfer(Buffer + 2, user);
            if(user->fileTransfer->isLoaded())
            {
                tempData.i = user->fileTransfer->getBlocksCount();
                sendMessage(&from, 200, &tempData.i, 4);
            }
            else
            {
                sendMessage(&from, 300, NULL, 0);
                delete user->fileTransfer;
                user->fileTransfer = NULL;
            }
            break;
        case 520: // move file
            ioThreadPool.add2pathFunction(Buffer + 2, user, moveFile);
            break;
        case 521: // copy file
            ioThreadPool.add2pathFunction(Buffer + 2, user, copyFile);
            break;
        case 522: // remove file
            ioThreadPool.add1pathFunction(Buffer + 2, user, removeFile);
            break;
        case 523: // get file size
            ioThreadPool.add1pathFunction(Buffer + 2, user, getFilesize);
            break;
        case 524: // get md5 of file
            ioThreadPool.add1pathFunction(Buffer + 2, user, getMD5OfFile);
            break;
#ifndef WIN32
        case 525: // symlink file
            ioThreadPool.add2pathFunction(Buffer + 2, user, symbolicLink);
            break;
#endif
        case 530: // cd
            if(user->moveFolder(Buffer + 2))
                sendMessage(&from, 200, NULL, 0);
            else
                sendMessage(&from, 300, NULL, 0);
            break;
        case 531: // create directory
            ioThreadPool.add1pathFunction(Buffer + 2, user, createDirectory);
            break;
        case 532: // remove directory
            ioThreadPool.add1pathFunction(Buffer + 2, user, removeFolder);
            break;
        case 533: // move directory
            ioThreadPool.add2pathFunction(Buffer + 2, user, moveDirectory);
            break;
        case 534: // copy directory
            ioThreadPool.add2pathFunction(Buffer + 2, user, copyFolder);
            break;
        case 535: // get contents of directory
            ioThreadPool.add1pathFunction(Buffer + 2, user, getContentDirectory);
            break;
        case 536: // pwd
            if(user->folderPath()[0])
                sendMessage(&from, 200, user->folderPath(), strlen(user->folderPath()));
            else
                sendMessage(&from, 200, (char*)"/", 1);
            break;
        default:
            sendMessage(&from, 391, (char*)"unknown command", 15);
            break;
        }
    }
_errorExit:
#ifdef WIN32
    WSACleanup();
#endif
    userControlThread.join();
    canExit = true;
}

int sendMessage(const struct sockaddr_in* to, uint16_t msgCode, const void* data, int datalen)
{
    static bool lockSend = false; // mini mutex

    char buffer[BUFFER_SERVER_SIZE + 5];
    memcpy(buffer, &msgCode, sizeof(msgCode));
    if(datalen > BUFFER_SERVER_SIZE)
        datalen = BUFFER_SERVER_SIZE;
    if(data && datalen > 0)
        memcpy(buffer + sizeof(msgCode), data, datalen);

    while (lockSend) ;
    lockSend = true;
    int retVal = sendto(sock, buffer, datalen + sizeof(msgCode), 0, (struct sockaddr *)to, sizeof(struct sockaddr_in));
    lockSend = false;
    return (retVal);
}

void userControl(UserList* listUsers)
{
#define WAIT_USER_CONTROL_SECONDS 50
	int i;
    while (!needExit)
    {
        for(i = 0;!needExit && i < WAIT_USER_CONTROL_SECONDS; ++i)
#ifdef WIN32
            Sleep(1000); //mili-seconds
#else
            sleep(1);    // seconds
#endif
        if(needExit)
        	listUsers->userControl();
    }
}
