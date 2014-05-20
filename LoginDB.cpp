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
        {
            this->head = temp;
            last = temp;
        }
        else
        {
            last->next() = temp;
            last = temp;
        }
        this->count++;
    }
    fclose(src);
}

void LoginDB::add(const char* username, const char* password, Login::LOGIN_ACCESS state)
{
    uint8_t md5Res[16];
    MD5((uint8_t*)password, strlen(password), md5Res);
    Login* curr = this->head;
    Login* newOne = new Login(username, md5Res, state);
    if(!curr)
    {
        this->head = newOne;
        this->count++;
        return;
    }
    while(curr->next())
        curr = curr->next();
    curr->next() = newOne;
}

Login* LoginDB::check(const char* username, const uint8_t* passwordMD5) const
{
    Login* curr = this->head;
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
    Login* curr = this->head;
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
    {
        delete (this->head);
    }
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
