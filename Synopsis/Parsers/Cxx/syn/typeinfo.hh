// vim: set ts=8 sts=2 sw=2 et:
// File: typeinfo.hh

#ifndef H_SYNOPSIS_CPP_TYPEINFO
#define H_SYNOPSIS_CPP_TYPEINFO

#include "type.hh"
#include "dumper.hh"


class TypeInfo : public Types::Visitor
{
public:
  Types::Type* type;
  bool is_const;
  bool is_volatile;
  bool is_null;
  size_t deref;

  //. Constructor
  TypeInfo(Types::Type* t)
  {
    type = t; is_const = is_volatile = is_null = false; deref = 0;
    set(t);
  }
  //. Set to the given type
  void set(Types::Type* t)
  {
    type = t;
    t->accept(this);
  }
  //. Base -- null is flagged since it is special
  void visit_base(Types::Base* base)
  {
    if (base->name().back() == "__null_t")
      is_null = true;
  }
  //. Modifiers -- recurse on the alias type
  void visit_modifier(Types::Modifier* mod)
  {
    Types::Type::Mods::const_iterator iter;
    // Check for const
    for (iter = mod->pre().begin(); iter != mod->pre().end(); iter++)
      if (*iter == "const")
        is_const = true;
      else if (*iter == "volatile")
        is_volatile = true;
    // Check for derefs
    for (iter = mod->post().begin(); iter != mod->post().end(); iter++)
      if (*iter == "*")
        deref++;
      else if (*iter == "[]")
        deref++;
  
    set(mod->alias());
  }
  //. Declared -- check for typedef
  void visit_declared(Types::Declared* t)
  {
    try
      {
        AST::Typedef* tdef = Types::declared_cast<AST::Typedef>(t);
        // Recurse on typedef alias
        set(tdef->alias());
      }
    catch (const Types::wrong_type_cast&)
      { /* Ignore -- just means not a typedef */ }
  }
};

//. Output operator for debugging
std::ostream& operator << (std::ostream& o, TypeInfo& i);
#endif
