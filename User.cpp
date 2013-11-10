#include <cstring>
#include "User.h"

void User::copyFrom(const struct sockaddr_in* from)
{
	int i;
	this->_from = new struct sockaddr_in;
	this->_from->sin_family = from->sin_family;
	this->_from->sin_port   = from->sin_port;
#ifdef WIN32
	this->_from->sin_addr.S_un.S_addr = from->sin_addr.S_un.S_addr;
#else
	this->_from->sin_addr.s_addr = from->sin_addr.s_addr;
#endif	
	for(i = 0; i < 8; i++)
		this->_from->sin_zero[i] = from->sin_zero[i];
}

User::User()
{
	this->_folderPath = NULL;
	this->_from = NULL;
	this->_lastUse = 0;
	this->_logedIn = FALSE;
	this->_timeout = FALSE;
	this->_initialized = FALSE;
}

User::User(const struct sockaddr_in* from)
{
	this->_folderPath = new string("");
	copyFrom(from);
	this->_lastUse = time(NULL);
	this->_logedIn = FALSE;
	this->_timeout = FALSE;
	this->_initialized = TRUE;
}

User::User(const User& other)
{
	this->_folderPath = new string(*other._folderPath);
	copyFrom(other._from);
	this->_lastUse = time(NULL);
	this->_logedIn = FALSE;
	this->_timeout = FALSE;
	this->_initialized = TRUE;
}

User::~User() {
	if(_folderPath)
		delete _folderPath;
	if(_from)
		delete _from;
}

void User::reset(const struct sockaddr_in* from)
{
	if(_folderPath)
		delete _folderPath;
	if(_from)
		delete _from;
	this->_folderPath = new string("/");
	copyFrom(from);
	this->_lastUse = time(NULL);
	this->_logedIn = FALSE;
	this->_timeout = FALSE;
	this->_initialized = TRUE;
}

void User::resetTime()
{
	this->_lastUse = time(NULL);
}

bool_t User::equals(const User& other) const
{
	if(other._from->sin_port != this->_from->sin_port)
		return (FALSE);
#ifdef WIN32
	if(other._from->sin_addr.S_un.S_addr != this->_from->sin_addr.S_un.S_addr)
#else
	if(other._from->sin_addr.s_addr != this->_from->sin_addr.s_addr)
#endif
		return (FALSE);
	return (TRUE);
}

bool_t User::operator==(const User& other) const
{
	return (this->equals(other));
}

bool_t User::operator!=(const User& other) const
{
	return (!this->equals(other));
}

bool_t User::moveFolder(char* path)
{
	if(*path == 0)
		return (TRUE);
	int i;
	int last;
	char* newStr = NULL;

	// replace all \ into /
	for(i = 0; path[i] != 0; i++)
	{
		if(path[i] == '\\')
			path[i] = '/';
		if(i > 0 && path[i] == '/' && path[i - 1] == '/') // we have two slashes one by one
			return (FALSE);
	}
	if(path[i - 1] == '/') // we can use i-1 because path[i] after the for is the '/0' ending
		path[i - 1] = 0; // if the path finishes in divider, remove it

	if(*path == '/') // for example /path/folder => just replace the current
	{
		if(this->_folderPath)
			delete (this->_folderPath);
		this->_folderPath = new string(path);
		goto _fin;
	}

	newStr = (char*)malloc(FILENAME_MAX);
	if(this->_folderPath)
	{
		strcpy(newStr, this->_folderPath->c_str());
	}
	else
	{
		newStr[0] = '/';
		newStr[1] = 0;
	}

_check_path:
	if(!*path)
		goto _fin;
	if(*path == '.' && path[1] == '.')
	{
		if(newStr[1] == 0)
			return (FALSE); // trying to move back when we are already on root
		for(i = 0; newStr[i] != 0; i++)
			if(newStr[i] == '/')
				last = i; // find the last occurrence of '/'
		newStr[last] = 0;
		if(path[2] == 0)
			goto _fin;
		path += 3;
		goto _check_path;
	}

	for(i = 0; path[i] != 0 && path[i] != '/'; i++) ; // find the first occurrence of '/'
	last = path[i];
	path[i] = 0;
	strcat(newStr, "/");
	strcat(newStr, path);
	path[i] = '/';
	path += i + 1;
	if(last != 0)
		goto _check_path;

_fin:
	if(newStr)
	{
		if(this->_folderPath)
			delete (this->_folderPath);
		this->_folderPath = new string(newStr);
		free(newStr);
	}
	return (TRUE);
}

bool_t User::isLoged() const
{
	return (this->_logedIn);
}

void User::logIn()
{
	this->_logedIn = TRUE;
}

const char* User::folderPath() const
{
	return (this->_folderPath->c_str());
}

struct sockaddr_in* User::from() const
{
	return (this->_from);
}

char* User::getRealFile(char* relativeFile)
{
	char backup[FILENAME_MAX];
	char* res;
	strcpy(backup, this->_folderPath->c_str());

	if(!this->moveFolder(relativeFile))
	{
		res = NULL;
		goto _exit;
	}
	res = getRealDirectory((char*)this->_folderPath->c_str());
_exit:
	delete this->_folderPath;
	this->_folderPath = new string(backup);
	return (res);
}
