//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/Trace.hh>
#include "Translator.hh"
#include "Debugger.hh"
#include "Declaration.hh"
#include "Statement.hh"
#include "File.hh"

using namespace Synopsis;

namespace 
{
  std::string extract_symbol(const Symbol *s) { return s ? s->name : "<nil>";}
}

void Translator::define_type(AST::ScopedName name, AST::Type type)
{
  ast.types().attr("__setitem__")(Python::Tuple(name, type));
}

bool TypeTranslator::find_type(const std::string &name)
{
  Python::Object get = ast.types().attr("get");
  this->type = AST::Type(get(Python::Tuple(AST::ScopedName(name))), false);
  return this->type;
}

void TypeTranslator::traverse_base(BaseType *type)
{
  Trace trace("TypeTranslator::traverse_base");
  bool anonymous = (type->typemask == BaseType::Struct ||
		    type->typemask == BaseType::Union ||
		    type->typemask == BaseType::Enum) && !type->tag;

  if (type->typemask == BaseType::Struct || type->typemask == BaseType::Union)
  {
    std::string name = anonymous ? this->symbol : extract_symbol(type->tag);
    std::string metatype = type->typemask == BaseType::Struct ? "struct" : "union";

    // if stDefn is non-empty, we have to declare a new struct/union type
    if (type->stDefn)
    {
      AST::ASTKit kit;
      AST::TypeKit types;
      AST::Class c = kit.create_class(this->file, 0, "C", metatype, name);
      this->type = types.create_declared("C", name, c);
      define_type(name, this->type);

      Python::List declarations = c.declarations();
      ExpressionTranslator et(*this);
      for (size_t i = 0; i != type->stDefn->nComponents; ++i)
      {
	et.translate_decl(type->stDefn->components[i]);
	declarations.append(et.declaration);
      }
      if (anonymous) this->declaration = c;
      else scope.append(c);
    }
    // else we look it up and set the type to 'declared' (if found) or 'unknown'
    else
    {
      AST::ASTKit kit;
      AST::TypeKit types;
      if (!find_type(name))
      {
	this->type = types.create_unknown("C", name);
	define_type(name, this->type);
      }
    }
  }
  else if (type->typemask == BaseType::Enum)
  {
    std::string name = anonymous ? this->symbol : extract_symbol(type->tag);
    std::string metatype = "enum";

    if (type->enDefn)
    {
      AST::ASTKit kit;
      AST::TypeKit types;
      
      AST::Enumerators enumerators;
      ExpressionTranslator et(*this);
      for (size_t i = 0; i != type->enDefn->nElements; ++i)
      {
	//       et.translate_decl(type->enDefn->values[i]);
	std::string name = extract_symbol(type->enDefn->names[i]);
	enumerators.append(kit.create_enumerator(this->file, 0, "C", name, ""));
      }

      AST::Enum e = kit.create_enum(this->file, 0, "C", name, enumerators);
      this->type = types.create_declared("C", name, e);
      define_type(name, this->type);

      if (anonymous) this->declaration = e;
      else scope.append(e);
    }
    // else we look it up and set the type to 'declared' (if found) or 'unknown'
    else
    {
      AST::ASTKit kit;
      AST::TypeKit types;
      if (!find_type(name))
      {
	this->type = types.create_unknown("C", name);
	define_type(name, this->type);
      }
    }
  }
  else
  {
    std::string name = type->typemask == BaseType::UserType ? extract_symbol(type->typeName) : BaseType::to_string(type->typemask);
    if (type->typemask == BaseType::UserType)

    AST::ScopedName name(BaseType::to_string(type->typemask));
    if (!find_type(BaseType::to_string(type->typemask)))
    {
      AST::TypeKit kit;
      this->type = kit.create_base("C", name);
      define_type(name, this->type);
    }
  }
}

void TypeTranslator::traverse_ptr(PtrType *type)
{
  Trace trace("TypeTranslator::traverse_ptr");
  // get the wrapped type
  type->subType->accept(this);

  static Python::Object module = Python::Object::import("Synopsis.AST");

  if (this->declaration && this->declaration.is_instance(module.attr("Function")))
  {
    AST::Function f = this->declaration; // 'downcast'
    // now change type to 'Function' and declaration to 'none'(it will be dealt with later)
    AST::TypeKit types;
    AST::TypeList parameters;
    for (AST::Function::Parameters::iterator i = f.parameters().begin();
	 i != f.parameters().end();
	 ++i)
      parameters.append(i->type());
    this->type = types.create_function_ptr("C", f.return_type(),
					   AST::Modifiers(std::string("")),
					   parameters);
    this->declaration = AST::Declaration(); // None
  }
  else
  {
    // create and define the wrapper type
    AST::TypeKit kit;
    this->type = kit.create_modifier("C", this->type,
				     AST::Modifiers(),
				     AST::Modifiers(Python::Tuple("*")));
  }
}

