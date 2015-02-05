#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "UserList.h"
#include "User.h"

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
        delete this->head;
        this->head = temp;
    }
    this->head = nullptr;
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
    User** res = nullptr;
    this->isSearching = true;
    if(!curr)
    {
        this->head = createNewNode();
        res = &this->head->arr[0];
        this->head->count = 1;
        ++this->nodesCount;
        ++this->userCount;
        goto _exit;
    }
    while(curr) // move through all nodes
    {
        if(curr->count < USERS_IN_USERS_ARRAY)
        {
            res = &curr->arr[curr->count++];
            ++this->userCount;
            goto _exit;
        }

        if(!curr->next) // we didn't found a not full node => we need to create
        {
            curr->next = createNewNode();
            curr = curr->next;
            res = &curr->arr[0];
            curr->count = 1;
            ++this->userCount;
            ++this->nodesCount;
            goto _exit;
        }
        curr = curr->next;
    }
_exit:
    if(res)
        *res = new (std::nothrow) User(user);
    this->isSearching = false;
    return (*res);
}

User* UserList::findUser(const struct sockaddr_in& user) const
{
    int i;
    const ListNode* curr = this->head;
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
    return (nullptr);
}

void UserList::userControl()
{
    static constexpr unsigned CLEAR_AFTER_COUNT = 128;
    unsigned i;
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
    for(const ListNode* curr = this->head; curr; curr = curr->next)
        for (i = 0; i < curr->count; ++i)
            curr->arr[i]->print();
}
