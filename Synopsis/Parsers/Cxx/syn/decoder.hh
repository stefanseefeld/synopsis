// Synopsis C++ Parser: decoder.hh header file
// Decodes the names and types encoded by OCC

// $Id: decoder.hh,v 1.14 2002/11/17 12:11:43 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2002 Stephen Davies
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#ifndef H_SYNOPSIS_CPP_DECODER
#define H_SYNOPSIS_CPP_DECODER

#include <string>
#include <vector>

// Forward decl of Types::Type
namespace Types
{
class Type;
}

// bad duplicate typedef.. hmm
typedef std::vector<std::string> ScopedName;

// Forward decl of Builder
class Builder;

class Lookup;

#if defined(__GNUC__) && __GNUC__ >= 3 && __GNUC_MINOR__ >= 2
namespace std
{
//. A specialization of the char_traits for unsigned char
template<>
struct char_traits<unsigned char>
{
    typedef unsigned char	char_type;
    typedef int 	        int_type;
    typedef streampos 	pos_type;
    typedef streamoff 	off_type;
    typedef mbstate_t 	state_type;

    static void
    assign(char_type& __c1, const char_type& __c2)
    {
        __c1 = __c2;
    }

    static bool
    eq(const char_type& __c1, const char_type& __c2)
    {
        return __c1 == __c2;
    }

    static bool
    lt(const char_type& __c1, const char_type& __c2)
    {
        return __c1 < __c2;
    }

    static int
    compare(const char_type* __s1, const char_type* __s2, size_t __n)
    {
        return memcmp(__s1, __s2, __n);
    }

    static size_t
    length(const char_type* __s)
    {
        return strlen(reinterpret_cast<const char*>(__s));
    }

    static const char_type*
    find(const char_type* __s, size_t __n, const char_type& __a)
    {
        return static_cast<const char_type*>(memchr(__s, __a, __n));
    }

    static char_type*
    move(char_type* __s1, const char_type* __s2, size_t __n)
    {
        return static_cast<char_type*>(memmove(__s1, __s2, __n));
    }

    static char_type*
    copy(char_type* __s1, const char_type* __s2, size_t __n)
    {
        return static_cast<char_type*>(memcpy(__s1, __s2, __n));
    }

    static char_type*
    assign(char_type* __s, size_t __n, char_type __a)
    {
        return static_cast<char_type*>(memset(__s, __a, __n));
    }

    static char_type
    to_char_type(const int_type& __c)
    {
        return static_cast<char_type>(__c);
    }

    // To keep both the byte 0xff and the eof symbol 0xffffffff
    // from ending up as 0xffffffff.
    static int_type
    to_int_type(const char_type& __c)
    {
        return static_cast<int_type>(static_cast<unsigned char>(__c));
    }

    static bool
    eq_int_type(const int_type& __c1, const int_type& __c2)
    {
        return __c1 == __c2;
    }

    static int_type
    eof()
    {
        return static_cast<int_type>(EOF);
    }

    static int_type
    not_eof(const int_type& __c)
    {
        return (__c == eof()) ? 0 : __c;
    }
};
};
#endif

//. A string type for the encoded names and types
typedef std::basic_string<unsigned char> code;
//. A string iterator type for the encoded names and types
typedef code::iterator code_iter;

//. A function to make a code string from a normal string
inline code make_code(const char* c)
{
    if (c == 0)
        return code();
    return code((const unsigned char*)c);
}

//. Insertion operator for encoded names and types
std::ostream& operator <<(std::ostream& o, const code& s);

//. Decoder for OCC encodings. This class can be used to decode the names and
//. types encoded by OCC for function and variable types and names.
class Decoder
{
public:
    //. Constructor
    Decoder(Builder*);

    //. Convert a char* to a 'code' type
    static code toCode(char*);

    //. Initialise the type decoder
    void init(const char*);

    //. Returns the iterator used in decoding
    code_iter& iter()
    {
        return m_iter;
    }

    //. Return a Type object from the encoded type.
    //. @preconditions You must call init() first.
    Types::Type* decodeType();

    //. Decodes a Qualified type. iter must be just after the Q
    Types::Type* decodeQualType();

    //. Decodes a Template type. iter must be just after the T
    Types::Type* decodeTemplate();

    //. Decodes a FuncPtr type. iter must be just after the F. The vector is
    //. the postmod - if it contains a * then it will be removed and given to
    //. the funcptr instead
    Types::Type* decodeFuncPtr(std::vector<std::string>&);

    //. Decode a name
    std::string decodeName();

    //. Decode a qualified name
    ScopedName decodeQualified();

    //. Decode a name starting from the given iterator.
    //. Note the iterator passed need not be from the currently decoding
    //. string since this is a simple method.
    std::string decodeName(code_iter);

    //. Decode a name starting from the given char*
    std::string decodeName(const char*);

    //. Decode a qualified name with only names in it
    void decodeQualName(ScopedName& names);

    //. Returns true if the char* is pointing to a name (that starts with a
    //. length). This is needed since char can be signed or unsigned, and
    //. explicitly casting to one or the other is ugly
    bool isName(const char *ptr);

private:
    //. The encoded type string currently being decoded
    code m_string;
    //. The current position in m_enc_iter
    code_iter m_iter;

    //. The builder
    Builder* m_builder;

    //. The lookup
    Lookup* m_lookup;
};

inline bool Decoder::isName(const char *ptr)
{
  const code::value_type* iter = reinterpret_cast<const code::value_type*>(ptr);
  return iter && iter[0] > 0x80;
}

#endif // header guard
// vim: set ts=8 sts=4 sw=4 et:
