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

  virtual void traverse_base(BaseType *) = 0;
  virtual void traverse_ptr(PtrType *) = 0;
  virtual void traverse_array(ArrayType *) = 0;
  virtual void traverse_bit_field(BitFieldType *) = 0;
  virtual void traverse_function(FunctionType *) = 0;
};

class ExpressionVisitor
{
public:
  virtual ~ExpressionVisitor() {}
  
  virtual void traverse_int(IntConstant *) = 0;
  virtual void traverse_uint(UIntConstant *) = 0;
  virtual void traverse_float(FloatConstant *) = 0;
  virtual void traverse_char(CharConstant *) = 0;
  virtual void traverse_string(StringConstant *) = 0;
  virtual void traverse_array(ArrayConstant *) = 0;
  virtual void traverse_enum(EnumConstant *) = 0;
  virtual void traverse_variable(Variable *) = 0;
  virtual void traverse_call(FunctionCall *) = 0;

  virtual void traverse_unary(UnaryExpr *) = 0;
  virtual void traverse_binary(BinaryExpr *) = 0;
  virtual void traverse_trinary(TrinaryExpr *) = 0;
  virtual void traverse_assign(AssignExpr *) = 0;
  virtual void traverse_rel(RelExpr *) = 0;
  virtual void traverse_cast(CastExpr *) = 0;
  virtual void traverse_sizeof(SizeofExpr *) = 0;
  virtual void traverse_index(IndexExpr *) = 0;
};

class StatementVisitor
{
public:
  virtual ~StatementVisitor() {}
  
  virtual void traverse_statement(Statement *) = 0;
  virtual void traverse_file_line(FileLineStemnt *) = 0;
  virtual void traverse_include(InclStemnt *) = 0;
  virtual void traverse_end_include(EndInclStemnt *) = 0;
  virtual void traverse_expression(ExpressionStemnt *) = 0;
  virtual void traverse_if(IfStemnt *) = 0;
  virtual void traverse_switch(SwitchStemnt *) = 0;
  virtual void traverse_for(ForStemnt *) = 0;
  virtual void traverse_while(WhileStemnt *) = 0;
  virtual void traverse_do_while(DoWhileStemnt *) = 0;
  virtual void traverse_goto(GotoStemnt *) = 0;
  virtual void traverse_return(ReturnStemnt *) = 0;
  virtual void traverse_declaration(DeclStemnt *) = 0;
  virtual void traverse_typedef(TypedefStemnt *) = 0;
  virtual void traverse_block(Block *) = 0;
  virtual void traverse_function_definition(FunctionDef *) = 0;
};

#endif
