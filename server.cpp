#include <cstdlib>
#include <cstdio>
#include <cstring>
#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#endif

#include "UserList.h"
#include "server.h"
#include "fileControl.h"
#include "LoginDB.h"

UserList* listUsers = NULL;
SOCKET sock;

extern bool_t needExit;
extern bool_t canExit;
extern unsigned short port;

threadReturnValue userControl(void* arg);

threadReturnValue startServer(void* arg)
{
    LoginDB* usersDB = (LoginDB*)arg;
    char Buffer[BUFFER_SERVER_SIZE];
    int retval;
    User* user;
    struct sockaddr_in server;
    struct sockaddr_in from;
    short msgCode;
    socklen_t fromlen = sizeof(from);
    fsData data;

    union {
        struct {
            char* src;
            char* dst;
            byte_t src_len;
            byte_t dst_len;
        } src_dst;
        char* path;
        Login* loginClass;
        int i;
        unsigned long long int l;
        byte_t md5[MD5_RESULT_LENGTH];
        struct {
            char* username;
            byte_t* md5Password;
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

    if(listUsers)
        delete listUsers;
    listUsers = new UserList();
#ifdef WIN32
    _beginthread(userControl, 0, NULL);
#else
    pthread_t thread;
    pthread_create(&thread, NULL, userControl, NULL);
#endif
    while(!needExit)
    {
        retval = recvfrom(sock, Buffer, sizeof(Buffer), 0, (struct sockaddr *)&from, &fromlen);
        if (retval == SOCKET_ERROR || retval < 2)
            continue;
        Buffer[retval] = 0;
        memcpy(&msgCode, Buffer, sizeof(msgCode));

        if(!(user = listUsers->findUser(from)))
            user = (*listUsers)[listUsers->addUser(from)];
        else
            user->resetTime();

        if(msgCode != 100 && !user->isLoged())
        {
            sendMessage(&from, 100, NULL, 0);
        }
        else
        {
            switch(msgCode)
            {
            case 100: // login
                tempData.login.username = Buffer + 18;
                tempData.login.md5Password = (byte_t*)(Buffer + 2);
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
                listUsers->removeUser(user);
                sendMessage(&from, 200, NULL, 0);
                break;
            case 200:
                sendMessage(&from, 200, NULL, 0);
                break;
            case 210: // block in File Upload
                if(!user->fileTransfer)
                    sendMessage(&from, 300, NULL, 0);
                else
                    user->fileTransfer->recieveBlock(Buffer + 2, retval - 2);
                break;
            case 211: // ask for block range
                if(!user->fileTransfer)
                    sendMessage(&from, 300, NULL, 0);
                else
                    user->fileTransfer->askForBlocksRange(*((int*)(Buffer + 2)), *((int*)(Buffer + 6)));
                break;
            case 212: // ask for block
                if(!user->fileTransfer)
                    sendMessage(&from, 300, NULL, 0);
                else
                    user->fileTransfer->askForBlock(*((int*)(Buffer + 2)));
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
                user->fileTransfer = new FileTransfer(Buffer + 6, user, *(int*)(Buffer + 2));
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
                tempData.src_dst.src_len = *(Buffer + 2);
                data.data.path2.src = Buffer + 3;
                tempData.src_dst.dst_len = *(Buffer + 5 + tempData.src_dst.src_len);
                data.data.path2.dst = Buffer + 6 + tempData.src_dst.src_len;
                createFSthread(moveFile, &data, user);
                break;
            case 521: // copy file
                tempData.src_dst.src_len = *(Buffer + 2);
                data.data.path2.src = Buffer + 3;
                tempData.src_dst.dst_len = *(Buffer + 5 + tempData.src_dst.src_len);
                data.data.path2.dst = Buffer + 6 + tempData.src_dst.src_len;
                createFSthread(copyFile, &data, user);
                break;
            case 522: // remove file
                data.data.path = Buffer + 2;
                createFSthread(removeFile, &data, user);
                break;
            case 523: // get file size
                tempData.l = getFilesize(Buffer + 2, user);
                if(tempData.l == (unsigned long long int)-1)
                    sendMessage(&from, 300, NULL, 0);
                else
                    sendMessage(&from, 200, &tempData.l, sizeof(unsigned long long int));
                break;
            case 524: // get md5 of file
                data.data.path = Buffer + 2;
                createFSthread(getMD5OfFile, &data, user);
                break;
#ifndef WIN32
            case 525: // symlink file
                tempData.src_dst.src_len = *(Buffer + 2);
                data.data.path2.src = Buffer + 3;
                tempData.src_dst.dst_len = *(Buffer + 5 + tempData.src_dst.src_len);
                data.data.path2.dst = Buffer + 6 + tempData.src_dst.src_len;
                createFSthread(copyFile, &data, user);
                break;
#endif
            case 530: // cd
                if(user->moveFolder(Buffer + 2))
                    sendMessage(&from, 200, NULL, 0);
                else
                    sendMessage(&from, 300, NULL, 0);
                break;
            case 531: // create directory
                data.data.path = Buffer + 2;
                createFSthread(createDirectory, &data, user);
                break;
            case 532: // remove directory
                data.data.path = Buffer + 2;
                createFSthread(removeFolder, &data, user);
                break;
            case 533: // move directory
                tempData.src_dst.src_len = *(Buffer + 2);
                data.data.path2.src = Buffer + 3;
                tempData.src_dst.dst_len = *(Buffer + 5 + tempData.src_dst.src_len);
                data.data.path2.dst = Buffer + 6 + tempData.src_dst.src_len;
                createFSthread(moveDirectory, &data, user);
                break;
            case 534: // copy directory
                tempData.src_dst.src_len = *(Buffer + 2);
                data.data.path2.src = Buffer + 3;
                tempData.src_dst.dst_len = *(Buffer + 5 + tempData.src_dst.src_len);
                data.data.path2.dst = Buffer + 6 + tempData.src_dst.src_len;
                createFSthread(copyFolder, &data, user);
                break;
            case 535: // get contents of directory
                data.data.path = Buffer + 2;
                createFSthread(getContentDirectory, &data, user);
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

    }
_errorExit:
#ifdef WIN32
    WSACleanup();
#endif
    if(listUsers)
        delete listUsers;
    listUsers = NULL;
    canExit = TRUE;
#ifndef WIN32
    return NULL;
#endif
}

int sendMessage(struct sockaddr_in* to, short msgCode, const void* data, int datalen)
{
    static bool_t lockSend = FALSE; // mini mutex

    char buffer[BUFFER_SERVER_SIZE + 5];
    memcpy(buffer, &msgCode, 2);
    if(datalen > BUFFER_SERVER_SIZE)
        datalen = BUFFER_SERVER_SIZE;
    if(data && datalen > 0)
        memcpy(buffer + 2, data, datalen);

    while (lockSend) ;
    lockSend = TRUE;
    int retVal = sendto(sock, buffer, datalen + 2, 0, (struct sockaddr *)to, sizeof(struct sockaddr_in));
    lockSend = FALSE;
    return (retVal);
}

threadReturnValue userControl(void* arg)
{
#ifdef WIN32
    const int WAIT_TIME = 50000; //mili-seconds
#else
    const int WAIT_TIME = 50;    // seconds
#endif
    while (!needExit)
    {
#ifdef WIN32
        Sleep(WAIT_TIME);
#else
        sleep(WAIT_TIME);
#endif
        listUsers->userControl();
    }
#ifndef WIN32
    return NULL;
#endif
}
