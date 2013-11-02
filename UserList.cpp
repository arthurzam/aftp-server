#include "UserList.h"

#include "stdio.h"

UserList::UserList() {
	this->head = NULL;
}

UserList::~UserList() {
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

	printf("        cleared\n");
}

User* UserList::operator[](int index)
{
	ListNode* curr = this->head;
	while(index >= USERS_IN_USERS_ARRAY && curr) // get the node when the current index is hosted in array
	{
		curr = curr->next;
		index -= USERS_IN_USERS_ARRAY;
	}
	if(!curr) // if we exited the bound of list
		return (NULL);
	return (curr->arr[index]);
}

int UserList::operator+(const User &user)
{
	return (this->addUser(user));
}

int UserList::removeUser(int index)
{
	ListNode* curr = this->head;
	while(index >= USERS_IN_USERS_ARRAY && curr) // get the node when the current index is hosted in array
	{
		curr = curr->next;
		index -= USERS_IN_USERS_ARRAY;
	}
	if(!curr) // if we exited the bound of list
		return (EXIT_FAILURE);
	delete curr->arr[index];
	curr->arr[index] = NULL;
	curr->isFull = FALSE;
	return (EXIT_SUCCESS);
}

int UserList::removeUser(const User* user)
{
	int i;
	ListNode* curr = this->head;
	while(curr)
	{
		for (i = 0; i < USERS_IN_USERS_ARRAY; i++)
			if(curr->arr[i] && curr->arr[i]->equals(*user))
			{
				delete curr->arr[i];
				curr->arr[i] = NULL;
				return (EXIT_SUCCESS);
			}
		curr = curr->next;
	}
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
			return (index);
		}

		index += USERS_IN_USERS_ARRAY;
		if(!curr->next) // we didn't found a not full node => we need to create
		{
			curr->next = createNewNode();
			curr = curr->next;
			curr->arr[0] = new User(user);
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
	while(curr)
	{
		for (i = 0; i < USERS_IN_USERS_ARRAY; i++)
			if(curr->arr[i] && curr->arr[i]->equals(user))
				return (index + i);
		curr = curr->next;
		index += USERS_IN_USERS_ARRAY;
	}
	return (-1);
}

User* UserList::findUser(const User &user) const
{
	int i;
	ListNode* curr = this->head;
	while(curr)
	{
		for (i = 0; i < USERS_IN_USERS_ARRAY; i++)
			if(curr->arr[i] && curr->arr[i]->equals(user))
				return (curr->arr[i]);
		curr = curr->next;
	}
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
