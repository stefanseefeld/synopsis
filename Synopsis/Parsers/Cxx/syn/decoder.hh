// File: decoder.hh
// Decodes the names and types encoded by OCC

#ifndef H_SYNOPSIS_CPP_DECODER
#define H_SYNOPSIS_CPP_DECODER

#include <string>
#include <vector>

// Forward decl of Types::Type
namespace Types { class Type; }

// bad duplicate typedef.. hmm
typedef std::vector<std::string> ScopedName;

// Forward decl of Builder
class Builder;

class Lookup;


//. A string type for the encoded names and types
typedef std::basic_string<unsigned char> code;
//. A string iterator type for the encoded names and types
typedef code::iterator code_iter;

//. Insertion operator for encoded names and types
std::ostream& operator <<(std::ostream& o, code& s);

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
    Types::Type* decodeType();

    //. Decodes a Qualified type. iter must be just after the Q
    Types::Type* decodeQualType();

    //. Decodes a Template type. iter must be just after the T
    Types::Type* decodeTemplate();

    //. Decodes a FuncPtr type. iter must be just after the F
    Types::Type* decodeFuncPtr();

    //. Decode a name
    std::string decodeName();

    //. Decode a qualified name
    ScopedName decodeQualified();

    //. Decode a name starting from the given iterator.
    //. Note the iterator passed need not be from the currently decoding
    //. string since this is a simple method.
    std::string decodeName(code_iter);

    //. Decode a name starting from the given char*
    std::string decodeName(char*);

    //. Decode a qualified name with only names in it
    void decodeQualName(ScopedName& names);

    //. Returns true if the char* is pointing to a name (that starts with a
    //. length). This is needed since char can be signed or unsigned, and
    //. explicitly casting to one or the other is ugly
    bool isName(char* ptr);

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

inline bool Decoder::isName(char* ptr)
{
    code::value_type* iter = reinterpret_cast<code::value_type*>(ptr);
    return iter && iter[0] > 0x80;
}

#endif // header guard
