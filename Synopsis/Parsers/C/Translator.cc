//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "Translator.hh"
#include "PrintTraversal.hh"
#include "Declaration.hh"
#include "Statement.hh"
#include "File.hh"
#include "Trace.hh"

namespace AST = Synopsis::AST;
namespace Python = Synopsis;

PrintTraversal printer(std::cout, false);

Translator::Translator(PyObject *ast, bool verbose, bool debug)
  : my_ast(ast), my_verbose(verbose), my_debug(debug)
{
}

Translator::~Translator()
{
}

void Translator::traverse_base(BaseType *)
{
  Trace trace("Translator::traverse_base");
}

void Translator::traverse_ptr(PtrType *)
{
  Trace trace("Translator::traverse_ptr");
}

void Translator::traverse_array(ArrayType *)
{
  Trace trace("Translator::traverse_array");
}

void Translator::traverse_bit_field(BitFieldType *)
{
  Trace trace("Translator::traverse_bit_field");
}

void Translator::traverse_function(FunctionType *function)
{
  Trace trace("Translator::traverse_function");
  // save function name
  std::string name = my_current_symbol;
  function->subType->accept(this);
  // save return type
  AST::Type return_type = my_current_type;
  AST::Function::Parameters parameters;
  for (size_t i = 0; i != function->nArgs; ++i)
  {
    traverse_decl(function->args[i]);
    // add parameter here
  }
  AST::Function func = my_kit.create_function(my_file, -1, "C", "function",
					      AST::Modifiers(),
					      return_type,
					      AST::ScopedName(name),
					      name);
  my_ast.declarations().append(func);
}

void Translator::traverse_symbol(Symbol *symbol)
{
  Trace trace("Translator::traverse_symbol");
  if (symbol) my_current_symbol = symbol->name; // what is 'symbol->entry' ?? 
}

void Translator::traverse_int(IntConstant *)
{
  Trace trace("Translator::traverse_int");
}

void Translator::traverse_uint(UIntConstant *)
{
  Trace trace("Translator::traverse_uint");
}

void Translator::traverse_float(FloatConstant *)
{
  Trace trace("Translator::traverse_float");
}

void Translator::traverse_char(CharConstant *)
{
  Trace trace("Translator::traverse_char");
}

void Translator::traverse_string(StringConstant *)
{
  Trace trace("Translator::traverse_string");
}

void Translator::traverse_array(ArrayConstant *)
{
  Trace trace("Translator::traverse_array");
}

void Translator::traverse_enum(EnumConstant *)
{
  Trace trace("Translator::traverse_enum");
}

void Translator::traverse_variable(Variable *)
{
  Trace trace("Translator::traverse_variable");
}

void Translator::traverse_call(FunctionCall *)
{
  Trace trace("Translator::traverse_call");
}

void Translator::traverse_unary(UnaryExpr *)
{
  Trace trace("Translator::traverse_unary");
}

void Translator::traverse_binary(BinaryExpr *)
{
  Trace trace("Translator::traverse_binary");
}

void Translator::traverse_trinary(TrinaryExpr *)
{
  Trace trace("Translator::traverse_trinary");
}

void Translator::traverse_assign(AssignExpr *)
{
  Trace trace("Translator::traverse_assign");
}

void Translator::traverse_rel(RelExpr *)
{
  Trace trace("Translator::traverse_rel");
}

void Translator::traverse_cast(CastExpr *)
{
  Trace trace("Translator::traverse_cast");
}

void Translator::traverse_sizeof(SizeofExpr *)
{
  Trace trace("Translator::traverse_sizeof");
}

void Translator::traverse_index(IndexExpr *)
{
  Trace trace("Translator::traverse_index");
}

void Translator::traverse_label(Label *)
{
  Trace trace("Translator::traverse_label");
}

void Translator::traverse_decl(Decl *node)
{
  Trace trace("Translator::traverse_decl");
  traverse_symbol(node->name);
  node->form->accept(this);
}

void Translator::traverse_statement(Statement *)
{
  Trace trace("Translator::traverse_statement");
}

void Translator::traverse_file_line(FileLineStemnt *)
{
  Trace trace("Translator::traverse_file_line");
}

void Translator::traverse_include(InclStemnt *)
{
  Trace trace("Translator::traverse_include");
}

void Translator::traverse_end_include(EndInclStemnt *)
{
  Trace trace("Translator::traverse_end_include");
}

void Translator::traverse_expression(ExpressionStemnt *)
{
  Trace trace("Translator::traverse_expression");
}

void Translator::traverse_if(IfStemnt *)
{
  Trace trace("Translator::traverse_if");
}

void Translator::traverse_switch(SwitchStemnt *)
{
  Trace trace("Translator::traverse_switch");
}

void Translator::traverse_for(ForStemnt *)
{
  Trace trace("Translator::traverse_for");
}

void Translator::traverse_while(WhileStemnt *)
{
  Trace trace("Translator::traverse_while");
}

void Translator::traverse_do_while(DoWhileStemnt *)
{
  Trace trace("Translator::traverse_do_while");
}

void Translator::traverse_goto(GotoStemnt *)
{
  Trace trace("Translator::traverse_goto");
}

void Translator::traverse_return(ReturnStemnt *)
{
  Trace trace("Translator::traverse_return");
}

void Translator::traverse_declaration(DeclStemnt *node)
{
  Trace trace("Translator::traverse_declaration");
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    traverse_label(*i);
  
  if (!node->decls.empty())
  {
    for (DeclVector::const_iterator i = node->decls.begin();
	 i != node->decls.end(); ++i)
    {
      traverse_decl(*i);
    }
  }
}

void Translator::traverse_typedef(TypedefStemnt *)
{
  Trace trace("Translator::traverse_typedef");
}

void Translator::traverse_block(Block *)
{
  Trace trace("Translator::traverse_block");
}

void Translator::traverse_function_definition(FunctionDef *)
{
  Trace trace("Translator::traverse_function_definition");
}

void Translator::traverse_file(File *file)
{
  Trace trace("Translator::traverse_file");

  my_file = my_ast.files().get(file->my_name);
  if (!my_file) // hmm, so the file wasn't preprocessed...
  {
    my_file = my_kit.create_source_file(file->my_name, file->my_name, "C");
    my_ast.files().set(file->my_name, my_file);
  }
  for (Statement *statement = file->my_head; statement; statement = statement->next)
    statement->accept(this);
}

