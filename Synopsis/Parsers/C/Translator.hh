//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Translator_hh
#define _Translator_hh

#include <Synopsis/AST/ASTKit.hh>
#include <Synopsis/AST/TypeKit.hh>
#include <iostream>
#include <string>
#include <vector>
#include <stack>

#include <Visitor.hh>
#include <Declaration.hh>
#include <File.hh>

struct Translator
{
  Translator(Synopsis::AST::AST a, Synopsis::Python::List s, bool v, bool d)
    : ast(a), scope(s), verbose(v), debug(d) {}
  Translator(Translator &t)
    : ast(t.ast), file(t.file), scope(t.scope), verbose(t.verbose), debug(t.debug) {}

protected:
  void define_type(Synopsis::AST::ScopedName, Synopsis::AST::Type);

  Synopsis::AST::AST        ast;
  Synopsis::AST::SourceFile file;
  Synopsis::Python::List    scope;
  bool                      verbose;
  bool                      debug;
};

struct TypeTranslator : TypeVisitor, Translator
{
  TypeTranslator(Translator &t, const std::string &s) : Translator(t), symbol(s) {}

  virtual void traverse_base(BaseType *);
  virtual void traverse_ptr(PtrType *);
  virtual void traverse_array(ArrayType *);
  virtual void traverse_bit_field(BitFieldType *);
  virtual void traverse_function(FunctionType *);

  Synopsis::AST::Type        type;
  Synopsis::AST::Declaration declaration;
  std::string                symbol;

private:
  bool find_type(const std::string &);
};

struct ExpressionTranslator : ExpressionVisitor, Translator
{
  ExpressionTranslator(Translator &t) : Translator(t) {}

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

  void translate_decl(Decl *);

  Synopsis::AST::Type        type;
  Synopsis::AST::Declaration declaration;
};

struct StatementTranslator : StatementVisitor, Translator
{
  StatementTranslator(Synopsis::AST::AST a, Synopsis::Python::List s, bool v, bool d)
    : Translator(a, s, v, d) {}
  StatementTranslator(Translator &t) : Translator(t) {}

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

  void translate_file(File *);  

private:
  void set_file(const std::string &, const std::string &);

};

#endif
