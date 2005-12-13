//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "TypeEvaluator.hh"
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/Trace.hh>
#include <cassert>

#undef TRUE
#undef FALSE

using Synopsis::Trace;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
using namespace Synopsis::TypeAnalysis;

BinaryPromotion::BinaryPromotion()
{
  // Store all known binary type promotions here.
}

Type const *BinaryPromotion::find(Type const *left, Type const *right)
{
  ReturnTypes::iterator i = my_return_types.find(Key(left, right));
  return i == my_return_types.end() ? 0 : i->second;
}

namespace
{
std::string const TRUE = "true";
std::string const FALSE = "false";


Type const *numeric_type(char const *position, size_t length)
{
  // TODO: If there is no explicit specifier for signedness or long,
  //       simply assume 'int'. Implement 2.13.1 [lex.icon]
  if (length > 2 && *position == '0' && 
      (*(position + 1) == 'x' || *(position + 1) == 'X'))
    // hexadecimal literal
    return &UINT;
  else if (length > 1 && *position == '0' && *(position + 1) != '.')
    // octal literal
    return &UINT;
  else if (TRUE.compare(0, 4, position, length) == 0 ||
	   FALSE.compare(0, 5, position, length) == 0)
    return &BOOL;
  char const *c = position;
  for (size_t i = 0; i != length; ++i, ++c)
  {
    if (*c == '.' || *c == 'E' || *c == 'e')
      // floating-point literal
      return (*(position + length - 1) == 'l' ||
	      *(position + length - 1) == 'L' ? &DOUBLE : &FLOAT);
  }
  // integral literal
  if (*(position + length - 1) == 'l' || 
      *(position + length - 1) == 'L')
    return (*(position + length - 2) == 'u' || 
	    *(position + length - 2) == 'U' ? &ULONG : &LONG);
  return (*(position + length - 1) == 'u' ||
	  *(position + length - 1) == 'U' ? &UINT : &INT);
}

// BinaryPromotion binary_promotion;

}

Type const *TypeEvaluator::evaluate(PT::Node const *node)
{
  my_type = 0;
  if (node) const_cast<PT::Node *>(node)->accept(this);
  return my_type;
}

void TypeEvaluator::visit(PT::Literal *node)
{
  switch (node->type())
  {
    case Token::CharConst: my_type = &CHAR; break;
    case Token::WideCharConst: my_type = &WCHAR; break;
    case Token::StringL: 
      my_type = new Pointer(new CVType(&CHAR, CVType::CONST));
      break;
    case Token::WideStringL:
      my_type = new Pointer(new CVType(&WCHAR, CVType::CONST));
      break;
    case Token::Constant:
      my_type = numeric_type(node->position(), node->length());
      break;
    default:
      std::cerr << "unmatched type for literal " 
 		<< std::string(node->position(), node->length()) 
 		<< ' ' << node->type() << std::endl;
      my_type = &CHAR;
      break;
  }
}

void TypeEvaluator::visit(PT::Identifier *node)
{
  PT::Encoding name(node);
  ST::SymbolSet symbols = my_scope->unqualified_lookup(name, ST::Scope::DEFAULT);
  if (symbols.size() == 1)
  {
//     Symbol const *symbol = *symbols.begin();
//     VariableName const *variable = dynamic_cast<VariableName const *>(symbol);
//     if (variable) my_type.set(variable->type(), variable->scope());
//     else throw TypeError(name, symbol->ptree()->encoded_type());
  }
  else
  {
    // ???
  }
}

void TypeEvaluator::visit(PT::Kwd::This *)
{
  // FIXME: TBD
  //        * find the current scope (object)
  //        * find its type
}

void TypeEvaluator::visit(PT::Name *node)
{
//   SymbolSet symbols = my_scope->lookup(node->encoded_name());
//   if (symbols.size() == 1)
//   {
//     Symbol const *symbol = *symbols.begin();
//     VariableName const *variable = dynamic_cast<VariableName const *>(symbol);
//     if (variable) my_type.set(variable->type(), variable->scope());
//     else throw TypeError(node->encoded_name(), symbol->ptree()->encoded_type());
//   }
//   else
  {
    // ???
  }
}

