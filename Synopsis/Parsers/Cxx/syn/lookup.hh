// vim: set ts=8 sts=2 sw=2 et:
// File: lookup.hh

#ifndef H_SYNOPSIS_CPP_LOOKUP
#define H_SYNOPSIS_CPP_LOOKUP

#include "ast.hh"

// Forward declare some Types::Type's
namespace Types
{
  class Type;
  class Base;
  class Named;
  class Unknown;
  class TemplateType;
  class FuncPtr;
}

// Forward declare the Builder class
class Builder;

class ScopeInfo;
typedef std::vector<ScopeInfo*> ScopeSearch; // TODO: move to common

//. AST Builder.
//. This class manages the building of an AST, including queries on the
//. existing AST such as name and type lookups. The building operations are
//. called by SWalker as it walks the parse tree.
class Lookup
{
public:
  //. Constructor
  Lookup(Builder*);

  //. Destructor. Recursively destroys all AST objects
  ~Lookup();

  //. Sets the swalker
  //void set_swalker(SWalker* swalker) { m_swalker = swalker; }

  //. Changes the current accessability for the current scope
  void set_access(AST::Access);

 
  //
  // AST Methods
  //

  //. Returns the current scope
  AST::Scope* scope();

  //. Returns the global scope
  AST::Scope* global();


  //
  // Type Methods
  //

  //. Looks up the name in the current scope. This method always succeeds --
  //. if the name is not found it forward declares it.
  Types::Named* lookupType(const std::string& name, bool func_okay = false);

  //. Looks up the qualified name in the current scope. This method always
  //. succeeds -- if the name is not found it forwards declares it.
  //. @param names The list of identifiers given
  //. @param fuc_okay If true, multiple declarations will not cause an error (needs fixing)
  //. @param scope If set determines the scope to start lookup from, else the
  //. current scope is used
  Types::Named* lookupType(const ScopedName& names, bool func_okay=false, AST::Scope* scope=NULL);

  //. Looks up the name in the scope of the given scope. This method may
  //. return a NULL ptr if the lookup failed.
  Types::Named* lookupType(const std::string& name, AST::Scope* scope);

  //. Looks up the function in the given scope with the given args. 
  AST::Function* lookupFunc(const std::string& , AST::Scope*, const std::vector<Types::Type*>&);

  //. Looks up the function operator in the current scope with the given
  //. types. May return NULL if builtin operator or no operator is found.
  AST::Function* lookupOperator(const std::string& oper, Types::Type* left_type, Types::Type* right_type);

  //. Maps a scoped name into a vector of scopes and the final type. Returns
  //. true on success.
  bool mapName(const ScopedName& name, std::vector<AST::Scope*>&, Types::Named*&);

  //. Returns the types for an array operator on the given type with an
  //. argument of the given type. If a function is used then it is stored in
  //. the function ptr ref given, else the ptr is set to NULL.
  Types::Type* arrayOperator(Types::Type* object, Types::Type* arg, AST::Function*&);

  //. Resolves the final type of the given type. If the given type is an
  //. Unknown, it checks to see if the type has been defined yet and returns
  //. that instead.
  Types::Named* resolveType(Types::Named* maybe_unknown);

private:
  //. Looks up the name in the current scope. This method may fail and
  //. return a NULL ptr.
  Types::Named* lookup(const std::string& name, bool func_okay = false);

  //. Searches for name in the list of Scopes. This method may return NULL
  //. if the name is not found.
  Types::Named* lookup(const std::string& name, const ScopeSearch&, bool func_okay = false) throw ();

  //. Searches for name in the given qualified scope. This method may return
  //. NULL if the name is not found. Lookup proceeds according to the spec:
  //. if 'scope' is a Class scope, then scope and all base classes are
  //. searched, else if it's a 'namespace' scope then all usings are checked.
  Types::Named* lookupQual(const std::string& name, const ScopeInfo*, bool func_okay = false);

  //. Return a ScopeInfo* for the given Declaration. This method first looks for
  //. an existing Scope* in the Private map.
  ScopeInfo* find_info(AST::Scope*);

  //. Utility class to add all functions with the given name in the given
  //. Scope's dictionary to the given vector. May throw an error if the
  //. types looked up are not functions.
  void findFunctions(const std::string&, ScopeInfo*, std::vector<AST::Function*>&);

  //. Determines the best function from the given list for the given
  //. arguments using heuristics. Returns the function and stores the cost
  AST::Function* bestFunction(const std::vector<AST::Function*>&, const std::vector<Types::Type*>&, int& cost);

  //. Formats the search of the given Scope for logging
  std::string dumpSearch(ScopeInfo* scope);

  //. A pointer to the Builder.
  Builder* m_builder;

}; // class Builder

#endif

