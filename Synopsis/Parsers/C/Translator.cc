// $Id: Translator.cc,v 1.1 2003/08/20 02:16:37 stefan Exp $
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
// $Log: Translator.cc,v $
// Revision 1.1  2003/08/20 02:16:37  stefan
// first steps towards a C parser backend (based on the ctool)
//
//

#include <ctool/ctool.h>
#include "Translator.hh"

Translator::Translator()
{
}

Translator::~Translator()
{
}

void Translator::traverse_base(BaseType *)
{
}

void Translator::traverse_ptr(PtrType *)
{
}

void Translator::traverse_array(ArrayType *)
{
}

void Translator::traverse_bit_field(BitFieldType *)
{
}

void Translator::traverse_function(FunctionType *)
{
}


void Translator::traverse_symbol(Symbol *)
{
}

void Translator::traverse_int(IntConstant *)
{
}

void Translator::traverse_uint(UIntConstant *)
{
}

void Translator::traverse_float(FloatConstant *)
{
}

void Translator::traverse_char(CharConstant *)
{
}

void Translator::traverse_string(StringConstant *)
{
}

void Translator::traverse_array(ArrayConstant *)
{
}

void Translator::traverse_enum(EnumConstant *)
{
}

void Translator::traverse_variable(Variable *)
{
}

void Translator::traverse_call(FunctionCall *)
{
}

void Translator::traverse_unary(UnaryExpr *)
{
}

void Translator::traverse_binary(BinaryExpr *)
{
}

void Translator::traverse_trinary(TrinaryExpr *)
{
}

void Translator::traverse_assign(AssignExpr *)
{
}

void Translator::traverse_rel(RelExpr *)
{
}

void Translator::traverse_cast(CastExpr *)
{
}

void Translator::traverse_sizeof(SizeofExpr *)
{
}

void Translator::traverse_index(IndexExpr *)
{
}

void Translator::traverse_label(Label *)
{
}

void Translator::traverse_decl(Decl *)
{
}

void Translator::traverse_statement(Statement *)
{
}

void Translator::traverse_file_line(FileLineStemnt *)
{
}

void Translator::traverse_include(InclStemnt *)
{
}

void Translator::traverse_end_include(EndInclStemnt *)
{
}

void Translator::traverse_expression(ExpressionStemnt *)
{
}

void Translator::traverse_if(IfStemnt *)
{
}

void Translator::traverse_switch(SwitchStemnt *)
{
}

void Translator::traverse_for(ForStemnt *)
{
}

void Translator::traverse_while(WhileStemnt *)
{
}

void Translator::traverse_do_while(DoWhileStemnt *)
{
}

void Translator::traverse_goto(GotoStemnt *)
{
}

void Translator::traverse_return(ReturnStemnt *)
{
}

void Translator::traverse_declaration(DeclStemnt *)
{
}

void Translator::traverse_typedef(TypedefStemnt *)
{
}

void Translator::traverse_block(Block *)
{
}

void Translator::traverse_function_definition(FunctionDef *)
{
}

void Translator::traverse_unit(TransUnit *)
{
}

