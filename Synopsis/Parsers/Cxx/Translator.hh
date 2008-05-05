//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef H_SYNOPSIS_CPP_SYNOPSIS
#define H_SYNOPSIS_CPP_SYNOPSIS

#include <Python.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <stack>
#include "ASG.hh"
#include "Types.hh"

class FileFilter;

//. The Translator class maps from C++ objects to Python objects
class Translator : public ASG::Visitor, public Types::Visitor
{
public:

  Translator(FileFilter*, PyObject *ir);
  ~Translator();

  void translate(ASG::Scope* global);
  void set_builtin_decls(const ASG::Declaration::vector& builtin_decls);

  //
  // types from the Synopsis.Type module
  //
  PyObject* Base(Types::Base*);
  PyObject* Unknown(Types::Named*);
  PyObject* Declared(Types::Declared*);
  PyObject* Dependent(Types::Dependent*);
  PyObject* Template(Types::Template*);
  PyObject* Modifier(Types::Modifier*);
  PyObject* Array(Types::Array*);
  PyObject* Parameterized(Types::Parameterized*);
  PyObject* FuncPtr(Types::FuncPtr*);
  
  //
  // types from the Synopsis.ASG module
  //
  PyObject* SourceFile(ASG::SourceFile*);
  PyObject* Include(ASG::Include*);
  PyObject* Declaration(ASG::Declaration*);
  PyObject* Builtin(ASG::Builtin*);
  PyObject* Macro(ASG::Macro*);
  PyObject* Forward(ASG::Forward*);
  PyObject* Scope(ASG::Scope*);
  PyObject* Namespace(ASG::Namespace*);
  PyObject* Inheritance(ASG::Inheritance*);
  PyObject* Class(ASG::Class*);
  PyObject* ClassTemplate(ASG::ClassTemplate *);
  PyObject* Typedef(ASG::Typedef*);
  PyObject* Enumerator(ASG::Enumerator*);
  PyObject* Enum(ASG::Enum*);
  PyObject* Variable(ASG::Variable*);
  PyObject* Const(ASG::Const*);
  PyObject* Parameter(ASG::Parameter*);
  PyObject* Function(ASG::Function*);
  PyObject* Operation(ASG::Operation*);
  PyObject* UsingDirective(ASG::UsingDirective*);
  PyObject* UsingDeclaration(ASG::UsingDeclaration*);

  //
  // ASG::Visitor methods
  //
  void visit_declaration(ASG::Declaration*);
  void visit_builtin(ASG::Builtin*);
  void visit_macro(ASG::Macro*);
  void visit_scope(ASG::Scope*);
  void visit_namespace(ASG::Namespace*);
  void visit_class(ASG::Class*);
  void visit_class_template(ASG::ClassTemplate *);
  void visit_inheritance(ASG::Inheritance*);
  void visit_forward(ASG::Forward*);
  void visit_typedef(ASG::Typedef*);
  void visit_variable(ASG::Variable*);
  void visit_const(ASG::Const*);
  void visit_enum(ASG::Enum*);
  void visit_enumerator(ASG::Enumerator*);
  void visit_function(ASG::Function*);
  void visit_operation(ASG::Operation*);
  void visit_parameter(ASG::Parameter*);
  void visit_using_directive(ASG::UsingDirective*);
  void visit_using_declaration(ASG::UsingDeclaration*);
  
  //
  // Types::Visitor methods
  //
  //void visitType(Types::Type*);
  void visit_unknown(Types::Unknown*);
  void visit_modifier(Types::Modifier*);
  void visit_array(Types::Array*);
  //void visitNamed(Types::Named*);
  void visit_base(Types::Base*);
  void visit_dependent(Types::Dependent*);
  void visit_declared(Types::Declared*);
  void visit_template_type(Types::Template*);
  void visit_parameterized(Types::Parameterized*);
  void visit_func_ptr(Types::FuncPtr*);

private:
  //. Compiler Firewalled private data
  struct Private;
  friend struct Private;
  Private* m;

  //.
  //. helper methods
  //.
  void addComments(PyObject* pydecl, ASG::Declaration* cdecl);

  ///////// EVERYTHING BELOW HERE SUBJECT TO REVIEW AND DELETION


  /*
    PyObject *lookupType(const std::string &, PyObject *);
    PyObject *lookupType(const std::string &);
    PyObject *lookupType(const std::vector<std::string>& qualified);
    
    static void addInheritance(PyObject *, const std::vector<PyObject *> &);
    static PyObject *N2L(const std::string &);
    static PyObject *V2L(const std::vector<std::string> &);
    static PyObject *V2L(const std::vector<PyObject *> &);
    static PyObject *V2L(const std::vector<size_t> &);
    void pushClassBases(PyObject* clas);
    PyObject* resolveDeclared(PyObject*);
    void addDeclaration(PyObject *);
  */
private:
  PyObject* m_asg_module;
  PyObject* m_sf_module;
  PyObject *m_ir;
  PyObject* m_declarations;
  PyObject* m_dictionary;
  
  FileFilter* m_filter;
};

#endif
