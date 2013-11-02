#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "UserList.h"
#include "server.h"
#include "fileControl.h"

#define BUFFER_SERVER_SIZE 0x400 // = 1024

UserList* listUsers = NULL;
SOCKET sock;

extern bool_t needExit;
extern bool_t canExit;

short getMsgCode(char* data, unsigned int datalen);

THREAD_RETURN_VALUE startServer(void* arg)
{
    char Buffer[BUFFER_SERVER_SIZE];
    unsigned short port=DEFAULT_PORT;
    int retval;
    from_len_t fromlen;
    User* user;
    struct sockaddr_in server;
    struct sockaddr_in from;
    short msgCode;

    union {
    	struct {
    		char* src;
    		char* dst;
    		byte_t src_len;
    	} src_dst;
    	char* path;
    	int i;
    	struct {
    		char* username;
    		byte_t* md5Password;
    	} login;
    } tempData;


#ifdef WIN32
    WSADATA wsaData;
    if ((retval = WSAStartup(0x202, &wsaData)) != 0)
    {
    	fprintf(stderr,"Server: WSAStartup() failed with error %d\n", retval);
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
        fprintf(stderr,"Server: socket() failed with error %d\n", WSAGetLastError());
        goto _errorExit;
    }
#else
	if (sock < 0)
	{
		fprintf(stderr,"Server: socket() failed\n");
		goto _errorExit;
	}
#endif

#ifdef WIN32
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        fprintf(stderr,"Server: bind() failed with error %d\n", WSAGetLastError());
        goto _errorExit;
    }
#else
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
    	fprintf(stderr,"Server: bind() failed\n");
    	goto _errorExit;
    }
#endif

    if(listUsers)
    	delete listUsers;
    listUsers = new UserList();
    printf("I'm working on port %d\n" ,DEFAULT_PORT);


    while(!needExit)
	{
        fromlen =sizeof(from);

        retval = recvfrom(sock,Buffer, sizeof(Buffer), 0, (struct sockaddr *)&from, &fromlen);
        printf("Server: Received datagram from %s:%d\n", inet_ntoa(from.sin_addr), from.sin_port);
        if (retval == SOCKET_ERROR || retval == 0)
            continue;
        User tempUser(&from);
		Buffer[retval] = 0;
		msgCode = getMsgCode(Buffer, retval);
		printf("msg code is %04x\n", msgCode);

		if(!(user = listUsers->findUser(tempUser)))
			user = (*listUsers)[listUsers->addUser(tempUser)];
		else
			user->resetTime();

		if(msgCode == 105)
		{
			listUsers->removeUser(user);
			sendMessage(&from, 200, NULL, 0);
			printf("user %s:%d removed\n", inet_ntoa(from.sin_addr), from.sin_port);
			continue;
		}
		else if(msgCode != 100 && !user->isLoged())
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
				// TODO: check if login is correct
				user->logIn();
				sendMessage(&from, 101, Buffer, strlen(Buffer));
				break;
			case 200:
				sendMessage(&from, 200, NULL, 0);
				break;
			case 500: // info
				sendMessage(&from, 400, (char*)"AFTP Server made by Arthur Zamarin, 2013", 41);
				break;
			case 520: // move file
				tempData.src_dst.src_len = *(Buffer + 2);
				tempData.src_dst.src = Buffer + 3;
				tempData.src_dst.dst = Buffer + 5 + tempData.src_dst.src_len;
				if(moveFile(tempData.src_dst.src, tempData.src_dst.dst))
					sendMessage(&from, 200, NULL, 0);
				else
					sendMessage(&from, 300, NULL, 0);
				break;
			case 521: // copy file
				tempData.src_dst.src_len = *(Buffer + 2);
				tempData.src_dst.src = Buffer + 3;
				tempData.src_dst.dst = Buffer + 5 + tempData.src_dst.src_len;
				if(copyFile(tempData.src_dst.src, tempData.src_dst.dst))
					sendMessage(&from, 200, NULL, 0);
				else
					sendMessage(&from, 300, NULL, 0);
				break;
			case 530: // cd
				if(!user->moveFolder(Buffer + 2))
					sendMessage(&from, 300, NULL, 0);
				else
					sendMessage(&from, 200, (char*)user->folderPath(), strlen(user->folderPath()));
				break;
			case 531: // create dir
				if(createDirectory(Buffer + 2))
					sendMessage(&from, 200, NULL, 0);
				else
					sendMessage(&from, 300, NULL, 0);
				break;
			case 999:
				tempData.path = user->getRelativeFile(Buffer + 2);
				if(!tempData.path)
					sendMessage(&from, 300, NULL, 0);
				else
					sendMessage(&from, 200, tempData.path, strlen(tempData.path));
				break;
				/*
			case 500: // action
				actionControl(Buffer + sizeof(msgCode), retval - sizeof(msgCode), user);
				break;*/
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
	canExit = TRUE;
#ifdef WIN32
	return;
#else
	return NULL;
#endif
}

int sendMessage(struct sockaddr_in* to, short msgCode, char* data, int datalen)
{
	static bool_t lockSend = FALSE;
	char* buffer = (char*)malloc(datalen + sizeof(msgCode));
	memcpy(buffer, &msgCode, sizeof(msgCode));
	if(data && datalen > 0)
		memcpy(buffer + sizeof(msgCode), data, datalen);
	while (lockSend) ;
	lockSend = TRUE;
	printf("send message %hd, %s\n", msgCode, data);
	int retVal = sendto(sock, buffer, datalen + sizeof(msgCode), 0, (struct sockaddr *)to, sizeof(struct sockaddr_in));
	lockSend = FALSE;
	return (retVal);
}

short getMsgCode(char* data, unsigned int datalen)
{
	short ret;
	if(datalen < sizeof(ret))
		return (-1);
	memcpy(&ret, data, sizeof(ret));
	return (ret);
}
