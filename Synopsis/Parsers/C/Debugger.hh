//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Debugger_hh
#define _Debugger_hh

#include "Visitor.hh"
#include "Expression.hh"
#include "File.hh"
#include <iostream>

//. Debuggers are intended to be used to
//. easily inspect (i.e. print out) the structure
//. and content of part of a syntax tree.

class Debugger
{
public:
  Debugger(std::ostream &os) : my_os(os), my_level(0) {}
  Debugger(Debugger &d) : my_os(d.my_os), my_level(d.my_level) {}
  size_t indentation() const { return my_level;}
  std::ostream &os() { return my_os;}
  void incr_indent() { ++my_level;}
  void decr_indent() { --my_level;}
  std::ostream &indent() { return my_os << std::string(my_level * 2, ' ');}
private:
  std::ostream &my_os;
  size_t        my_level;
};

class TypeDebugger : public TypeVisitor, private Debugger
{
public:
  TypeDebugger(std::ostream &os) : Debugger(os) {}
  TypeDebugger(Debugger &d) : Debugger(d) {}
  virtual ~TypeDebugger() {}

  virtual void traverse_base(BaseType *);
  virtual void traverse_ptr(PtrType *);
  virtual void traverse_array(ArrayType *);
  virtual void traverse_bit_field(BitFieldType *);
  virtual void traverse_function(FunctionType *);
};

class ExpressionDebugger : public ExpressionVisitor, private Debugger
{
public:
  ExpressionDebugger(std::ostream &os) : Debugger(os) {}
  ExpressionDebugger(Debugger &d) : Debugger(d) {}
  virtual ~ExpressionDebugger() {}
  
  virtual void traverse_int(IntConstant *);
  virtual void traverse_uint(UIntConstant *);
  virtual void traverse_float(FloatConstant *);
  virtual void traverse_char(CharConstant *);
  virtual void traverse_string(StringConstant *);
  virtual void traverse_array(ArrayConstant *);
  virtual void traverse_enum(EnumConstant *);
  virtual void traverse_variable(Variable *);
  virtual void traverse_call(FunctionCall *);

  virtual void traverse_unary(UnaryExpr *);
  virtual void traverse_binary(BinaryExpr *);
  virtual void traverse_trinary(TrinaryExpr *);
  virtual void traverse_assign(AssignExpr *);
  virtual void traverse_rel(RelExpr *);
  virtual void traverse_cast(CastExpr *);
  virtual void traverse_sizeof(SizeofExpr *);
  virtual void traverse_index(IndexExpr *);

  void debug_label(Label *);
  void debug_decl(Decl *);
};

class StatementDebugger : public StatementVisitor, private Debugger
{
public:
  StatementDebugger(std::ostream &os) : Debugger(os) {}
  StatementDebugger(Debugger &d) : Debugger(d) {}
  virtual ~StatementDebugger() {}
  
  virtual void traverse_statement(Statement *);
  virtual void traverse_file_line(FileLineStemnt *);
  virtual void traverse_include(InclStemnt *);
  virtual void traverse_end_include(EndInclStemnt *);
  virtual void traverse_expression(ExpressionStemnt *);
  virtual void traverse_if(IfStemnt *);
  virtual void traverse_switch(SwitchStemnt *);
  virtual void traverse_for(ForStemnt *);
  virtual void traverse_while(WhileStemnt *);
  virtual void traverse_do_while(DoWhileStemnt *);
  virtual void traverse_goto(GotoStemnt *);
  virtual void traverse_return(ReturnStemnt *);
  virtual void traverse_declaration(DeclStemnt *);
  virtual void traverse_typedef(TypedefStemnt *);
  virtual void traverse_block(Block *);
  virtual void traverse_function_definition(FunctionDef *);

  void debug_file(File *);
};

#endif
