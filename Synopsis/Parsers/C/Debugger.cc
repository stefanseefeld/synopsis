//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Declaration.hh>
#include <Statement.hh>
#include <File.hh>
#include <Expression.hh>
#include <Symbol.hh>
#include <Debugger.hh>

namespace 
{
  std::string extract_symbol(const Symbol *s) { return s ? s->name : "<nil>";}
}

void TypeDebugger::traverse_base(BaseType *type)
{
  indent() << "BaseType : typemask=" << type->typemask
	   << '(' << BaseType::to_string(type->typemask) << ')'
	   << " qualifier=" << type->qualifier.to_string()
	   << " tag=" << extract_symbol(type->tag)
	   << std::endl;
  incr_indent();
  if (type->stDefn)
  {
    indent() << "StructDef : tag=" << extract_symbol(type->stDefn->tag) << std::endl;
    ExpressionDebugger ed(*this);
    for (size_t i = 0; i != type->stDefn->nComponents; ++i)
      ed.debug_decl(type->stDefn->components[i]);
  }
  else if (type->enDefn)
  {
    indent() << "EnumDef : struct tag=" 
	     << extract_symbol(type->enDefn->tag) << std::endl;
    ExpressionDebugger ed(*this);
    for (size_t i = 0; i != type->enDefn->nElements; ++i)
    {
      indent() << " name=" << extract_symbol(type->enDefn->names[i]);
      if (type->enDefn->values[i])
	type->enDefn->values[i]->accept(&ed);
    }
    indent() << '\n';
  }
  decr_indent();
}

void TypeDebugger::traverse_ptr(PtrType *type)
{
  indent() << "PtrType : qualifier=" << type->qualifier.to_string() << std::endl;
  incr_indent();
  type->subType->accept(this);
  decr_indent();
}

void TypeDebugger::traverse_array(ArrayType *type)
{
  indent() << "ArrayType" << std::endl;
  incr_indent();
  ExpressionDebugger ed(*this);
  type->size->accept(&ed);
  TypeDebugger td(*this);
  type->subType->accept(&td);
  decr_indent();  
}

void TypeDebugger::traverse_bit_field(BitFieldType *type)
{
  indent() << "ButFieldType" << std::endl;
  incr_indent();
  ExpressionDebugger ed(*this);
  type->size->accept(&ed);
  TypeDebugger td(*this);
  type->subType->accept(&td);
  decr_indent();  
}

void TypeDebugger::traverse_function(FunctionType *type)
{
  indent() << "FunctionType : K&R style=" << type->KnR_decl << std::endl;
  incr_indent();
  type->subType->accept(this);
  ExpressionDebugger ed(*this);
  for (size_t i = 0; i != type->nArgs; ++i)
    ed.debug_decl(type->args[i]);
  decr_indent();  
}

void ExpressionDebugger::traverse_int(IntConstant *node)
{
  indent() << "IntConstant: lng=" << node->lng << std::endl;
}

void ExpressionDebugger::traverse_uint(UIntConstant *node)
{
}

void ExpressionDebugger::traverse_float(FloatConstant *node)
{
}

void ExpressionDebugger::traverse_char(CharConstant *node)
{
}

void ExpressionDebugger::traverse_string(StringConstant *node)
{
}

void ExpressionDebugger::traverse_array(ArrayConstant *node)
{
}

void ExpressionDebugger::traverse_enum(EnumConstant *node)
{
}

void ExpressionDebugger::traverse_variable(Variable *node)
{
}

void ExpressionDebugger::traverse_call(FunctionCall *node)
{
}

void ExpressionDebugger::traverse_unary(UnaryExpr *node)
{
}

void ExpressionDebugger::traverse_binary(BinaryExpr *node)
{
}

void ExpressionDebugger::traverse_trinary(TrinaryExpr *node)
{
}

void ExpressionDebugger::traverse_assign(AssignExpr *node)
{
}

void ExpressionDebugger::traverse_rel(RelExpr *node)
{
}

void ExpressionDebugger::traverse_cast(CastExpr *node)
{
}

void ExpressionDebugger::traverse_sizeof(SizeofExpr *node)
{
}

void ExpressionDebugger::traverse_index(IndexExpr *node)
{
}

void ExpressionDebugger::debug_label(Label *node)
{
}

void ExpressionDebugger::debug_decl(Decl *node)
{
  indent() << "Decl : storage=" << node->storage.to_string()
	   << " symbol=" << extract_symbol(node->name)
	   << (node->attrib ? "attrib=" + node->attrib->to_string() : "")
	   << std::endl;
  incr_indent();
  TypeDebugger td(*this);
  node->form->accept(&td);
  if (node->initializer)
  {
    ExpressionDebugger ed(*this);
    node->initializer->accept(&ed);
  }
  decr_indent();
}

void StatementDebugger::traverse_statement(Statement *node)
{
}

void StatementDebugger::traverse_file_line(FileLineStemnt *node)
{
}

void StatementDebugger::traverse_include(InclStemnt *node)
{
}

void StatementDebugger::traverse_end_include(EndInclStemnt *node)
{
}

void StatementDebugger::traverse_expression(ExpressionStemnt *node)
{
}

void StatementDebugger::traverse_if(IfStemnt *node)
{
}

void StatementDebugger::traverse_switch(SwitchStemnt *node)
{
}

void StatementDebugger::traverse_for(ForStemnt *node)
{
}

void StatementDebugger::traverse_while(WhileStemnt *node)
{
}

void StatementDebugger::traverse_do_while(DoWhileStemnt *node)
{
}

void StatementDebugger::traverse_goto(GotoStemnt *node)
{
}

void StatementDebugger::traverse_return(ReturnStemnt *node)
{
}

void StatementDebugger::traverse_declaration(DeclStemnt *node)
{
  indent() << "DeclStemnt" << std::endl;
  incr_indent();
  ExpressionDebugger ed(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ed.debug_label(*i);

  if (!node->decls.empty())
    for (DeclVector::const_iterator i = node->decls.begin();
	 i != node->decls.end();
	 ++i)
      ed.debug_decl(*i);
  decr_indent();
}

void StatementDebugger::traverse_typedef(TypedefStemnt *node)
{
}

void StatementDebugger::traverse_block(Block *node)
{
}

void StatementDebugger::traverse_function_definition(FunctionDef *node)
{
}

void StatementDebugger::debug_file(File *file)
{
  size_t include = 0;
  for (Statement *statement = file->my_head; statement; statement = statement->next)
  {
    if (include > 0)
    {
      if (statement->isEndInclude()) include--;
      else if (statement->isInclude()) include++;
    }
    else
    {
      if (statement->isInclude()) include++;
      statement->accept(this);
    }
  }
}
