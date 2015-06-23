#ifndef AFTP_SRC_USERLIST_H_
#define AFTP_SRC_USERLIST_H_

#include "defenitions.h"

class User;

class UserList {
    private:
        static constexpr unsigned USERS_IN_USERS_ARRAY = 0x400; // =1024
        typedef struct _listNode {
            User* arr[USERS_IN_USERS_ARRAY] = {nullptr};
            uint16_t count = 0;
            struct _listNode* next = nullptr;
        } ListNode;

        unsigned userCount = 0;
        unsigned nodesCount = 0;
        ListNode* head = nullptr;
        mutable bool isSearching = false;

        static ListNode* createNewNode()
        {
            return new ListNode;
        }
    public:
        constexpr UserList() = default;

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
