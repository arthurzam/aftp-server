#ifndef AFTP_SRC_USERLIST_H_
#define AFTP_SRC_USERLIST_H_

#include "User.h"
#include "defenitions.h"
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define USERS_IN_USERS_ARRAY 0x400 // =1024

class User;

struct _listNode {
	User* arr[USERS_IN_USERS_ARRAY];
	bool_t isFull;
	struct _listNode* next;
};
typedef struct _listNode ListNode;

class UserList {

private:
	int userCount;
	int nodesCount;
	ListNode* head;
	ListNode* createNewNode();
	bool_t isSearching;
public:
	UserList();
	~UserList();

	int removeUser(int index);
	int removeUser(const User* user);
	int addUser(const User &user);
	int findIndexOfUser(const User &user);
	User* findUser(const User &user);
	int getUserCount() const;

	User* operator[](int index);
	int operator+(const User &user);
	
	void userControl();
    void print() const;
};
#endif
