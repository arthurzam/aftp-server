#include "Login.h"

Login::Login(const char* username, const byte_t* password, byte_t state)
{
    strcpy(this->username, username);
    memcpy(this->password, password, MD5_RESULT_LENGTH);
    this->state = state;
    this->restrictedFolders = NULL;
    this->restrictedFoldersCount = 0;
    this->isInit = TRUE;
    this->_next = NULL;
}

Login::Login(FILE* srcFile)
{
    if(srcFile)
    {
        char folder[FILENAME_MAX + 1];
        byte_t i;
        if(fread(this->username, 1, USERNAME_MAX_LENGTH, srcFile) < USERNAME_MAX_LENGTH ||
           fread(this->password, 1, MD5_RESULT_LENGTH, srcFile) < MD5_RESULT_LENGTH ||
           fread(&this->state, 1, sizeof(this->state), srcFile) < sizeof(this->state) ||
           fread(&i, 1, sizeof(i), srcFile) < sizeof(i))
            goto _bad;

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
_bad:
        this->password[0] = 0;
        this->restrictedFolders = NULL;
        this->state = LOGIN_ACCESS_READ_ONLY;
        this->username[0] = 0;
        this->restrictedFoldersCount = 0;
        this->isInit = FALSE;
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
        delete temp;
    }
    if(this->_next)
        delete this->_next;
    this->_next = NULL;
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

void Login::print() const
{
    int i;
    folder* temp;
    printf("{%s, ", this->username);
    for(i = 0; i < MD5_RESULT_LENGTH; ++i)
        printf("%02x", this->password[i]);
    switch(this->state)
    {
        case LOGIN_ACCESS_ADMIN: printf(", admin"); break;
        case LOGIN_ACCESS_ALL: printf(", all"); break;
        case LOGIN_ACCESS_READ_ONLY: printf(", read only"); break;
        case LOGIN_ACCESS_LIMITED: printf(", limited"); break;
        case LOGIN_ACCESS_LIMITED_READ_ONLY: printf(", limited read only"); break;
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
