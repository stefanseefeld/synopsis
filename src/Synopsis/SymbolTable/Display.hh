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

class SymbolType : private SymbolVisitor
{
public:
  std::string name(SymbolTable::Symbol const *s)
  {
    s->accept(this);
    return type_;
  }
private:
  virtual void visit(SymbolTable::Symbol const *) { type_ = "unknown";}
  virtual void visit(SymbolTable::VariableName const *) { type_ = "variable";}
  virtual void visit(SymbolTable::ConstName const *) { type_ = "constant";}
  virtual void visit(SymbolTable::TypeName const *) { type_ = "type-name";}
  virtual void visit(SymbolTable::TypedefName const *) { type_ = "typedef";}
  virtual void visit(SymbolTable::ClassName const *) { type_ = "class-name";}
  virtual void visit(SymbolTable::EnumName const *) { type_ = "enum-name";}
  virtual void visit(SymbolTable::DependentName const *) { type_ = "dependent";}
  virtual void visit(SymbolTable::ClassTemplateName const *) { type_ = "class template";}
  virtual void visit(SymbolTable::FunctionName const *) { type_ = "function";}
  virtual void visit(SymbolTable::FunctionTemplateName const *) { type_ = "function template";}
  virtual void visit(SymbolTable::NamespaceName const *) { type_ = "namespace";}

  std::string   type_;
};

class SymbolDisplay : private SymbolVisitor
{
public:
  SymbolDisplay(std::ostream &os, size_t indent)
    : os_(os), indent_(indent, ' ') {}
  void display(PTree::Encoding const &, SymbolTable::Symbol const *);
private:
  std::ostream &prefix(std::string const &type)
  { return os_ << indent_ << type;}
  virtual void visit(SymbolTable::Symbol const *);
  virtual void visit(SymbolTable::VariableName const *);
  virtual void visit(SymbolTable::ConstName const *);
  virtual void visit(SymbolTable::TypeName const *);
  virtual void visit(SymbolTable::TypedefName const *);
  virtual void visit(SymbolTable::ClassName const *);
  virtual void visit(SymbolTable::EnumName const *);
  virtual void visit(SymbolTable::DependentName const *);
  virtual void visit(SymbolTable::ClassTemplateName const *);
  virtual void visit(SymbolTable::FunctionName const *);
  virtual void visit(SymbolTable::FunctionTemplateName const *);
  virtual void visit(SymbolTable::NamespaceName const *);

  std::ostream &os_;
  std::string   indent_;
  std::string   name_;
};

//. The ScopeDisplay class provides an annotated view of the symbol table,
//. for debugging purposes.
class ScopeDisplay : private SymbolTable::ScopeVisitor
{
public:
  ScopeDisplay(std::ostream &os) : os_(os), indent_(0) {}
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

  std::ostream &os_;
  size_t        indent_;
};

//. Display a Scope recursively with all its symbols and nested scopes
//. on the given output stream.
inline void display(SymbolTable::Scope const *s, std::ostream &os)
{
  ScopeDisplay sd(os);
  sd.display(s);
}

//. Display a SymbolSet on the given output stream.
inline void display(SymbolTable::SymbolSet const &s, std::ostream &os)
{
  SymbolDisplay sd(os, 0);
  for (SymbolTable::SymbolSet::const_iterator i = s.begin(); i != s.end(); ++i)
    sd.display(PTree::Encoding(), *i);
}

//. Display the name of the symbol type.
inline std::string type_name(SymbolTable::Symbol const *s)
{
  SymbolType st;
  return st.name(s);
}

}
}

#endif
