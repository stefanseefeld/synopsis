// File: decoder.hh
// Decodes the names and types encoded by OCC

#ifndef H_SYNOPSIS_CPP_DECODER
#define H_SYNOPSIS_CPP_DECODER

#include <string>
#include <vector>

using std::string;
using std::vector;

// Forward decl of Type::Type
namespace Type { class Type; }

// bad duplicate typedef.. hmm
typedef vector<string> Name;

// Forward decl of Builder
class Builder;


//. A string type for the encoded names and types
typedef basic_string<unsigned char> code;
//. A string iterator type for the encoded names and types
typedef code::iterator code_iter;

//. Decoder for OCC encodings. This class can be used to decode the names and
//. types encoded by OCC for function and variable types and names.
class Decoder {
public:
    //. Constructor
    Decoder(Builder*);

    //. Convert a char* to a 'code' type
    static code toCode(char*);

    //. Initialise the type decoder
    void init(char*);

    //. Returns the iterator used in decoding
    code_iter& iter() { return m_iter; }

    //. Return a Type object from the encoded type.
    //. @preconditions You must call init() first.
    Type::Type* decodeType();

    //. Decodes a Qualified type. iter must be just after the Q
    Type::Type* decodeQualType();

    //. Decodes a Template type. iter must be just after the T
    Type::Type* decodeTemplate();

    //. Decodes a FuncPtr type. iter must be just after the F
    Type::Type* decodeFuncPtr();

    //. Decode a name
    string decodeName();

    //. Decode a qualified name
    Name decodeQualified();

    //. Decode a name starting from the given iterator.
    //. Note the iterator passed need not be from the currently decoding
    //. string since this is a simple method.
    string decodeName(code_iter);

    //. Decode a name starting from the given char*
    string decodeName(char*);

    //. Decode a qualified name with only names in it
    void decodeQualName(vector<string>& names);

private:
    //. The encoded type string currently being decoded
    code m_string;
    //. The current position in m_enc_iter
    code_iter m_iter;

    //. The builder
    Builder* m_builder;

};

#endif // header guard
