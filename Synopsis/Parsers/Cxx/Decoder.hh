//
// Copyright (C) 2002 Stephen Davies
// Copyright (C) 2002 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef Decoder_hh_
#define Decoder_hh_

#include <Synopsis/PTree/Encoding.hh>
#include <string>
#include <vector>

// Forward decl of Types::Type
namespace Types
{
class Type;
class Parameterized;
}

// bad duplicate typedef.. hmm
typedef std::vector<std::string> ScopedName;

// Forward decl of Builder
class Builder;

class Lookup;

//. A string type for the encoded names and types
typedef Synopsis::PTree::Encoding::Code code;
//. A string iterator type for the encoded names and types
typedef code::iterator code_iter;

//. Decoder for OCC encodings. This class can be used to decode the names and
//. types encoded by OCC for function and variable types and names.
class Decoder
{
public:
    //. Constructor
    Decoder(Builder*);

    //. Initialise the type decoder
  void init(const Synopsis::PTree::Encoding &);

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
    Types::Parameterized* decodeTemplate();

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
  std::string decodeName(const Synopsis::PTree::Encoding &);

    //. Decode a qualified name with only names in it
    void decodeQualName(ScopedName& names);

    //. Returns true if the char* is pointing to a name (that starts with a
    //. length). This is needed since char can be signed or unsigned, and
    //. explicitly casting to one or the other is ugly
  bool isName(const Synopsis::PTree::Encoding &);

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

inline bool Decoder::isName(const Synopsis::PTree::Encoding &e)
{
  return !e.empty() && e.at(0) > 0x80;
}

#endif // header guard
// vim: set ts=8 sts=4 sw=4 et:
