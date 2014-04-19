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
    login_t* temp;
    login_t* last = this->head;
    if(last) // we need to put the last member in last
        while(last->next)
            last = last->next;
    if(!src)
        return;
    while(!feof(src))
    {
        temp = (login_t*)malloc(sizeof(login_t));
        temp->next = NULL;
        if(fread(temp->username, 1, USERNAME_MAX_LENGTH, src) < USERNAME_MAX_LENGTH ||
                fread(temp->password, 1, MD5_RESULT_LENGTH, src) < MD5_RESULT_LENGTH)
        {
            free(temp);
            break;
        }
        if(!this->head)
        {
            this->head = temp;
            last = temp;
        }
        else
        {
            last->next = temp;
            last = temp;
        }
        this->count++;
    }
    fclose(src);
}

void LoginDB::add(const char* username, const char* password)
{
    login_t* curr = this->head;
    login_t* newOne = (login_t*)malloc(sizeof(login_t));
    strcpy(newOne->username, username);
    md5((byte_t*)password, strlen(password), newOne->password);
    newOne->next = NULL;
    if(!curr)
    {
        this->head = newOne;
        this->count++;
        return;
    }
    while(curr->next)
        curr = curr->next;
    curr->next = newOne;
}

bool_t LoginDB::check(const char* username, const byte_t* passwordMD5) const
{
    login_t* curr = this->head;
    while(curr)
    {
        if(!(memcmp(curr->password, passwordMD5, MD5_RESULT_LENGTH) || strcmp(curr->username, username)))
            return (TRUE);
        curr = curr->next;
    }
    return (FALSE);
}

int LoginDB::save(const char* path) const
{
    if(!this->head)
        return (EXIT_FAILURE);
    FILE* dst = fopen(path, "wb");
    if(!dst)
        return (EXIT_FAILURE);
    login_t* curr = this->head;
    while(curr)
    {
        fwrite(curr->username, 1, USERNAME_MAX_LENGTH, dst);
        fwrite(curr->password, 1, MD5_RESULT_LENGTH, dst);
        curr = curr->next;
    }
    fclose(dst);
    return (EXIT_SUCCESS);
}

LoginDB::~LoginDB()
{
    login_t* temp = NULL;
    while(this->head)
    {
        temp = this->head->next;
        free(this->head);
        this->head = temp;
    }
    this->head = NULL;
}

bool_t LoginDB::isEmpty() const
{
    return (this->count == 0);
}

void LoginDB::print() const
{
    login_t* temp = this->head;
    int i = 1; // for human it is better to start with #1
    int j;

    for(; temp; temp = temp->next, ++i)
    {
        printf("user #%d: {%s, ", i, temp->username);

        for(j = 0; j < MD5_RESULT_LENGTH; j++)
            printf("%02x", temp->password[j]);

        printf(" }\n");
    }
}
