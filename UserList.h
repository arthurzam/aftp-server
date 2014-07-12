#ifndef AFTP_SRC_USERLIST_H_
#define AFTP_SRC_USERLIST_H_

#include "defenitions.h"

#define USERS_IN_USERS_ARRAY 0x400 // =1024

class User;

class UserList {
private:
    typedef struct _listNode {
        User* arr[USERS_IN_USERS_ARRAY];
        uint16_t count;
        struct _listNode* next;
    } ListNode;

    int userCount;
    int nodesCount;
    ListNode* head;
    mutable bool isSearching;

    static ListNode* createNewNode();
public:
    UserList();
    ~UserList();

    bool removeUser(const User* user);
    User* addUser(const struct sockaddr_in& user);
    User* findUser(const struct sockaddr_in& user) const;

    inline int getUserCount() const
    {
        return (this->userCount);
    }

    void userControl();
    void print() const;
};
#endif
