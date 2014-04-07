#include "UserList.h"

#include "stdio.h"

UserList::UserList()
{
    this->head = NULL;
    this->userCount = 0;
    this->nodesCount = 0;
    this->isSearching = FALSE;
}

UserList::~UserList()
{
    this->isSearching = TRUE;
    ListNode* temp = NULL;
    int i;
    while(this->head)
    {
        temp = this->head->next;
        for(i = 0; i < USERS_IN_USERS_ARRAY; i++)
            if(this->head->arr[i])
                delete this->head->arr[i];
        delete this->head;
        this->head = temp;
    }
    this->head = NULL;
}

User* UserList::operator[](int index) const
{
    ListNode* curr = this->head;
    if ((this->nodesCount << 10) < index)
        return (NULL);
    while(index >= USERS_IN_USERS_ARRAY && curr) // get the node when the current index is hosted in array
    {
        curr = curr->next;
        index -= USERS_IN_USERS_ARRAY;
    }
    if(!curr) // if we exited the bound of list
        return (NULL);
    return (curr->arr[index]);
}

int UserList::operator+=(const User &user)
{
    return (this->addUser(user));
}

int UserList::removeUser(int index)
{
    ListNode* curr = this->head;
    if ((this->nodesCount << 10) < index)
        return (EXIT_FAILURE);
    while(index >= USERS_IN_USERS_ARRAY && curr) // get the node when the current index is hosted in array
    {
        curr = curr->next;
        index -= USERS_IN_USERS_ARRAY;
    }
    if(!curr) // if we exited the bound of list
        return (EXIT_FAILURE);

    this->isSearching = TRUE;
    delete curr->arr[index];
    curr->arr[index] = NULL;
    curr->isFull = FALSE;
    this->userCount--;
    this->isSearching = FALSE;

    return (EXIT_SUCCESS);
}

int UserList::removeUser(const User* user)
{
    int i;
    ListNode* curr = this->head;
    this->isSearching = TRUE;
    while(curr)
    {
        for (i = 0; i < USERS_IN_USERS_ARRAY; i++)
            if(curr->arr[i] && curr->arr[i]->equals(*user))
            {
                delete curr->arr[i];
                curr->arr[i] = NULL;
                this->userCount--;
                this->isSearching = FALSE;
                return (EXIT_SUCCESS);
            }
        curr = curr->next;
    }
    this->isSearching = FALSE;
    return (EXIT_FAILURE);
}

int UserList::addUser(const User &user)
{
    ListNode* curr = this->head;
    int index = 0;
    int i;
    if(!curr)
    {
        this->head = createNewNode();
        this->head->arr[0] = new User(user);
        this->nodesCount++;
        return (0);
    }
    while(curr) // move through all nodes
    {
        if(!curr->isFull)
        {
            curr->isFull = TRUE;
            for(i = 0; i < USERS_IN_USERS_ARRAY; i++) // find an empty spot on the current array
            {
                if(!curr->arr[i]) // is empty (==NULL)
                {
                    curr->arr[i] = new User(user); // set
                    index += i;
                    break;
                }
            }
            for( ; i < USERS_IN_USERS_ARRAY; i++) // move through all other to check if there is another empty spot
            {
                if(!curr->arr[i]) // is empty?
                {
                    curr->isFull = FALSE; // update the current node flag, that isn't full
                    break;
                }
            }
            this->userCount++;
            return (index);
        }

        this->isSearching = TRUE;
        index += USERS_IN_USERS_ARRAY;
        if(!curr->next) // we didn't found a not full node => we need to create
        {
            curr->next = createNewNode();
            curr = curr->next;
            curr->arr[0] = new User(user);
            this->userCount++;
            this->nodesCount++;
            this->isSearching = FALSE;
            return (index);
        }
        curr = curr->next;
    }
    return (-1);
}

int UserList::findIndexOfUser(const User &user) const
{
    int index = 0, i;
    ListNode* curr = this->head;
    this->isSearching = TRUE;
    while(curr)
    {
        for (i = 0; i < USERS_IN_USERS_ARRAY; i++)
            if(curr->arr[i] && curr->arr[i]->equals(user))
            {
                this->isSearching = FALSE;
                return (index + i);
            }
        curr = curr->next;
        index += USERS_IN_USERS_ARRAY;
    }
    this->isSearching = FALSE;
    return (-1);
}

User* UserList::findUser(const User &user) const
{
    int i;
    ListNode* curr = this->head;
    this->isSearching = TRUE;
    while(curr)
    {
        for (i = 0; i < USERS_IN_USERS_ARRAY; i++)
            if(curr->arr[i] && curr->arr[i]->equals(user))
            {
                this->isSearching = FALSE;
                return (curr->arr[i]);
            }
        curr = curr->next;
    }
    this->isSearching = FALSE;
    return (NULL);
}

ListNode* UserList::createNewNode()
{
    ListNode* result = new ListNode;
    int i;
    result->isFull = FALSE;
    result->next = NULL;
    for(i = 0; i < USERS_IN_USERS_ARRAY; i++)
    {
        result->arr[i] = NULL;
    }
    return (result);
}

void UserList::userControl()
{
    const int CLEAR_AFTER_COUNT = 128;
    int i;
    ListNode* curr;
    if(this->userCount >= CLEAR_AFTER_COUNT) // check if we need to check because there is no matter to check when there is less that a little number of users
        for(curr = this->head; curr; curr = curr->next)
        {
            for (i = 0; i < USERS_IN_USERS_ARRAY; i++)
                while(this->isSearching);
            if(curr->arr[i] && curr->arr[i]->timeout())
            {
                this->userCount--;
                delete curr->arr[i];
                curr->arr[i] = NULL;
                curr->isFull = FALSE;
            }
        }
}

int UserList::getUserCount() const
{
    return (this->userCount);
}

void UserList::print() const
{
    int i;
    ListNode* curr;
    for(curr = this->head; curr; curr = curr->next)
    {
        for (i = 0; i < USERS_IN_USERS_ARRAY; i++)
            if(curr->arr[i])
            {
                curr->arr[i]->print();
            }
    }
}
