/*
  Copyright (C) 1997-2000 Shigeru Chiba, University of Tsukuba.

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/
/*
  Copyright (c) 1995, 1996 Xerox Corporation.
  All Rights Reserved.

  Use and copying of this software and preparation of derivative works
  based upon this software are permitted. Any copy of this software or
  of any derivative work must include the above copyright notice of
  Xerox Corporation, this paragraph and the one after it.  Any
  distribution of this software or derivative works must comply with all
  applicable United States export control laws.

  This software is made available AS IS, and XEROX CORPORATION DISCLAIMS
  ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE, AND NOTWITHSTANDING ANY OTHER PROVISION CONTAINED HEREIN, ANY
  LIABILITY FOR DAMAGES RESULTING FROM THE SOFTWARE OR ITS USE IS
  EXPRESSLY DISCLAIMED, WHETHER ARISING IN CONTRACT, TORT (INCLUDING
  NEGLIGENCE) OR STRICT LIABILITY, EVEN IF XEROX CORPORATION IS ADVISED
  OF THE POSSIBILITY OF SUCH DAMAGES.
*/

//  Hash Table

#ifndef _HashTable_hh
#define _HashTable_hh

#include <iosfwd>
#include "types.h"

typedef void* HashValue;
struct HashTableEntry;

class HashTable : public LightObject {
public:
    HashTable();
    HashTable(int) {}
    void MakeTable();
    bool IsEmpty();
    void Dump(std::ostream&);
    int AddEntry(const char* key, HashValue value, int* index = 0);
    int AddEntry(bool, const char* key, int len, HashValue value, int* index = 0);

    int AddEntry(const char* key, int len, HashValue value, int* index = 0) {
	return AddEntry(true, key, len, value, index);
    }

    // allow a duplicated entry to be inserted
    int AddDupEntry(const char* key, int len, HashValue value, int* index = 0) {
	return AddEntry(false, key, len, value, index);
    }

    bool Lookup(const char* key, HashValue* value);
    bool Lookup(const char* key, int len, HashValue* value);
    bool LookupEntries(const char* key, int len, HashValue* value, int& nth);
    HashValue Peek(int index);
    bool RemoveEntry(const char* key);
    bool RemoveEntry(const char* key, int len);
    void ReplaceValue(int index, HashValue value);

protected:
    char* KeyString(const char* key);
    char* KeyString(const char* key, int len);

    bool Lookup2(const char* key, HashValue* val, int* index);
    bool Lookup2(const char* key, int len, HashValue* val, int* index);
    static uint NextPrimeNumber(uint number);
    bool GrowTable(int increment);
    unsigned int StringToInt(const char*);
    unsigned int StringToInt(const char*, int);
    int HashFunc(unsigned int p, int n);

protected:
    HashTableEntry	*entries;
    int			Size;	// the max number of entries.
				// should be a prime number
    int			Prime2;
};

class BigHashTable : public HashTable {
public:
    BigHashTable();
};

#endif
