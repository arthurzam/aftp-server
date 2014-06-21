#include "UserList.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

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
        for(i = 0; i < USERS_IN_USERS_ARRAY && this->userCount != 0; ++i)
            if(this->head->arr[i])
            {
                delete this->head->arr[i];
                --this->userCount;
            }
        delete this->head;
        this->head = temp;
    }
    this->head = NULL;
}

int UserList::removeUser(const User* user)
{
    int i;
    ListNode* curr = this->head;
    this->isSearching = true;
    while(curr)
    {
        for (i = 0; i < USERS_IN_USERS_ARRAY; ++i)
            if(curr->arr[i] && curr->arr[i]->equals(*user))
            {
                delete curr->arr[i];
                curr->arr[i] = NULL;
                --this->userCount;
                this->isSearching = false;
                return (EXIT_SUCCESS);
            }
        curr = curr->next;
    }
    this->isSearching = false;
    return (EXIT_FAILURE);
}

User* UserList::addUser(const struct sockaddr_in& user)
{
    ListNode* curr = this->head;
    User* res = NULL;
    int i;
    this->isSearching = true;
    if(!curr)
    {
        this->head = createNewNode();
        res = this->head->arr[0] = new User(user);
        ++this->nodesCount;
        ++this->userCount;
        goto _exit;
    }
    while(curr) // move through all nodes
    {
        if(!curr->isFull)
        {
            curr->isFull = true;
            for(i = 0; i < USERS_IN_USERS_ARRAY; ++i) // find an empty spot on the current array
            {
                if(!curr->arr[i]) // is empty (==NULL)
                {
                    res = curr->arr[i] = new User(user);
                    break;
                }
            }
            for( ; i < USERS_IN_USERS_ARRAY; ++i) // move through all other to check if there is another empty spot
            {
                if(!curr->arr[i]) // is empty?
                {
                    curr->isFull = false; // update the current node flag, that isn't full
                    break;
                }
            }
            ++this->userCount;
            goto _exit;
        }

        if(!curr->next) // we didn't found a not full node => we need to create
        {
            curr->next = createNewNode();
            curr = curr->next;
            res = curr->arr[0] = new User(user);
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
        for (i = 0; i < USERS_IN_USERS_ARRAY; ++i)
            if(curr->arr[i] && curr->arr[i]->equals(user))
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
    int i, usersPassed = 0;
    ListNode* curr;
    if(this->userCount < CLEAR_AFTER_COUNT) // check if we need to check because there is no matter to check when there is less that a little number of users
        return;
    for(curr = this->head; curr && usersPassed != this->userCount; curr = curr->next)
    {
        while(this->isSearching);
        for (i = 0; i < USERS_IN_USERS_ARRAY && usersPassed != this->userCount; ++i)
        {
            if(curr->arr[i] && curr->arr[i]->timeout())
            {
                --this->userCount;
                delete curr->arr[i];
                curr->arr[i] = NULL;
                curr->isFull = false;
            }
            else
                ++usersPassed;
        }
    }
}

void UserList::print() const
{
    int i;
    ListNode* curr;
    for(curr = this->head; curr; curr = curr->next)
        for (i = 0; i < USERS_IN_USERS_ARRAY; ++i)
            if(curr->arr[i])
                curr->arr[i]->print();
}
