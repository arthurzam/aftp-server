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

bool_t moveFile(char* from, char* to, User* user)
{
	char* src = user->getRealFile(from);
	char* dst = user->getRealFile(to);
	int r = rename(src, dst);
	free(src);
	free(dst);
	return (r == 0);
}

bool_t copyFile(char* from, char* to, User* user)
{
	char* srcS = user->getRealFile(from);
	char* dstS = user->getRealFile(to);
	char buffer[0x100];
	FILE* src = fopen(srcS, "rb");
	FILE* dst;
	int i;

	if(!src)
		goto _badExit;
	dst = fopen(dstS, "wb");

	if(!dst)
	{
		fclose(src);
		goto _badExit;
	}

	while ((i = fread(buffer, 0x100, 1, src)))
		fwrite(buffer, i, 1, dst);

	fclose(dst);
	fclose(src);
	return (TRUE);

_badExit:
	free(srcS);
	free(dstS);
	return (FALSE);
}

bool_t removeFile(char* path, User* user)
{
	char* src = user->getRealFile(path);
	int r = remove(src);
	free(src);
	return (r == 0);
}

bool_t removeFolder(char* path, User* user)
{
#ifdef WIN32
	char command[FILENAME_MAX] = "rd /q /s ";
#else
	char command[FILENAME_MAX] = "rm -r -f ";
#endif
	bool_t flag = TRUE;
	//TODO: build this function
	char* src = user->getRealFile(path);
	if (!isDirectory(src))
	{
		flag = FALSE;
		goto _exit;
	}
	strcat (command, src);
	printf("              %s\n", command);
	system (command);
	if (isDirectory(src))
		flag = FALSE;
_exit:
	free(src);
	return (flag);
}

bool_t copyFolder(char* from, char* to, User* user)
{
#ifdef WIN32
	char command[FILENAME_MAX] = "xcopy /s /e /h ";
#else
	char command[FILENAME_MAX] = "cp -r -f ";
#endif
	bool_t flag = TRUE;
	//TODO: build this function
	char* src = user->getRealFile(from);
	char* dst = user->getRealFile(to);
	if (!isDirectory(src) || isDirectory(dst))
	{
		flag = FALSE;
		goto _exit;
	}
	strcat (command, src);
	strcat (command, " ");
	strcat (command, dst);
	printf("              %s\n", command);
	system (command);
	if (isDirectory(src))
		flag = FALSE;
_exit:
	free(src);
	free(dst);
	return (flag);
}

bool_t isFileExists(char* path)
{
	FILE* file = fopen(path, "rb");
	if(!file)
		return (FALSE);
	fclose(file);
	return (TRUE);
}
