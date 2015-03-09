#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "Login.h"
#include "fileControl.h"

Login::Login(const char* username, const uint8_t* password, LOGIN_ACCESS state)
{
    strncpy(this->username, username, sizeof(this->username) - 1);
    this->username[sizeof(this->username) - 1] = 0;
    memcpy(this->password, password, sizeof(this->password));
    this->state = (uint8_t)state;
    this->isInit = true;
}

Login::Login(FILE* srcFile)
{
    if(!srcFile)
        return;
    char folder[REL_PATH_MAX + 1];
    char* P;
    uint8_t i;
    if(fread(this->username, 1, sizeof(this->username), srcFile) < sizeof(this->username) ||
       fread(this->password, 1, sizeof(this->password), srcFile) < sizeof(this->password) ||
       fread(&this->state, 1, sizeof(this->state), srcFile) < sizeof(this->state) ||
       fread(&i, 1, sizeof(i), srcFile) < sizeof(i))
        return;

    for(; i; --i)
    {
        if((P = strchr(fgets(folder, REL_PATH_MAX + 1, srcFile), '\n')))
            *P = '\0';
        this->addRestrictedFolder(folder);
    }
    this->isInit = true;
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
    replaceSeperator(dirP);

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
    for(folder* temp = this->restrictedFolders; temp; temp = temp->next)
        if(!strncmp(path, temp->folder, temp->folderLen))
            return (true);
    return (false);
}

bool Login::save(FILE* dstFile) const
{
    if(!dstFile)
        return (false);
    fwrite(this->username, 1, sizeof(this->username), dstFile);
    fwrite(this->password, 1, sizeof(this->password), dstFile);
    fwrite(&this->state, 1, sizeof(this->state), dstFile);
    fwrite(&this->restrictedFoldersCount, 1, sizeof(this->restrictedFoldersCount), dstFile);
    for(folder* temp = this->restrictedFolders; temp; temp = temp->next)
    {
        fputs(temp->folder, dstFile);
        fputc('\n', dstFile);
    }
    return (true);
}

void Login::print() const
{
    static const char* state_msg[] = {
        "admin",
        "limited",
        "all",
    };
    int i;
    folder* temp;
    printf("{%s, ", this->username);
    for(i = 0; i < MD5_DIGEST_LENGTH; ++i)
        printf("%02x", this->password[i]);
    printf(", %s", state_msg[this->state]);
    if(this->restrictedFolders)
    {
        puts(", [");
        for(temp = this->restrictedFolders; temp; temp = temp->next)
        {
            puts(temp->folder);
            puts(", ");
        }
        puts("\b\b]"); // remove last ", "
    }
    puts("}\n");
}
