#include <cstdio>
#include <cstring>
#include <openssl/md5.h>

#include "LoginDB.h"

LoginDB::LoginDB()
{
    this->count = 0;
    this->head = NULL;
}

LoginDB::LoginDB(const char* filePath)
{
    this->count = 0;
    this->head = NULL;
    load(filePath);
}

void LoginDB::load(const char* filePath)
{
    FILE* src = fopen(filePath, "rb");
    Login* temp;
    Login* last = this->head;
    if(last) // we need to put the last member in last
        while(last->next())
            last = last->next();
    if(!src)
        return;
    while(!feof(src))
    {
        temp = new Login(src);
        if(!temp->isLoaded())
        {
            delete temp;
            break;
        }
        if(!this->head)
            this->head = temp;
        else
            last->next() = temp;
        last = temp;
        this->count++;
    }
    fclose(src);
}

void LoginDB::add(Login* next)
{
    Login* curr = this->head;
    if(!curr)
    {
        this->head = next;
    }
    else
    {
        while(curr->next())
            curr = curr->next();
        curr->next() = next;
    }
    this->count++;
}

void LoginDB::add(const char* username, const char* password, Login::LOGIN_ACCESS state)
{
    uint8_t md5Res[MD5_DIGEST_LENGTH];
    MD5((uint8_t*)password, strlen(password), md5Res);
    this->add(new Login(username, md5Res, state));
}

bool LoginDB::remove(int number)
{
    Login* toDel;
    if(number > this->count || number <= 0)
        return (false);
    if(number > 1)
    {
        Login* temp = this->head;
        for(int i = 1; i < number; ++i, temp = temp->next()) ;
        toDel = temp->next();
        temp->next() = toDel->next();
    }
    else
    {
        toDel = this->head;
        this->head = toDel->next();
    }
    toDel->next() = NULL;
    delete toDel;
    --this->count;
    return (true);
}

const Login* LoginDB::check(const char* username, const uint8_t* passwordMD5) const
{
    const Login* curr = this->head;
    while(curr)
    {
        if(curr->check(username, passwordMD5))
            return (curr);
        curr = curr->next();
    }
    return (NULL);
}

bool LoginDB::save(const char* path) const
{
    if(!this->head)
        return (false);
    FILE* dst = fopen(path, "wb");
    if(!dst)
        return (false);
    const Login* curr = this->head;
    while(curr)
    {
        curr->save(dst);
        curr = curr->next();
    }
    fclose(dst);
    return (true);
}

LoginDB::~LoginDB()
{
    if(this->head)
        delete (this->head);
    this->head = NULL;
}

void LoginDB::print() const
{
    Login* temp = this->head;
    int i = 1; // for human it is better to start with #1

    for(; temp; temp = temp->next(), ++i)
    {
        printf("user #%d: ", i);
        temp->print();
    }
}

void LoginDB::input()
{
    char username[USERNAME_MAX_LENGTH];
    uint8_t md5R[MD5_DIGEST_LENGTH];
    char str[FILENAME_MAX];
    int i;
    Login::LOGIN_ACCESS state;

    printf("enter username: ");
    scanf("%32s", username);
    printf("enter password: ");
    scanf("%s", str);
    MD5((uint8_t*)str, strlen(str), md5R);
    printf("choose state:\n 0. admin\n 1. limited\n 2. all\nyour choice: ");
    scanf("%u", &i);
    state = (i < 3 ? (Login::LOGIN_ACCESS)i : Login::LOGIN_ACCESS::LIMITED);
    Login* n = new Login(username, md5R, state);
    if(state == Login::LOGIN_ACCESS::LIMITED)
    {
        printf("now you add the restricted folders [press only ENTER to finish]\n(the folder should start with / and end with / - otherwise unknown behavior might happen)\n");
        fgetc(stdin); // input empty \n from previous scanf - I don't know why, but we need!
        do {
            fgets(str, FILENAME_MAX, stdin);
            str[strlen(str) - 1] = 0;
            if(str[0])
            {
                n->addRestrictedFolder(str);
            }
        } while(str[0]);
    }

    this->add(n);
}