void TypeEvaluator::visit(PT::FstyleCastExpr *node)
{
//   my_type.set(node->encoded_type(), my_scope);
}

void TypeEvaluator::visit(PT::AssignExpr *node)
{
  node->cdr()->accept(this);
}

void TypeEvaluator::visit(PT::CondExpr *node)
{
  my_type = &BOOL;
//   type_of(PT::nth<2>(node));
}

void TypeEvaluator::visit(PT::InfixExpr *node)
{
  Type const *lhs = evaluate(PT::nth<0>(node));
  PT::Node *op = PT::nth<1>(node);
  assert(op->is_atom() && op->length() <= 2);
  Type const *rhs = evaluate(PT::nth<2>(node));

  Type const *retn = 0;//binary_promotion.find(lhs, rhs);
  if (retn) my_type = retn;
  else
  {
    // look up user-defined binary operators
  }
}

void TypeEvaluator::visit(PT::PmExpr *node)
{
  PT::nth<2>(node)->accept(this);
//   my_type.dereference();
}

void TypeEvaluator::visit(PT::CastExpr *node) 
{
//   my_type.set(PTree::nth<1>(PTree::nth<1>(node))->encoded_type(), my_scope);
}

void TypeEvaluator::visit(PT::UnaryExpr *node)
{
  PT::nth<1>(node)->accept(this);
  PT::Node *op = PT::nth<0>(node);
//   if(*op == '*') my_type.dereference();
//   else if(*op == '&') my_type.reference();
}

void TypeEvaluator::visit(PT::ThrowExpr *)
{
//   my_type.set_void();
}

void TypeEvaluator::visit(PT::SizeofExpr *)
{
//   my_type.set_int();
}

void TypeEvaluator::visit(PT::TypeidExpr *)
{
  // FIXME: Should be type (node->nth<2>()->nth<1>()->encoded_type(), my_scope);
//   my_type.set_int();
}

void TypeEvaluator::visit(PT::TypeofExpr *) 
{
  // FIXME: Should be type (node->nth<2>()->nth<1>()->encoded_type(), my_scope);
//   my_type.set_int();
}

void TypeEvaluator::visit(PT::NewExpr *node)
{
  PT::List *p = node;
  PT::Node *userkey = p->car();
  if(!userkey || !userkey->is_atom()) p = node->cdr(); // user keyword
  if(*p->car() == "::") p = p->cdr();
  PT::Node *type = PT::nth<2>(p);
//   if(*type->car() == '(')
//     my_type.set(PTree::nth<1>(PTree::nth<1>(type))->encoded_type(), my_scope);
//   else
//     my_type.set(PTree::nth<1>(type)->encoded_type(), my_scope);
//   my_type.reference();
}

void TypeEvaluator::visit(PT::DeleteExpr *)
{
//   my_type.set_void();
}

void TypeEvaluator::visit(PT::ArrayExpr *node)
{
  node->car()->accept(this);
//   my_type.dereference();
}

void TypeEvaluator::visit(PT::FuncallExpr *node)
{
  node->car()->accept(this);
//   if(!my_type.is_function())
//     my_type.dereference(); // maybe a pointer to a function
//   my_type.dereference();
}

void TypeEvaluator::visit(PT::PostfixExpr *node)
{
  node->car()->accept(this);
}

void TypeEvaluator::visit(PT::DotMemberExpr *node)
{
  node->car()->accept(this);
//   my_type.set_member(PTree::nth<2>(node));
}

void TypeEvaluator::visit(PT::ArrowMemberExpr *node)
{
  node->car()->accept(this);
//   my_type.dereference();
//   my_type.set_member(PTree::nth<2>(node));
}

void TypeEvaluator::visit(PT::ParenExpr *node)
{
  PT::Node *body = PT::nth<1>(node);
//   if (!body) my_type.set("v");
//   else body->accept(this);
}

