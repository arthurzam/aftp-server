#include "Login.h"

Login::Login(const char* username, const byte_t* password, LOGIN_ACCESS state)
{
    strcpy(this->username, username);
    memcpy(this->password, password, MD5_RESULT_LENGTH);
    this->state = state;
    this->restrictedFolders = NULL;
    this->restrictedFoldersCount = 0;
    this->isInit = TRUE;
}

Login::Login(FILE* srcFile)
{
    if(srcFile)
    {
        char folder[FILENAME_MAX + 1];
        byte_t i;
        fread(this->username, 1, USERNAME_MAX_LENGTH, srcFile);
        fread(this->password, 1, MD5_RESULT_LENGTH, srcFile);
        fread(&this->state, 1, sizeof(this->state), srcFile);
        fread(&i, 1, sizeof(i), srcFile);

        this->restrictedFolders = NULL;
        this->restrictedFoldersCount = 0;
        for(; i; --i)
        {
            fgets(folder, FILENAME_MAX + 1, srcFile);
            this->addRestrictedFolder(folder);
        }
        this->isInit = TRUE;
    }
    else
    {
        this->password[0] = 0;
        this->restrictedFolders = NULL;
        this->state = LOGIN_ACCESS_READ_ONLY;
        this->username[0] = 0;
        this->restrictedFoldersCount = 0;
        this->isInit = FALSE;
    }
}

Login::~Login()
{
    folder* temp;
    while(this->restrictedFolders)
    {
        temp = this->restrictedFolders;
        this->restrictedFolders = this->restrictedFolders->next;
        delete temp;
    }
}

bool_t Login::check(const char* username, const byte_t* password) const
{
    return (!(memcmp(this->password, password, MD5_RESULT_LENGTH) || strcmp(this->username, username)));
}

void Login::addRestrictedFolder(const char* dir)
{
    folder* newOne = (folder*)malloc(sizeof(folder));
    folder* temp = this->restrictedFolders;
    memcpy(newOne->folder, dir, FILENAME_MAX);
    newOne->folder[FILENAME_MAX] = 0;
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

bool_t Login::isRestrictedFolder(const char* path) const
{
    if(this->state == LOGIN_ACCESS_ADMIN)
        return (FALSE);
    folder* temp;
    for(temp = this->restrictedFolders; temp; temp = temp->next)
    {
        if(!strncmp(path, temp->folder, temp->folderLen))
            return (TRUE);
    }
    return (FALSE);
}

bool_t Login::save(FILE* dstFile) const
{
    if(!dstFile)
        return (FALSE);
    fwrite(this->username, 1, USERNAME_MAX_LENGTH, dstFile);
    fwrite(this->password, 1, MD5_RESULT_LENGTH, dstFile);
    fwrite(&this->state, 1, sizeof(this->state), dstFile);
    fwrite(&this->restrictedFoldersCount, 1, sizeof(this->restrictedFoldersCount), dstFile);
    folder* temp;
    for(temp = this->restrictedFolders; temp; temp = temp->next)
    {
        fputs(temp->folder, dstFile);
        fputc('\n', dstFile);
    }
    return (TRUE);
}
