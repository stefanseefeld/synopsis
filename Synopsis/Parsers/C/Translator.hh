// $Id: Translator.hh,v 1.1 2003/08/20 02:16:37 stefan Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2003 Stefan Seefeld
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// $Log: Translator.hh,v $
// Revision 1.1  2003/08/20 02:16:37  stefan
// first steps towards a C parser backend (based on the ctool)
//
//

#ifndef _Translator_hh
#define _Translator_hh

#include <iostream>
#include <string>
#include <vector>

#include <Traversal.hh>

class Translator : public Traversal
{
public:
  Translator();
  virtual ~Translator();
  virtual void traverse_base(BaseType *);
  virtual void traverse_ptr(PtrType *);
  virtual void traverse_array(ArrayType *);
  virtual void traverse_bit_field(BitFieldType *);
  virtual void traverse_function(FunctionType *);

  virtual void traverse_symbol(Symbol *);

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

  virtual void traverse_label(Label *);
  virtual void traverse_decl(Decl *);

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
  virtual void traverse_unit(File *);  
private:
};

#endif
