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

bool needExit;
bool canExit;
unsigned short port = DEFAULT_PORT;
char* base_server_folder = (char*)DEFAULT_SERVER_BASE_FOLDER;
extern UserList* listUsers;

void stopServer()
{
    printf("the server is stopping!\n");
    needExit = true;
    while(!canExit); // wait for all to exit
    printf("the server stopped\n");

    if(listUsers)
        delete listUsers;
}

bool startServerThread(LoginDB* userDB)
{
    // replace all bad separators into good one
    char* pathP = base_server_folder - 1;
    while((pathP = strchr(pathP + 1, PATH_SEPERATOR_BAD)))
        *pathP = PATH_SEPERATOR_GOOD;

    // check for last char is PATH_SEPERATOR_GOOD => if not set it
    if(*((pathP = base_server_folder + strlen(base_server_folder)) - 1) != PATH_SEPERATOR_GOOD)
    {
        *(pathP++) = PATH_SEPERATOR_GOOD;
        *(pathP) = '\0';
    }


    if(!isDirectory(base_server_folder))
    {
#ifdef WIN32
        if(!CreateDirectoryA(base_server_folder, NULL))
#else
        if(mkdir(base_server_folder, 0700))
#endif
            return (false);
    }
    needExit = false;
    canExit = false;
#ifdef WIN32
    _beginthread(startServer, 0, userDB);
#else
    pthread_t thread;
    pthread_create(&thread, NULL, startServer, userDB);
#endif
    return (true);
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
    bool exit = true;
    bool serverRunning = false;
    listUsers = NULL;
    LoginDB userDB;
    union {
        int choice;
        struct {
            char username[USERNAME_MAX_LENGTH];
            char password[0xFF];
        } user;
        char filePath[FILENAME_MAX];
    } data;
    char basePath[FILENAME_MAX];
    if(argc > 1) // there are parameters
    {
        for(int i = 1; i < argc; i++)
        {
            if(!strcmp(argv[i], "-db") && i + 1 < argc)
            {
                userDB.load(argv[++i]);
            }
            else if(!strcmp(argv[i], "-p") && i + 1 < argc)
            {
                data.choice = atoi(argv[++i]);
                if(data.choice > 0xFFFF)
                {
                    fprintf(stderr, "Bad port specified!\n");
                    return (0);
                }
                port = (short)data.choice;
            }
            else if(!strcmp(argv[i], "-f") && i + 1 < argc)
            {
                strncpy(basePath, argv[++i], FILENAME_MAX);
                base_server_folder = basePath;
            }
            else if(!strcmp(argv[i], "-h"))
            {
                printf("This is the AFTP server\n");
                printf("  usage:\n");
                printf("    -db (path)   load the Login database from this file\n");
                printf("    -p  (port)   the port on which the server should listen\n");
                printf("    -f  (path)   the base folder path for the server root folder\n");
                printf("    -h           show this text\n");
                printf("    -a           auto start server\n");
                return (0);
            }
            else if(!strcmp(argv[i], "-a"))
            {
                serverRunning = true;
            }
        }
    }
#ifdef WIN32
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)signalHandler, true))
    {
        fprintf(stderr, "Unable to install handler!\n");
    }
#else
    signal(SIGINT, signalHandler);
#endif
    if(serverRunning)
        serverRunning = startServerThread(&userDB);
    do {
        if(canExit)
            serverRunning = true;
        printf("0. exit\n");
        if(serverRunning)
            printf("1. stop server\n");
        else
            printf("1. start server\n");
        printf("2. add new user\n3. load login database from file\n4. save login database to file\n5. print database\n6. print current connections\n7. clear screen\n8. server configurations\n  your choice: ");
        scanf("%d", &data.choice);
        switch(data.choice)
        {
        case 0:
            exit = true;
            if(serverRunning)
            {
                stopServer();
                serverRunning = false;
            }
            break;
        case 1:
            if(serverRunning)
            {
                stopServer();
                serverRunning = false;
            }
            else
            {
                serverRunning = startServerThread(&userDB);
            }
            break;
        case 2:
            printf("username: ");
            scanf("%s", data.user.username);
            printf("password: ");
            scanf("%s", data.user.password);
            userDB.add(data.user.username, data.user.password, Login::LOGIN_ACCESS_ADMIN);
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
        case 8:
            if(serverRunning)
            {
                printf("the server is running, you can't configure it in this state!\n");
            }
            else
            {
                printf("0. return\n1. change port (%d)\n2. change server's base folder (%s)\n  your choice: ", port, base_server_folder);
                scanf("%d", &data.choice);
                switch(data.choice)
                {
                case 1:
                    printf("enter new port: ");
                    scanf("%d", &data.choice);
                    if(data.choice > 0xFFFF)
                        fprintf(stderr, "Bad port specified!\n");
                    else
                        port = (short)data.choice;
                    break;
                case 2:
                    printf("enter new base folder: ");
                    scanf("%s", basePath);
                    base_server_folder = basePath;
                }
            }
            break;
        }
    } while(!exit);
    if(listUsers)
        delete listUsers;
}
