// File: dict.cc
// Implementation of Dictionary

#include "dict.hh"
#include "ast.hh"
#include "type.hh"

#include <map>
#include <vector>
#include <string>
using std::string;
using std::vector;
using std::multimap;

typedef multimap<string, Type::Named*> name_map;

//. Define nested class
struct Dictionary::Data {
    name_map map;
};

//. Constructor
Dictionary::Dictionary() {
    m = new Data;
}

//. Destructor
Dictionary::~Dictionary() {
    delete m;
}

//. Quick check if the name is in the dict
bool Dictionary::has_key(string name)
{
    return (m->map.find(name) != m->map.end());
}

//. Lookup single
Type::Named* Dictionary::lookup(string name) throw (MultipleError)
{
    name_map::iterator iter = m->map.lower_bound(name);
    name_map::iterator end = m->map.upper_bound(name);
    // Check for not found
    if (iter == end)
	throw KeyError(name);
    Type::Named* type = iter->second;
    if (++iter == end)
	return type;
    // Create exception object
    MultipleError* exc = new MultipleError;
    exc->types.push_back(type);
    do {
	exc->types.push_back(iter->second);
    } while (++iter != end);
    throw exc;
}

//. Lookup multiple
vector<Type::Named*> Dictionary::lookupMultiple(string name)
{
    name_map::iterator iter = m->map.lower_bound(name);
    name_map::iterator end = m->map.upper_bound(name);
    // Check for not found
    if (iter == end)
	throw KeyError(name);
    // Store typearation pointers in a vector
    vector<Type::Named*> types;
    do {
	types.push_back(iter->second);
    } while (++iter != end);
    return types;
}

void Dictionary::insert(Type::Named* type)
{
    string key = type->name().back();
    m->map.insert(name_map::value_type(key, type));
}

void Dictionary::insert(AST::Declaration* decl)
{
    insert(new Type::Declared(decl->name(), decl));
}
