#ifndef USER_H_
#define USER_H_

#include <string>
#include <ctime>
#include "defenitions.h"
#include "fileControl.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
using namespace std;

class User {
private:
	struct sockaddr_in* _from;
	string* _folderPath;
	time_t _lastUse;
	bool_t _timeout;
	bool_t _logedIn;
	bool_t _initialized;

	void copyFrom(const struct sockaddr_in* from);
public:
	User();
	User(const struct sockaddr_in* from);
	User(const User& other);
	~User();

	void resetTime();
	void reset(const struct sockaddr_in* from);

	bool_t isTimeout() const;
	void timeout();
	bool_t isLoged() const;
	void logIn();
	const char* folderPath() const;
	struct sockaddr_in* from() const;
	char* getRelativeFile(char* relativeFile);
	/*
	 * changes the folder path using the given path.
	 * moves through every path between slash.
	 * Therefore if there is suddenly a bad part of path, all the parts until the bad part.
	 * possible bad parts of path:
	 *    two slash one after another
	 *    move back (..) when already on root
	 * return (TRUE) if everything is OK, and (FALSE) if error.
	 */
	bool_t moveFolder(char* path);

	bool_t equals(const User& other) const;
	bool_t operator==(const User& other) const;
	bool_t operator!=(const User& other) const;
};

#endif
