// File: dict.cc
// Implementation of Dictionary

#include "dict.hh"
#include "ast.hh"

#include <map>
#include <vector>
#include <string>
using std::string;
using std::vector;
using std::multimap;

typedef multimap<string, AST::Declaration*> decl_map;

//. Define nested class
struct Dictionary::Data {
    decl_map map;
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
AST::Declaration* Dictionary::lookup(string name) throw (MultipleDeclarations)
{
    decl_map::iterator iter = m->map.lower_bound(name);
    decl_map::iterator end = m->map.upper_bound(name);
    // Check for not found
    if (iter == end)
	throw KeyError(name);
    AST::Declaration* decl = iter->second;
    if (++iter == end)
	return decl;
    // Create exception object
    MultipleDeclarations* exc = new MultipleDeclarations;
    exc->declarations.push_back(decl);
    do {
	exc->declarations.push_back(iter->second);
    } while (++iter != end);
    throw exc;
}

//. Lookup multiple
vector<AST::Declaration*> Dictionary::lookupMultiple(string name)
{
    decl_map::iterator iter = m->map.lower_bound(name);
    decl_map::iterator end = m->map.upper_bound(name);
    // Check for not found
    if (iter == end)
	throw KeyError(name);
    // Store declaration pointers in a vector
    vector<AST::Declaration*> decls;
    do {
	decls.push_back(iter->second);
    } while (++iter != end);
    return decls;
}
