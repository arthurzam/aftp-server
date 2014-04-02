#include <cstring>
#include <cstdio>
#include <cstdlib>
#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <signal.h>
#endif

#include "server.h"
#include "LoginDB.h"
#include "UserList.h"

bool_t needExit;
bool_t canExit;
extern UserList* listUsers;

void stopServer()
{
	printf("the server is stopping!\n");
	needExit = TRUE;
	while(!canExit); // wait for all to exit
	printf("the server stopped\n");

	if(listUsers)
		delete listUsers;
}

#ifdef WIN32
BOOL WINAPI signalHandler(DWORD signum)
#else
void signalHandler(int signum)
#endif
{
    switch(signum)
    {
#ifdef WIN32
		case CTRL_C_EVENT:
#else
		case SIGINT:
#endif
			if(listUsers)
				delete listUsers;
			break;
    }
	exit(signum);
}

inline void clearScreen()
{
#ifdef WIN32
	system("cls");
#else
	system("clear");
#endif
}

int main(int argc, char **argv)
{
	bool_t exit = FALSE;
	bool_t serverRunning = FALSE;
    listUsers = NULL;
	LoginDB userDB;
	union{
		int choice;
		struct{
			char username[USERNAME_MAX_LENGTH];
			char password[0xFF];
		} user;
		char filePath[FILENAME_MAX];
	} data;
#ifdef WIN32
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)signalHandler,TRUE))
	{
		fprintf(stderr, "Unable to install handler!\n");
	}
#else
	signal(SIGINT, signalHandler);
#endif
	do{
		if(canExit)
			serverRunning = FALSE;
		printf("0. exit\n");
		if(serverRunning)
			printf("1. stop server\n");
		else
			printf("1. start server\n");
		printf("2. add new user\n3. load login database from file\n4. save login database to file\n5. print database\n6. print current connections\n7. clear screen\n  your choice: ");
		scanf("%d", &data.choice);
		switch(data.choice)
		{
			case 0:
				exit = TRUE;
				if(serverRunning)
				{
					stopServer();
					serverRunning = FALSE;
				}
				break;
			case 1:
				if(serverRunning)
				{
					stopServer();
					serverRunning = FALSE;
				}
				else
				{
					needExit = FALSE;
					canExit = FALSE;
					//startServer(&userDB); //F_IX: remove
#ifdef WIN32
					_beginthread(startServer, 0, &userDB);
#else
					pthread_t thread;
					pthread_create(&thread, NULL, startServer, &userDB);
#endif
					serverRunning = TRUE;
				}
				break;
			case 2:
				printf("username: ");
				scanf("%s", data.user.username);
				printf("password: ");
				scanf("%s", data.user.password);
				userDB.add(data.user.username, data.user.password);
				clearScreen();
				printf("added\n");
				break;
			case 3:
				printf("enter path to file: ");
				scanf("%s", data.filePath);
				userDB.load(data.filePath);
				clearScreen();
				printf("loaded\n");
				break;
			case 4:
				printf("enter path to file: ");
				scanf("%s", data.filePath);
				userDB.save(data.filePath);
				clearScreen();
				printf("saved\n");
				break;
			case 5:
				clearScreen();
				userDB.print();
				break;
            case 6:
                clearScreen();
                if(listUsers && listUsers->getUserCount() > 0)
                    listUsers->print();
                else
                    printf("empty\n");
                break;
			case 7:
				clearScreen();
				break;
		}
	}while(!exit);
	if(listUsers)
		delete listUsers;
}
