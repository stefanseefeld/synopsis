// File: decoder.cc

#include "decoder.hh"
#include "type.hh"
#include "builder.hh"
#include <iostream>

Decoder::Decoder(Builder* builder)
    : m_builder(builder)
{
}

std::string Decoder::decodeName()
{
    size_t length = *m_iter++ - 0x80;
    std::string name(length, '\0');
    std::copy(m_iter, m_iter + length, name.begin());
    m_iter += length;
    return name;
}

std::string Decoder::decodeName(code_iter iter)
{
    size_t length = *iter++ - 0x80;
    std::string name(length, '\0');
    std::copy(iter, iter + length, name.begin());
    iter += length;
    return name;
}

std::string Decoder::decodeName(char* iter)
{
    size_t length = *reinterpret_cast<unsigned char*>(iter++) - 0x80;
    std::string name(iter, length);
    return name;
}

void Decoder::decodeQualName(std::vector<std::string>& names)
{
    if (*m_iter++ != 'Q') return;
    int scopes = *m_iter++ - 0x80;
    while (scopes--) {
	// Only handle names here
	if (*m_iter >= 0x80) { // Name
	    names.push_back(decodeName());
	} else {
	    std::cerr << "Warning: Unknown type inside Q name: " << *m_iter << std::endl;
	    // FIXME: provide << operator for m_string !
// 	    std::cerr << "         Decoding " << m_string << std::endl;
	}
    }
}

void Decoder::init(char* string)
{
    m_string = reinterpret_cast<unsigned char*>(string);
    m_iter = m_string.begin();
}

Type::Type* Decoder::decodeType()
{
    code_iter end = m_string.end();
    std::vector<std::string> premod, postmod;
    std::string name;
    Type::Type *baseType = NULL;

    // Loop forever until broken
    while (m_iter != end && !name.length() && !baseType) {
        int c = *m_iter++;
	switch (c) {
        case 'P': postmod.insert(postmod.begin(), "*"); break;
        case 'R': postmod.insert(postmod.begin(), "&"); break;
        case 'S': premod.push_back("signed"); break;
        case 'U': premod.push_back("unsigned"); break;
        case 'C': premod.push_back("const"); break;
        case 'V': premod.push_back("volatile"); break;
        case 'A': premod.push_back("[]"); break;
        case '*': name = "..."; break;
        case 'i': name = "int"; break;
        case 'v': name = "void"; break;
        case 'b': name = "bool"; break;
        case 's': name = "short"; break;
        case 'c': name = "char"; break;
        case 'l': name = "long"; break;
        case 'j': name = "long long"; break;
        case 'f': name = "float"; break;
        case 'd': name = "double"; break;
        case 'r': name = "long double"; break;
        case 'e': name = "..."; break;
        case '?': return NULL;
        case 'Q': baseType = decodeQualType(); break;
	case '_': --m_iter; return NULL; // end of func params
        case 'F': baseType = decodeFuncPtr(); break;
        case 'T': baseType = decodeTemplate(); break;
        case 'M':
	    // Pointer to member. Format is same as for named types
	    name = decodeName() + "::*";
	    break;
        default:
	    if (c > 0x80) { --m_iter; name = decodeName(); break; } 
	    // FIXME
// 	    std::cerr << "\nUnknown char decoding '"<<m_string<<"': " 
// 		 << char(c) << " " << c << " at "
// 		 << (m_iter - m_string.begin()) << std::endl;
	} // switch
    } // while
    if (!baseType && !name.length()) {
      // FIXME
// 	std::cerr << "no type or name found decoding " << m_string << std::endl;
	return 0;
    }
    if (!baseType)
        baseType = m_builder->lookupType(name);
    if (premod.empty() && postmod.empty())
        return baseType;
    Type::Type* ret = new Type::Modifier(baseType, premod, postmod);
    return ret;
}

Type::Type* Decoder::decodeQualType()
{
    // Qualified type: first is num of scopes, each a name.
    int scopes = *m_iter++ - 0x80;
    std::vector<std::string> names;
    std::vector<Type::Type*> types; // if parameterized
    while (scopes--) {
	// Only handle two things here: names and templates
	if (*m_iter >= 0x80) { // Name
	    names.push_back(decodeName());
	} else if (*m_iter == 'T') {
	    // Template :(
	    ++m_iter;
	    std::string tname = decodeName();
	    code_iter tend = m_iter + *m_iter++ - 0x80;
	    while (m_iter < tend)
		types.push_back(decodeType());
	    names.push_back(tname);
	} else {
	    std::cerr << "Warning: Unknown type inside Q: " << *m_iter << std::endl;
	    // FIXME
// 	    std::cerr << "         Decoding " << m_string << std::endl;
	}
    }
    // Ask for qualified lookup
    Type::Type* baseType;
    try {
	baseType = m_builder->lookupType(names);
    } catch (...) {
	// Ignore error, and return an Unknown instead
	return new Type::Unknown(names);
    }
    // If the type is a template, then parameterize it with the params found
    // in the T decoding
    Type::Declared* declared = dynamic_cast<Type::Declared*>(baseType);
    AST::Class* tempclas = declared ? dynamic_cast<AST::Class*>(declared->declaration()) : NULL;
    Type::Template* templType = tempclas ? tempclas->templateType() : NULL;
    if (templType && types.size()) {
	return new Type::Parameterized(templType, types);
    }
    return baseType;
}

Type::Type* Decoder::decodeFuncPtr()
{
    // Function ptr. Encoded same as function
    Type::Type::Mods postmod;
    std::vector<Type::Type*> params;
    while (1) {
	Type::Type* type = decodeType();
	if (type) params.push_back(type);
	else break;
    }
    ++m_iter; // skip over '_'
    Type::Type* returnType = decodeType();
    Type::Type* ret = new Type::FuncPtr(returnType, postmod, params);
    return ret;
}

Type::Type* Decoder::decodeTemplate()
{
    // Template type: Name first, then size of arg field, then arg
    // types eg: T6vector54cell <-- 5 is len of 4cell
    std::string name = decodeName();
    code_iter tend = m_iter + *m_iter++ - 0x80;
    std::vector<Type::Type*> types;
    while (m_iter <= tend)
	types.push_back(decodeType());
    Type::Type* type = m_builder->lookupType(name);
    // if type is declared and declaration is class and class is template..
    Type::Declared* declared = dynamic_cast<Type::Declared*>(type);
    Type::Template* templ = NULL;
    if (declared) {
	AST::Class* t_class = dynamic_cast<AST::Class*>(declared->declaration());
	if (t_class) {
	    templ = t_class->templateType();
	}
    }
    return new Type::Parameterized(templ, types);
} 

