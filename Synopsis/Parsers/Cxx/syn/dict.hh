// vim: set ts=8 sts=2 sw=2 et:
// File: dict.hh

#ifndef H_SYNOPSIS_CPP_DICT
#define H_SYNOPSIS_CPP_DICT

#include <vector>
#include <string>

// Forward declaration of Type::Named
namespace Types
{ class Named; }

// Forward declaration of AST::Declaration
namespace AST
{ class Declaration; }

//. Dictionary of named declarations with lookup.
//. This class maintains a dictionary of names, which index types,
//. supposedly declared in the scope that has this dictionary. There may be
//. only one declaration per name, except in the case of function names.
class Dictionary
{
public:
  //. Constructor
  Dictionary();
  //. Destructor
  ~Dictionary();

  //. The type of multiple entries. We don't want to include type.hh just for
  //. this, so this is a workaround
  typedef std::vector<Types::Named*> Type_vector;

  //. Exception thrown when multiple declarations are found when one is
  //. expected. The list of declarations is stored in the exception.
  struct MultipleError
  {
    // The vector of types that *were* found. This is returned so that whoever
    // catches the error can proceed straight away
    Type_vector types;
  };

  //. Exception thrown when a name is not found in lookup*()
  struct KeyError
  {
    //. Constructor
    KeyError(const std::string& n)
    : name(n)
    { }
    //. The name which was not found
    std::string name;
  };

  //. Returns true if name is in dict
  bool has_key(const std::string& name);

  //. Lookup a name in the dictionary. If more than one declaration has this
  //. name then an exception is thrown.
  Types::Named* lookup(const std::string& name);// throw (MultipleError, KeyError);

  //. Lookup a name in the dictionary expecting multiple decls. Use this
  //. method if you expect to find more than one declaration, eg importing
  //. names via a using statement.
  Type_vector lookupMultiple(const std::string& name) throw (KeyError);

  //. Add a declaration to the dictionary. The name() is extracted from the
  //. declaration and its last string used as the key. The declaration is
  //. stored as a Type::Declared which is created inside this method.
  void insert(AST::Declaration* decl);

  //. Add a named type to the dictionary. The name() is extracted from the
  //. type and its last string used as they key.
  void insert(Types::Named* named);

  //. Dump the contents for debugging
  void dump();


private:
  struct Data;
  //. The private data. This is a forward declared * to speed compilation since
  //. std::map is a large template.
  Data*  m;
};

#endif // header guard
