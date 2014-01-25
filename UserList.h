#ifndef AFTP_SRC_USERLIST_H_
#define AFTP_SRC_USERLIST_H_

#include "User.h"
#include "defenitions.h"

#define USERS_IN_USERS_ARRAY 0x400 // =1024

class User;

struct _listNode {
	User* arr[USERS_IN_USERS_ARRAY];
	int isFull;
	struct _listNode* next;
};
typedef struct _listNode ListNode;

class UserList {

private:
	int userCount;
	int nodesCount;
	ListNode* head;
	ListNode* createNewNode();
public:
	UserList();
	~UserList();

	int removeUser(int index);
	int removeUser(const User* user);
	int addUser(const User &user);
	int findIndexOfUser(const User &user) const;
	User* findUser(const User &user) const;
	unsigned int getUserCount() const;

	User* operator[](int index);
	int operator+(const User &user);
};
#endif
