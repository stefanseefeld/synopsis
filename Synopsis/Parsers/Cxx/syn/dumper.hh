// File: dumper.hh
// Dumper is a visitor similar to Formatter/DUMP.py

#ifndef H_SYNOPSIS_CPP_DUMPER
#define H_SYNOPSIS_CPP_DUMPER

#include "ast.hh"
#include "type.hh"

//. Formats Types in a way suitable for output
class TypeFormatter : public Types::Visitor
{
public:
  TypeFormatter();

  //. Sets the current scope
  void setScope(const ScopedName& scope);

  //
  // Type Visitor
  //
  //. Returns a formatter string for given type
  std::string format(const Types::Type*);
  virtual void visit_type(Types::Type*);
  virtual void visit_unknown(Types::Unknown*);
  virtual void visit_modifier(Types::Modifier*);
  virtual void visit_named(Types::Named*);
  virtual void visit_base(Types::Base*);
  virtual void visit_declared(Types::Declared*);
  virtual void visit_template_type(Types::Template*);
  virtual void visit_parameterized(Types::Parameterized*);
  virtual void visit_func_ptr(Types::FuncPtr*);

protected:
  //. The Type String
  std::string m_type;
  //. The current scope name
  ScopedName m_scope;
  //. Returns the given Name relative to the current scope
  std::string colonate(const ScopedName& name);
};

//. Dumper displays the AST to the screen
class Dumper : public AST::Visitor, public TypeFormatter {
public:
  Dumper();

  //. Sets to only show decls with given filename
  void onlyShow(const std::string& fname);
  
  std::string formatParam(AST::Parameter*);
  //
  // AST Visitor
  //
  void visit(const std::vector<AST::Declaration*>&);
  void visit(const std::vector<AST::Comment*>&);
  virtual void visit_declaration(AST::Declaration*);
  virtual void visit_scope(AST::Scope*);
  virtual void visit_namespace(AST::Namespace*);
  virtual void visit_class(AST::Class*);
  virtual void visit_operation(AST::Operation*);
  virtual void visit_variable(AST::Variable*);
  virtual void visit_typedef(AST::Typedef*);
  virtual void visit_enum(AST::Enum*);
  virtual void visit_enumerator(AST::Enumerator*);

private:
  //. The indent depth
  int m_indent;
  //. The indent string
  std::string m_indent_string;
  //. Increases indent
  void indent();
  //. Decreases indent
  void undent();
  //. Only show this filename, if set
  std::string m_filename;

};

#endif // header guard
