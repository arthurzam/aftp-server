#include <cstring>
#include <cstdio>
#include <cstdlib>
#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

#include "server.h"

bool_t needExit = FALSE;
bool_t canExit = FALSE;

int main(int argc, char **argv)
{
	printf("server is started\n");
	char buf[120];
	//startServer(NULL);
#ifdef WIN32
	_beginthread(startServer, 0, NULL);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, startServer, NULL);
#endif
	do{
		printf("    Do you want to exit? \n");
		scanf("%s", buf);
		system(CLEAR_SCREEN);
	}while(strcmp(buf, "yes"));
	printf("    Start Exit \n");
	needExit = TRUE;
	while(!canExit); // wait for all to exit
	printf("    Finish \n");
	return (0);
}