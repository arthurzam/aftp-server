#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "UserList.h"
#include "User.h"

UserList::UserList()
{
    this->head = NULL;
    this->userCount = 0;
    this->nodesCount = 0;
    this->isSearching = false;
}

UserList::~UserList()
{
    this->isSearching = true;
    ListNode* temp = NULL;
    int i;
    while(this->head)
    {
        temp = this->head->next;
        for(i = 0; i < this->head->count; ++i)
        {
            delete this->head->arr[i];
            --this->userCount;
        }
        free(this->head);
        this->head = temp;
    }
    this->head = NULL;
}

bool UserList::removeUser(const User* user)
{
    int i;
    ListNode* curr = this->head;
    this->isSearching = true;
    while(curr)
    {
        for (i = 0; i < this->head->count; ++i)
            if(curr->arr[i]->equals(*user))
            {
                delete curr->arr[i];
                curr->arr[i] = curr->arr[--curr->count];
                --this->userCount;
                this->isSearching = false;
                return (true);
            }
        curr = curr->next;
    }
    this->isSearching = false;
    return (false);
}

User* UserList::addUser(const struct sockaddr_in& user)
{
    ListNode* curr = this->head;
    User* res = NULL;
    this->isSearching = true;
    if(!curr)
    {
        this->head = createNewNode();
        res = this->head->arr[0] = new User(user);
        this->head->count = 1;
        ++this->nodesCount;
        ++this->userCount;
        goto _exit;
    }
    while(curr) // move through all nodes
    {
        if(curr->count < USERS_IN_USERS_ARRAY)
        {
            res = curr->arr[curr->count++] = new User(user);
            ++this->userCount;
            goto _exit;
        }

        if(!curr->next) // we didn't found a not full node => we need to create
        {
            curr->next = createNewNode();
            curr = curr->next;
            res = curr->arr[0] = new User(user);
            curr->count = 1;
            ++this->userCount;
            ++this->nodesCount;
            goto _exit;
        }
        curr = curr->next;
    }
_exit:
    this->isSearching = false;
    return (res);
}

User* UserList::findUser(const struct sockaddr_in& user) const
{
    int i;
    ListNode* curr = this->head;
    this->isSearching = true;
    while(curr)
    {
        for (i = 0; i < curr->count; ++i)
            if(curr->arr[i]->equals(user))
            {
                this->isSearching = false;
                return (curr->arr[i]);
            }
        curr = curr->next;
    }
    this->isSearching = false;
    return (NULL);
}

UserList::ListNode* UserList::createNewNode()
{
    ListNode* result = (ListNode*)malloc(sizeof(ListNode));
    memset(result, 0, sizeof(ListNode));
    return (result);
}

void UserList::userControl()
{
    const int CLEAR_AFTER_COUNT = 128;
    int i;
    ListNode* curr;
    if(this->userCount < CLEAR_AFTER_COUNT) // check if we need to check because there is no matter to check when there is less that a little number of users
        return;
    for(curr = this->head; curr; curr = curr->next)
    {
        while(this->isSearching);
        for (i = 0; i < curr->count; ++i)
        {
            if(curr->arr[i] && curr->arr[i]->timeout())
            {
                --this->userCount;
                delete curr->arr[i];
                curr->arr[i] = curr->arr[--curr->count];
            }
        }
    }
}

void UserList::print() const
{
    int i;
    ListNode* curr;
    for(curr = this->head; curr; curr = curr->next)
        for (i = 0; i < curr->count; ++i)
            curr->arr[i]->print();
}
