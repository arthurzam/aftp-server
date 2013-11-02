#include "fileControl.h"
#include "defenitions.h"
#include <cstdio>
#include <cstring>

bool_t isDirectory(char* directory)
{
	DIR *dir;
	dir = opendir(directory);
	if(!dir)
		return (FALSE);
	closedir (dir);
	return (TRUE);
}

char* getRealDirectory(char* realativDirectory)
{
	char* realDirectory = (char*)malloc(FILENAME_MAX);
	int i;
	strcpy(realDirectory, SERVER_BASE_FOLDER);
	for(i = 0; realativDirectory[i]; i++)
		if(realativDirectory[i] == '/')
			realativDirectory[i] = '\\';
	strcat(realDirectory, (*realativDirectory == '\\' ? realativDirectory + 1 : realativDirectory));
	return (realDirectory);
}

//TODO: complete 'ls' command
string* getContentDirectory(char* directory)
{
	string* result = new string();
	DIR *dir;
	struct dirent *ent;
	dir = opendir (directory);
	if(!dir)
		return (result);
	while ((ent = readdir (dir)) != NULL)
	{

	}
	return (result);
}

bool_t createDirectory(char* realativDirectory)
{
	char* directory = getRealDirectory(realativDirectory);
#ifdef WIN32
	if(CreateDirectory(directory, NULL))
	{
		free(directory);
		return (TRUE);
	}
	free(directory);
	return (FALSE);
#else
	struct stat st = {0};
	if (stat(directory, &st) == -1)
	{
	    if(mkdir(directory, 0700) == 0)
	    {
	    	free(directory);
	    	return (TRUE);
	    }
	}
	free(directory);
	return (FALSE);
#endif
}

bool_t moveFile(char* from, char* to)
{
	return (rename(from, to) == 0);
}

bool_t copyFile(char* from, char* to)
{
	char buffer[0x100];
	FILE* src = fopen(from, "rb");
	FILE* dst;
	int i;

	if(!src)
		return (FALSE);
	dst = fopen(to, "wb");

	while(!feof(src) && (i = fread(buffer, 0x100, 1, src)) == 0x100)
		fwrite(buffer, 0x100, 1, dst);
	fwrite(buffer, i, 1, dst);

	fclose(dst);
	fclose(src);

	return (TRUE);
}

void removeFile(char* path)
{
	remove(path);
}

bool_t isFileExists(char* path)
{
	FILE* file = fopen(path, "rb");
	if(!file)
		return (FALSE);
	fclose(file);
	return (TRUE);
}