void TypeTranslator::traverse_array(ArrayType *type)
{
  Trace trace("TypeTranslator::traverse_array");
  // get the wrapped type
  type->subType->accept(this);

  // create and define the wrapper type
  AST::TypeKit kit;
  // FIXME: Synopsis.Type.Array takes a list of integers for the array
  // sizes, which excludes compile-time-constant expressions other than
  // integer constants. For now use '1' here, as this has to be addressed in
  // the synopsis type hierarchy
  Python::TypedList<int> sizes;
  sizes.append(1);
  this->type = kit.create_array("C", this->type, sizes);
}

void TypeTranslator::traverse_bit_field(BitFieldType *)
{
  Trace trace("TypeTranslator::traverse_bit_field");
}

// a FunctionType is encountered in function declarations as well
// as function pointer declarations. As we can't tell the difference
// here, we have to store type and declaration and let the caller
// decide what to do with them
void TypeTranslator::traverse_function(FunctionType *function)
{
  Trace trace("TypeTranslator::traverse_function");
  function->subType->accept(this);
  // save return type
  AST::Type return_type = this->type;
  // collect parameters
  AST::Function::Parameters parameters;
  AST::ASTKit kit;
  for (size_t i = 0; i != function->nArgs; ++i)
  {
    ExpressionTranslator ed(*this);
    ed.translate_decl(function->args[i]);
    AST::Parameter p = kit.create_parameter(AST::Modifiers(),
					    ed.type,
					    AST::Modifiers(),
					    extract_symbol(function->args[i]->name),
					    "");
    parameters.append(p);
  }
  AST::ScopedName name(this->symbol);
  AST::Function func = kit.create_function(this->file, -1, "C", "function",
					   AST::Modifiers(),
					   return_type,
					   name,
					   this->symbol);
  func.parameters().extend(parameters);
  this->declaration = func;
//   this->scope.append(func);
  AST::TypeKit types;
  this->type = types.create_declared("C", name, func);
  define_type(name, this->type);
}

void ExpressionTranslator::traverse_int(IntConstant *)
{
  Trace trace("ExpressionTranslator::traverse_int");
}

void ExpressionTranslator::traverse_uint(UIntConstant *)
{
  Trace trace("ExpressionTranslator::traverse_uint");
}

void ExpressionTranslator::traverse_float(FloatConstant *)
{
  Trace trace("ExpressionTranslator::traverse_float");
}

void ExpressionTranslator::traverse_char(CharConstant *)
{
  Trace trace("ExpressionTranslator::traverse_char");
}

void ExpressionTranslator::traverse_string(StringConstant *)
{
  Trace trace("ExpressionTranslator::traverse_string");
}

void ExpressionTranslator::traverse_array(ArrayConstant *)
{
  Trace trace("ExpressionTranslator::traverse_array");
}

void ExpressionTranslator::traverse_enum(EnumConstant *)
{
  Trace trace("ExpressionTranslator::traverse_enum");
}

void ExpressionTranslator::traverse_variable(Variable *)
{
  Trace trace("ExpressionTranslator::traverse_variable");
}

void ExpressionTranslator::traverse_call(FunctionCall *)
{
  Trace trace("ExpressionTranslator::traverse_call");
}

void ExpressionTranslator::traverse_unary(UnaryExpr *)
{
  Trace trace("ExpressionTranslator::traverse_unary");
}

void ExpressionTranslator::traverse_binary(BinaryExpr *)
{
  Trace trace("ExpressionTranslator::traverse_binary");
}

void ExpressionTranslator::traverse_trinary(TrinaryExpr *)
{
  Trace trace("ExpressionTranslator::traverse_trinary");
}

void ExpressionTranslator::traverse_assign(AssignExpr *)
{
  Trace trace("ExpressionTranslator::traverse_assign");
}

void ExpressionTranslator::traverse_rel(RelExpr *)
{
  Trace trace("ExpressionTranslator::traverse_rel");
}

void ExpressionTranslator::traverse_cast(CastExpr *)
{
  Trace trace("ExpressionTranslator::traverse_cast");
}

