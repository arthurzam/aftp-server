#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <thread>

#ifdef WIN32
#include <windows.h>
#include <process.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <openssl/rsa.h>
#include <openssl/pem.h>

#include "LoginDB.h"
#include "UserList.h"
#include "defenitions.h"
#include "fileControl.h"
#include "server.h"

bool needExit;
uint16_t port = DEFAULT_PORT;
char* base_server_folder = (char*)DEFAULT_SERVER_BASE_FOLDER;

static void stopServer(std::thread& serverThread)
{
    printf("the server is stopping!\n");
    needExit = true;
    serverThread.join();
    printf("the server stopped\n");
}

static bool startServerThread(LoginDB* userDB, UserList* listUsers, std::thread& serverThread, const rsa_control_t& rsaControl)
{
    // replace all bad separators into good one
    char* pathP = base_server_folder - 1;
    replaceSeperator(pathP);

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
		struct stat st;
        memset(&st, 0, sizeof(struct stat));
        if (stat(base_server_folder, &st) && mkdir(base_server_folder, 0755))
#endif
            return (false);
    }
    needExit = false;
    serverThread = std::thread(startServer, userDB, listUsers, rsaControl);
    return (true);
}

inline void clearScreen()
{
#ifdef WIN32
    system("cls");
#else
    system("reset");
#endif
}

static void generateRSAKey(const char* path)
{
    RSA* keypair = RSA_generate_key(RSA_NSIZE, 3, NULL, NULL);
    FILE* f = fopen(path, "wb");
    PEM_write_RSAPublicKey(f, keypair);
    PEM_write_RSAPrivateKey(f, keypair, NULL, NULL, 0, 0, NULL);
    fclose(f);
    RSA_free(keypair);
}

static RSA* readRSAKey(const char* path)
{
    RSA* keypair = RSA_new();
    FILE* f = fopen(path, "rb");
    PEM_read_RSAPublicKey(f, &keypair, NULL, NULL);
    PEM_read_RSAPrivateKey(f, &keypair, NULL, NULL);
    fclose(f);
    return keypair;
}

int main(int argc, char **argv)
{
    bool exit = false;
    bool serverRunning = false;
    LoginDB userDB;
    UserList listUsers;
    std::thread serverThread;
    rsa_control_t rsaControl; //TODO: set this object
    union {
        int choice;
        struct {
            char username[Login::USERNAME_MAX_LENGTH];
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
                printf("This is the AFTP server\n"
                       "  usage:\n"
                       "    -db (path)   load the Login database from this file\n"
                       "    -p  (port)   the port on which the server should listen\n"
                       "    -f  (path)   the base folder path for the server root folder\n"
                       "    -h           show this text\n"
                       "    -a           auto start server\n");
                return (0);
            }
            else if(!strcmp(argv[i], "-a"))
            {
                serverRunning = true;
            }
            else if(!strcmp(argv[i], "-g") && i + 1 < argc)
            {
                generateRSAKey(argv[++i]);
                readRSAKey(argv[++i]); // FIXME: remove
                return (0);
            }
        }
    }
    if(serverRunning)
        serverRunning = startServerThread(&userDB, &listUsers, serverThread, rsaControl);
    do {
        printf("0. exit\n"
               "1. %s server\n"
               "2. Login configurations\n"
               "3. print current connections\n"
               "4. clear screen\n"
               "5. server configurations\n"
               "  your choice: ", (serverRunning ? "stop" : "start"));
        scanf("%d", &data.choice);
        switch(data.choice)
        {
            case 0:
                exit = true;
                if(serverRunning)
                {
                    stopServer(serverThread);
                    serverRunning = false;
                }
                break;
            case 1:
                if(serverRunning)
                {
                    stopServer(serverThread);
                    serverRunning = false;
                }
                else
                {
                    serverRunning = startServerThread(&userDB, &listUsers, serverThread, rsaControl);
                }
                break;
            case 2:
                printf("0. return\n"
                       "1. load database from file\n"
                       "2. save database to file\n"
                       "3. add\n"
                       "4. remove\n"
                       "5. print\n"
                       "  your choice: ");
                scanf("%d", &data.choice);
                switch(data.choice)
                {
                    case 1:
                        printf("enter path to file: ");
                        scanf("%s", data.filePath);
                        userDB.load(data.filePath);
                        clearScreen();
                        printf("loaded\n");
                        break;
                    case 2:
                        printf("enter path to file: ");
                        scanf("%s", data.filePath);
                        userDB.save(data.filePath);
                        clearScreen();
                        printf("saved\n");
                        break;
                    case 3:
                        userDB.input();
                        clearScreen();
                        printf("added\n");
                        break;
                    case 4:
                        clearScreen();
                        userDB.print();
                        printf(" the number of login to remove: ");
                        scanf("%d", &data.choice);
                        userDB.remove(data.choice);
                        clearScreen();
                        printf("removed\n");
                        break;
                    case 5:
                        clearScreen();
                        userDB.print();
                        break;
                }
                break;
            case 3:
                clearScreen();
                if(listUsers.getUserCount() > 0)
                    listUsers.print();
                else
                    printf("empty\n");
                break;
            case 4:
                clearScreen();
                break;
            case 5:
                if(serverRunning)
                {
                    printf("the server is running, you can't configure it in this state!\n");
                }
                else
                {
                    printf("0. return\n"
                           "1. change port (%d)\n"
                           "2. change server's base folder (%s)\n"
                           "  your choice: ", port, base_server_folder);
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
}
