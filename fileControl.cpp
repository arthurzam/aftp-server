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

char* getRealDirectory(char* realativDirectory, char* result)
{
	char* realDirectory;
	if(result)
		realDirectory = result;
	else
		realDirectory = (char*)malloc(FILENAME_MAX);
	int i;
	strcpy(realDirectory, SERVER_BASE_FOLDER);
	for(i = 0; realativDirectory[i]; i++)
		if(realativDirectory[i] == PATH_SEPERATOR_BAD)
			realativDirectory[i] = PATH_SEPERATOR_GOOD;
	strcat(realDirectory, (*realativDirectory == PATH_SEPERATOR_GOOD ? realativDirectory + 1 : realativDirectory));
	return (result ? NULL : realDirectory);
}

#define FILE_LIST_SEPARATOR '|'
bool_t getContentDirectory(char* directory, User* user)
{
	char* base = user->getRealFile(directory);
	int dirLen = strlen(base);
    if(base[dirLen - 1] != PATH_SEPERATOR_GOOD)
    {
        base[dirLen++] = PATH_SEPERATOR_GOOD;
        base[dirLen] = 0;
    }

	int resLen = 0;
	char result[BUFFER_SERVER_SIZE + 5] = {0};
	result[BUFFER_SERVER_SIZE + 4] = 0; // just to be safe from BUFFER_OVERFLOW

	DIR *dir;
	struct dirent *ent;
	dir = opendir (base);
	if(!dir)
		goto _bad_exit;
	bool_t flag;
	int fileNameLen;
	while ((ent = readdir (dir)) != NULL)
	{
		if(ent->d_name[0] == '.' && (ent->d_name[1] == 0 || (ent->d_name[1] == '.' && ent->d_name[2] == 0))) // the path is not "." or ".."
			continue;
		fileNameLen = strlen(ent->d_name);
		if(resLen + fileNameLen >= BUFFER_SERVER_SIZE - 10)
		{
			sendMessage(user->from(), 201, result, resLen);
			result[0] = 0;
			resLen = 0;
		}
		strcpy(base + dirLen, ent->d_name);
		flag = isDirectory(base);
		if(flag)
		{
			result[resLen++] = '[';
			result[resLen] =  0;
		}
		strcat(result + resLen, ent->d_name);
		resLen += fileNameLen;
		if(flag)
		{
			result[resLen++] = ']';
			result[resLen] =  0;
		}
		result[resLen++] = FILE_LIST_SEPARATOR;
		result[resLen] =  0;
	}
	closedir (dir);
	if(resLen)
		sendMessage(user->from(), 201, result, resLen);
	free(base);
	return (TRUE);
_bad_exit:
	free(base);
	return (FALSE);
}

bool_t createDirectory(char* realativDirectory)
{
	char* directory = getRealDirectory(realativDirectory, NULL);
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

long getFilesize(char* path, User* user)
{
	char* src = user->getRealFile(path);
	long r = -1;
#ifdef WIN32
	FILE* srcF = fopen(src, "r");
	if(srcF)
	{
		fseek(srcF, 0, SEEK_END);
		r = ftell(srcF);
	}
	fclose(srcF);
#else
	struct stat st;
	if(stat(src, &st) >= 0)
		r = st.st_size;
#endif
	free(src);
	return (r == 0);
}

bool_t getMD5OfFile(char* path, User* user, byte_t result[MD5_RESULT_LENGTH])
{
	char* filePath = user->getRealFile(path);
	md5_context ctx;
	int i;
	byte_t buffer[512];

	FILE* f = fopen(filePath, "rb");
	if(!f)
	{
		free(filePath);
		return (FALSE);
	}
	md5_init(&ctx);
	while((i = fread(buffer, 1, 512, f)))
	{
		md5_append(&ctx, buffer, i);
	}
	md5_finish(&ctx, result);
	free(filePath);
	return (TRUE);
}

bool_t removeFolder(char* path, User* user)
{
#ifdef WIN32
	char command[FILENAME_MAX] = "rd /q /s ";
#else
	char command[FILENAME_MAX] = "rm -r -f ";
#endif
	bool_t flag = TRUE;
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
