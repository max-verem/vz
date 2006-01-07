/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2005 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2005.

    ViZualizator is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ViZualizator is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ViZualizator; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

ChangeLog:
    2005-07-08: Code cleanup

*/


#ifndef VZHASH_HPP
#define VZHASH_HPP

//
#include <stdlib.h>
#include <string.h>

template <class T>
class vzHash
{
private:
	T* _values;
	char** _keys;
	unsigned int _count;
public:
	vzHash();
	int push(char* key,T element);
	T find(char* key);
	T value(unsigned int index);
	char* key(unsigned int index);
	unsigned int count();
	~vzHash();
};

template <class T>
vzHash<T>::vzHash()
{
	_values =(T*)malloc(0);
	_keys = (char**) malloc(0);
	_count = 0;
};

template <class T>
vzHash<T>::~vzHash()
{
	for(unsigned int i=0;i<_count;i++)
	{
		free(_keys[i]);
	};
	free(_keys);
	free(_values);
};


template <class T>
int vzHash<T>::push(char* key, T element)
{
	// check if element with such key present
	for(unsigned int i=0;i<_count;i++)
		if(strcmp(_keys[i],key) == 0)
		{
			// element present - modify entry
			_values[i] = element;
			return 0;
		}

	// realloc space and setup values
	_values = (T*)realloc(_values,(_count+1) * sizeof(T));
	_values[_count] = element;

	_keys = (char**) realloc (_keys, (_count + 1) * sizeof(char*));
	_keys[_count] = strcpy((char*)malloc(strlen(key) + 1), key);

	return ++_count;
};

template <class T>
T vzHash<T>::find(char* key)
{
	for(unsigned int i=0;i<_count;i++)
		if(strcmp(_keys[i],key) == 0)
			return _values[i];
	return NULL;
};

template <class T>
T vzHash<T>::value(unsigned int i)
{
	return (i<_count)?_values[i]:NULL;
	
};

template <class T>
char* vzHash<T>::key(unsigned int i)
{
	return (i<_count)?_keys[i]:NULL;
};

template <class T>
inline unsigned int vzHash<T>::count()
{
	return _count;
};

#endif