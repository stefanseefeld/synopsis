//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Visitor_hh
#define _Visitor_hh

class BaseType;
class PtrType;
class ArrayType;
class BitFieldType;
class FunctionType;

class IntConstant;
class UIntConstant;
class FloatConstant;
class CharConstant;
class StringConstant;
class ArrayConstant;
class EnumConstant;
class Variable;
class FunctionCall;

class UnaryExpr;
class BinaryExpr;
class TrinaryExpr;
class AssignExpr;
class RelExpr;
class CastExpr;
class SizeofExpr;
class IndexExpr;

class Label;
class Decl;

class Statement;
class FileLineStemnt;
class InclStemnt;
class EndInclStemnt;
class ExpressionStemnt;
class IfStemnt;
class SwitchStemnt;
class ForStemnt;
class WhileStemnt;
class DoWhileStemnt;
class GotoStemnt;
class ReturnStemnt;
class DeclStemnt;
class TypedefStemnt;
class Block;
class FunctionDef;

class TypeVisitor
{
public:
  virtual ~TypeVisitor() {}

  virtual void traverse_base(BaseType *) {}
  virtual void traverse_ptr(PtrType *) {}
  virtual void traverse_array(ArrayType *) {}
  virtual void traverse_bit_field(BitFieldType *) {}
  virtual void traverse_function(FunctionType *) {}
};

class ExpressionVisitor
{
public:
  virtual ~ExpressionVisitor() {}
  
  virtual void traverse_int(IntConstant *) {}
  virtual void traverse_uint(UIntConstant *) {}
  virtual void traverse_float(FloatConstant *) {}
  virtual void traverse_char(CharConstant *) {}
  virtual void traverse_string(StringConstant *) {}
  virtual void traverse_array(ArrayConstant *) {}
  virtual void traverse_enum(EnumConstant *) {}
  virtual void traverse_variable(Variable *) {}
  virtual void traverse_call(FunctionCall *) {}

  virtual void traverse_unary(UnaryExpr *) {}
  virtual void traverse_binary(BinaryExpr *) {}
  virtual void traverse_trinary(TrinaryExpr *) {}
  virtual void traverse_assign(AssignExpr *) {}
  virtual void traverse_rel(RelExpr *) {}
  virtual void traverse_cast(CastExpr *) {}
  virtual void traverse_sizeof(SizeofExpr *) {}
  virtual void traverse_index(IndexExpr *) {}
};

class StatementVisitor
{
public:
  virtual ~StatementVisitor() {}
  
  virtual void traverse_statement(Statement *) {}
  virtual void traverse_file_line(FileLineStemnt *) {}
  virtual void traverse_include(InclStemnt *) {}
  virtual void traverse_end_include(EndInclStemnt *) {}
  virtual void traverse_expression(ExpressionStemnt *) {}
  virtual void traverse_if(IfStemnt *) {}
  virtual void traverse_switch(SwitchStemnt *) {}
  virtual void traverse_for(ForStemnt *) {}
  virtual void traverse_while(WhileStemnt *) {}
  virtual void traverse_do_while(DoWhileStemnt *) {}
  virtual void traverse_goto(GotoStemnt *) {}
  virtual void traverse_return(ReturnStemnt *) {}
  virtual void traverse_declaration(DeclStemnt *) {}
  virtual void traverse_typedef(TypedefStemnt *) {}
  virtual void traverse_block(Block *) {}
  virtual void traverse_function_definition(FunctionDef *) {}
};

#endif
