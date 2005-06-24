//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolTable_Display_hh_
#define Synopsis_SymbolTable_Display_hh_

#include <Synopsis/SymbolTable.hh>

namespace Synopsis
{
namespace SymbolTable
{

class SymbolDisplay : private SymbolVisitor
{
public:
  SymbolDisplay(std::ostream &os, size_t indent)
    : my_os(os), my_indent(indent, ' ') {}
  void display(PTree::Encoding const &, SymbolTable::Symbol const *);
private:
  std::ostream &prefix(std::string const &type)
  { return my_os << my_indent << type;}
  virtual void visit(SymbolTable::Symbol const *);
  virtual void visit(SymbolTable::VariableName const *);
  virtual void visit(SymbolTable::ConstName const *);
  virtual void visit(SymbolTable::TypeName const *);
  virtual void visit(SymbolTable::TypedefName const *);
  virtual void visit(SymbolTable::ClassName const *);
  virtual void visit(SymbolTable::EnumName const *);
  virtual void visit(SymbolTable::ClassTemplateName const *);
  virtual void visit(SymbolTable::FunctionName const *);
  virtual void visit(SymbolTable::FunctionTemplateName const *);
  virtual void visit(SymbolTable::NamespaceName const *);

  std::ostream &my_os;
  std::string   my_indent;
  std::string   my_name;
};

//. The ScopeDisplay class provides an annotated view of the symbol table,
//. for debugging purposes.
class ScopeDisplay : private SymbolTable::ScopeVisitor
{
public:
  ScopeDisplay(std::ostream &os) : my_os(os), my_indent(0) {}
  virtual ~ScopeDisplay() {}
  void display(SymbolTable::Scope const *s) 
  { const_cast<SymbolTable::Scope *>(s)->accept(this);}
private:
  virtual void visit(SymbolTable::TemplateParameterScope *);
  virtual void visit(SymbolTable::LocalScope *);
  virtual void visit(SymbolTable::PrototypeScope *);
  virtual void visit(SymbolTable::FunctionScope *);
  virtual void visit(SymbolTable::Class *);
  virtual void visit(SymbolTable::Namespace *);

  void dump(SymbolTable::Scope const *);
  std::ostream &indent();

  std::ostream &my_os;
  size_t        my_indent;
};

inline void display(SymbolTable::Scope const *s, std::ostream &os)
{
  ScopeDisplay sd(os);
  sd.display(s);
}

}
}

#endif
