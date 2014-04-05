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
	char base[FILENAME_MAX];
	user->getRealFile(directory, base);
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
		return (FALSE);
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
	return (TRUE);
}

bool_t createDirectory(char* realativDirectory, User* user)
{
	char directory[FILENAME_MAX];
	user->getRealFile(realativDirectory, directory);
#ifdef WIN32
	if(CreateDirectory(directory, NULL))
	{
		return (TRUE);
	}
	return (FALSE);
#else
	struct stat st = {0};
	if (stat(directory, &st) == -1)
	{
	    if(mkdir(directory, 0700) == 0)
	    {
	    	return (TRUE);
	    }
	}
	return (FALSE);
#endif
}

bool_t moveFile(char* from, char* to, User* user)
{
	char src[FILENAME_MAX], dst[FILENAME_MAX];
	user->getRealFile(from, src);
	user->getRealFile(to, dst);
	return (rename(src, dst) == 0);
}

bool_t copyFile(char* from, char* to, User* user)
{
	bool_t flag = TRUE;
	char srcS[FILENAME_MAX], dstS[FILENAME_MAX];
	user->getRealFile(from, srcS);
	user->getRealFile(to, dstS);
#ifdef WIN32
	flag = CopyFileA(srcS, dstS, 0);
#else
	char buffer[0x100];
	FILE* src = fopen(srcS, "rb");
	FILE* dst = NULL;
	int i;

	if(!src)
	{
		flag = FALSE;
		goto _exit;
	}
	dst = fopen(dstS, "wb");

	if(!dst)
	{
		flag = FALSE;
		goto _exit;
	}

	while ((i = fread(buffer, 0x100, 1, src)))
		fwrite(buffer, i, 1, dst);

_exit:
	fclose(dst);
	fclose(src);
#endif
	return (flag);
}

bool_t removeFile(char* path, User* user)
{
	char src[FILENAME_MAX];
	user->getRealFile(path, src);
	return (remove(src) == 0);
}

long getFilesize(char* path, User* user)
{
	char src[FILENAME_MAX];
	user->getRealFile(path, src);
#ifdef WIN32
	HANDLE MF = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	DWORD High;
	if ( MF != INVALID_HANDLE_VALUE )
	{
		return (GetFileSize(MF, &High));
	}
#else
	struct stat st;
	if(stat(src, &st) >= 0)
		return (st.st_size);
#endif
	return (-1);
}

bool_t getMD5OfFile(char* path, User* user, byte_t result[MD5_RESULT_LENGTH])
{
	char filePath[FILENAME_MAX];
	user->getRealFile(path, filePath);
	md5_context ctx;
	int i;
	byte_t buffer[512];

	FILE* f = fopen(filePath, "rb");
	if(!f)
	{
		return (FALSE);
	}
	md5_init(&ctx);
	while((i = fread(buffer, 1, 512, f)))
	{
		md5_append(&ctx, buffer, i);
	}
	md5_finish(&ctx, result);
	return (TRUE);
}

bool_t removeFolder(char* path, User* user)
{
#ifdef WIN32
	char command[FILENAME_MAX + 9] = "rd /q /s ";
	const int baseCommandLen = 9;
#else
	char command[FILENAME_MAX + 9] = "rm -r -f ";
	const int baseCommandLen = 9;
#endif
	user->getRealFile(path, command + baseCommandLen); // put the full path in the command buffer
	if (!isDirectory(command + baseCommandLen))
		return (FALSE);
	system (command);
	if (isDirectory(command + baseCommandLen))
		return (FALSE);
	return (TRUE);
}

bool_t copyFolder(char* from, char* to, User* user)
{
#ifdef WIN32
	char command[2 * FILENAME_MAX + 20] = "xcopy /E /H /Y /i ";
	const int baseCommandLen = 18;
#else
	char command[2 * FILENAME_MAX + 17] = "cp -r -f ";
	const int baseCommandLen = 9;
#endif
	int i;
	user->getRealFile(from, command + baseCommandLen);
	if(!isDirectory(command + baseCommandLen))
		return (FALSE);
	i = strlen(command);
	command[i++] = ' ';
	user->getRealFile(to, command + i);
	if (isDirectory(command + i))
		return (FALSE);
	system (command);
	command[i - 1] = 0;
	return (isDirectory(command + i)); // did create directory?
}

bool_t isFileExists(char* path)
{
	FILE* file = fopen(path, "rb");
	if(!file)
		return (FALSE);
	fclose(file);
	return (TRUE);
}