void ExpressionTranslator::traverse_sizeof(SizeofExpr *)
{
  Trace trace("ExpressionTranslator::traverse_sizeof");
}

void ExpressionTranslator::traverse_index(IndexExpr *)
{
  Trace trace("ExpressionTranslator::traverse_index");
}

void ExpressionTranslator::translate_decl(Decl *node)
{
  Trace trace("ExpressionTranslator::translate_decl");
  std::string symbol = extract_symbol(node->name);
  TypeTranslator tt(*this, symbol);
  node->form->accept(&tt);
  this->declaration = tt.declaration;
  this->type = tt.type;
  if (symbol == "<nil>") return; // we are done
  AST::ASTKit kit;
  AST::ScopedName name(symbol);
  if (!this->declaration)
  {
    if (node->storage == Storage::Typedef)
    {
      this->declaration = kit.create_typedef(this->file, 0, "C",
					     "typedef",
					     name,
					     tt.type, false);
      AST::TypeKit types;
      this->type = types.create_declared("C", name, this->declaration);
      define_type(name, this->type);
    }
    else
      this->declaration = kit.create_variable(this->file, 0, "C",
					      "variable",
					      name,
					      tt.type, false);
  }
}

void StatementTranslator::traverse_statement(Statement *)
{
  Trace trace("StatementTranslator::traverse_statement");
}

void StatementTranslator::traverse_file_line(FileLineStemnt *)
{
  Trace trace("StatementTranslator::traverse_file_line");
}

void StatementTranslator::traverse_include(InclStemnt *)
{
  Trace trace("StatementTranslator::traverse_include");
}

void StatementTranslator::traverse_end_include(EndInclStemnt *)
{
  Trace trace("StatementTranslator::traverse_end_include");
}

void StatementTranslator::traverse_expression(ExpressionStemnt *)
{
  Trace trace("StatementTranslator::traverse_expression");
}

void StatementTranslator::traverse_if(IfStemnt *)
{
  Trace trace("StatementTranslator::traverse_if");
}

void StatementTranslator::traverse_switch(SwitchStemnt *)
{
  Trace trace("StatementTranslator::traverse_switch");
}

void StatementTranslator::traverse_for(ForStemnt *)
{
  Trace trace("StatementTranslator::traverse_for");
}

void StatementTranslator::traverse_while(WhileStemnt *)
{
  Trace trace("StatementTranslator::traverse_while");
}

void StatementTranslator::traverse_do_while(DoWhileStemnt *)
{
  Trace trace("StatementTranslator::traverse_do_while");
}

void StatementTranslator::traverse_goto(GotoStemnt *)
{
  Trace trace("StatementTranslator::traverse_goto");
}

void StatementTranslator::traverse_return(ReturnStemnt *)
{
  Trace trace("StatementTranslator::traverse_return");
}

void StatementTranslator::traverse_declaration(DeclStemnt *node)
{
  Trace trace("StatementTranslator::traverse_declaration");

  if (!node->decls.empty())
  {
    for (DeclVector::const_iterator i = node->decls.begin();
	 i != node->decls.end(); ++i)
    {
      ExpressionTranslator ed(*this);
      ed.translate_decl(*i);
      if (ed.declaration) scope.append(ed.declaration);
    }
  }
}

void StatementTranslator::traverse_typedef(TypedefStemnt *)
{
  Trace trace("StatementTranslator::traverse_typedef");
}

void StatementTranslator::traverse_block(Block *)
{
  Trace trace("StatementTranslator::traverse_block");
}

void StatementTranslator::traverse_function_definition(FunctionDef *)
{
  Trace trace("StatementTranslator::traverse_function_definition");
}

void StatementTranslator::translate_file(File *f)
{
  set_file(f->my_name, f->my_name);
  size_t include = 0;
  for (Statement *statement = f->my_head; statement; statement = statement->next)
  {
    if (include > 0)
    {
      if (statement->isEndInclude()) --include;
      else if (statement->isInclude()) ++include;
    }
    else
    {
      if (statement->isInclude()) include++;
      statement->accept(this);
    }
  }
}

void StatementTranslator::set_file(const std::string &long_name,
				   const std::string &short_name)
{
  file = ast.files().get(long_name);
  if (!file) // hmm, so the file wasn't preprocessed...
  {
    Synopsis::AST::ASTKit kit;
    file = kit.create_source_file(long_name, short_name, "C");
    ast.files().set(short_name, file);
  }
}
