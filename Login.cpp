#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "Login.h"

Login::Login(const char* username, const uint8_t* password, LOGIN_ACCESS state)
{
    strncpy(this->username, username, USERNAME_MAX_LENGTH - 1);
    this->username[USERNAME_MAX_LENGTH - 1] = 0;
    memcpy(this->password, password, MD5_DIGEST_LENGTH);
    this->state = (uint8_t)state;
    this->restrictedFolders = NULL;
    this->restrictedFoldersCount = 0;
    this->isInit = true;
    this->_next = NULL;
}

Login::Login(FILE* srcFile)
{
    if(srcFile)
    {
        char folder[REL_PATH_MAX + 1];
        char* P;
        uint8_t i;
        if(fread(this->username, 1, USERNAME_MAX_LENGTH, srcFile) < USERNAME_MAX_LENGTH ||
                fread(this->password, 1, MD5_DIGEST_LENGTH, srcFile) < MD5_DIGEST_LENGTH ||
                fread(&this->state, 1, sizeof(this->state), srcFile) < sizeof(this->state) ||
                fread(&i, 1, sizeof(i), srcFile) < sizeof(i))
            goto _bad;

        this->restrictedFolders = NULL;
        this->restrictedFoldersCount = 0;
        for(; i; --i)
        {
            if((P = strchr(fgets(folder, REL_PATH_MAX + 1, srcFile), '\n')))
                *P = 0;
            this->addRestrictedFolder(folder);
        }
        this->isInit = true;
    }
    else
    {
_bad:
        this->password[0] = 0;
        this->restrictedFolders = NULL;
        this->state = LOGIN_ACCESS::LIMITED;
        this->username[0] = 0;
        this->restrictedFoldersCount = 0;
        this->isInit = false;
    }
    this->_next = NULL;
}

Login::~Login()
{
    folder* temp;
    while(this->restrictedFolders)
    {
        temp = this->restrictedFolders;
        this->restrictedFolders = this->restrictedFolders->next;
        free(temp);
    }
    if(this->_next)
        delete this->_next;
    this->_next = NULL;
}

bool Login::check(const char* username, const uint8_t* password) const
{
    return (!(memcmp(this->password, password, MD5_DIGEST_LENGTH) || strcmp(this->username, username)));
}

void Login::addRestrictedFolder(const char* dir)
{
    folder* newOne = (folder*)malloc(sizeof(folder));
    folder* temp = this->restrictedFolders;

    strncpy(newOne->folder, dir, REL_PATH_MAX - 1);
    char* dirP = newOne->folder - 1;
    while((dirP = strchr(dirP + 1, PATH_SEPERATOR_BAD)))
        *dirP = PATH_SEPERATOR_GOOD;

    newOne->folder[REL_PATH_MAX - 1] = 0;
    newOne->folder[REL_PATH_MAX] = 0;
    newOne->folderLen = strlen(dir);
    newOne->next = NULL;
    if(!temp)
    {
        this->restrictedFolders = newOne;
    }
    else
    {
        while(temp->next)
            temp = temp->next;
        temp->next = newOne;
    }
    this->restrictedFoldersCount++;
}

bool Login::isRestrictedFolder(const char* path) const
{
    if(this->state != LOGIN_ACCESS::LIMITED)
        return (false);
    folder* temp;
    for(temp = this->restrictedFolders; temp; temp = temp->next)
    {
        if(!strncmp(path, temp->folder, temp->folderLen))
            return (true);
    }
    return (false);
}

bool Login::save(FILE* dstFile) const
{
    if(!dstFile)
        return (false);
    fwrite(this->username, 1, USERNAME_MAX_LENGTH, dstFile);
    fwrite(this->password, 1, MD5_DIGEST_LENGTH, dstFile);
    fwrite(&this->state, 1, sizeof(this->state), dstFile);
    fwrite(&this->restrictedFoldersCount, 1, sizeof(this->restrictedFoldersCount), dstFile);
    folder* temp;
    for(temp = this->restrictedFolders; temp; temp = temp->next)
    {
        fputs(temp->folder, dstFile);
        fputc('\n', dstFile);
    }
    return (true);
}

void Login::print() const
{
    int i;
    folder* temp;
    printf("{%s, ", this->username);
    for(i = 0; i < MD5_DIGEST_LENGTH; ++i)
        printf("%02x", this->password[i]);
    switch(this->state)
    {
    case LOGIN_ACCESS::ADMIN:
        printf(", admin");
        break;
    case LOGIN_ACCESS::ALL:
        printf(", all");
        break;
    case LOGIN_ACCESS::LIMITED:
        printf(", limited");
        break;
    }
    if(this->restrictedFolders)
    {
        printf(", [");
        for(temp = this->restrictedFolders; temp; temp = temp->next)
        {
            printf("%s, ", temp->folder);
        }
        printf("\b\b]"); // remove last ", "
    }
    printf("}\n");
}
