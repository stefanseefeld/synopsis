// File: decoder.hh
// Decodes the names and types encoded by OCC

#ifndef H_SYNOPSIS_CPP_DECODER
#define H_SYNOPSIS_CPP_DECODER

// Forward decl of Type::Type
namespace Type { class Type; }

// Forward decl of Type::Name
namespace Type { typedef vector<string> Name; }

//. A string type for the encoded names and types
typedef basic_string<unsigned char> code;
//. A string iterator type for the encoded names and types
typedef code::iterator code_iter;

//. Decoder for OCC encodings. This class can be used to decode the names and
//. types encoded by OCC for function and variable types and names.
class Decoder {
public:
    //. Constructor
    Decoder();

    //. Convert a char* to a 'code' type
    static code toCode(char*);

    //. Initialise the type decoder
    void init(char*);

    //. Returns the iterator used in decoding
    code_iter& iter() { return m_iter; }

    //. Return a Type object from the encoded type.
    //. @preconditions You must call init() first.
    Type* decodeType();

    //. Decode a name
    string decodeName();

    //. Decode a qualified name
    Name decodeQualified();

    //. Decode a name starting from the given iterator.
    //. Note the iterator passed need not be from the currently decoding
    //. string since this is a simple method.
    string decodeName(code_iter);

private:
    //. The encoded type string currently being decoded
    code m_string;
    //. The current position in m_enc_iter
    code_iter m_iter;

};

#endif // header guard
