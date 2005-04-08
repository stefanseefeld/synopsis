//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "TypeEvaluator.hh"
#include <Synopsis/PTree/Display.hh>
#include <cassert>

using namespace Synopsis;
using namespace PTree;
using namespace TypeAnalysis;

namespace
{
std::string const TRUE = "true";
std::string const FALSE = "false";

char const *numeric_type(char const *position, size_t length)
{
  // TODO: If there is no explicit specifier for signedness or long,
  //       simply assume 'int'. Implement 2.13.1 [lex.icon]
  if (length > 2 && *position == '0' && 
      (*(position + 1) == 'x' || *(position + 1) == 'X'))
    // hexadecimal literal
    return "Ui";
  else if (length > 1 && *position == '0' && *(position + 1) != '.')
    // octal literal
    return "Ui";
  else if (TRUE.compare(0, 4, position, length) == 0 ||
	   FALSE.compare(0, 5, position, length) == 0)
    return "b";
  char const *c = position;
  for (size_t i = 0; i != length; ++i, ++c)
  {
    if (*c == '.' || *c == 'E' || *c == 'e')
      // floating-point literal
      return (*(position + length - 1) == 'l' || *(position + length - 1) == 'L' ?
	      "d" : "f");
  }
  // integral literal
  if (*(position + length - 1) == 'l' || *(position + length - 1) == 'L')
    return (*(position + length - 2) == 'u' || *(position + length - 2) == 'U' ?
	      "Ul" : "l");
  return (*(position + length - 1) == 'u' || *(position + length - 1) == 'U' ?
	  "Ui" : "i");
}

}

Type const *TypeEvaluator::evaluate(Node const *node)
{
//   my_type.unknown();
  if (node) const_cast<Node *>(node)->accept(this);
  return my_type;
}

void TypeEvaluator::visit(Literal *node)
{
//   switch (node->type())
//   {
//     case Token::CharConst: my_type.set("c"); break;
//     case Token::WideCharConst: my_type.set("w"); break;
//     case Token::StringL: my_type.set("CPc"); break;
//     case Token::WideStringL: my_type.set("CPw"); break;
//     case Token::Constant:
//       my_type.set(numeric_type(node->position(), node->length()));
//       break;
//     default:
//       std::cerr << "unmatched type for literal " 
// 		<< std::string(node->position(), node->length()) 
// 		<< ' ' << node->type() << std::endl;
//       my_type.set("c");
//       break;
//   }
}

void TypeEvaluator::visit(Identifier *node)
{
  Encoding name = Encoding::simple_name(node);
  SymbolLookup::SymbolSet symbols = my_scope->lookup(name);
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

void TypeEvaluator::visit(PTree::This *)
{
  // FIXME: TBD
//   my_type.set(my_env->LookupThis());
}

void TypeEvaluator::visit(PTree::Name *node)
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

void TypeEvaluator::visit(PTree::FstyleCastExpr *node)
{
//   my_type.set(node->encoded_type(), my_scope);
}

void TypeEvaluator::visit(PTree::AssignExpr *node)
{
  PTree::first(node)->accept(this);
}

void TypeEvaluator::visit(CondExpr *node)
{
//   my_type.set("b");
//   type_of(PTree::third(node));
}

void TypeEvaluator::visit(InfixExpr *node)
{
  Type const *lhs = evaluate(first(node));
  Type const *rhs = evaluate(third(node));
  Node *op = second(node);
  assert(op->is_atom() && op->length() <= 2);

  // FIXME: TBD
//   my_type = lhs;

  /*
  if (op->length() == 1)
    switch (*op->position())
    {
      case '+':
	my_value = left + right;
	break;
      case '-':
	my_value = left - right;
	break;
      case '>':
	my_value = left > right;
	break;
      case '<':
	my_value = left < right;
	break;
      case '&':
	my_value = left & right;
	break;
      case '|':
	my_value = left | right;
	break;
      case '^':
	my_value = left ^ right;
	break;
      case '*':
	my_value = left * right;
	break;
      case '/':
	my_value = left / right;
	break;
      case '%':
	my_value = left % right;
	break;
      default:
        my_valid = false;
        break;
    }
  else if (*op->position() == '=' && op->position()[1] == '=')
    my_value = left == right;
  else if (*op->position() == '!' && op->position()[1] == '=')
    my_value = left != right;
  else if (*op->position() == '<' && op->position()[1] == '<')
    my_value = left << right;
  else if (*op->position() == '>' && op->position()[1] == '>')
    my_value = left >> right;
  else if (*op->position() == '&' && op->position()[1] == '&')
    my_value = left && right;
  else if (*op->position() == '|' && op->position()[1] == '|')
    my_value = left || right;
  else
    my_valid = false;
  */
}

void TypeEvaluator::visit(PTree::PmExpr *node)
{
  PTree::third(node)->accept(this);
//   my_type.dereference();
}

void TypeEvaluator::visit(PTree::CastExpr *node) 
{
//   my_type.set(PTree::second(PTree::second(node))->encoded_type(), my_scope);
}

void TypeEvaluator::visit(PTree::UnaryExpr *node)
{
  PTree::second(node)->accept(this);
  PTree::Node *op = PTree::first(node);
//   if(*op == '*') my_type.dereference();
//   else if(*op == '&') my_type.reference();
}

void TypeEvaluator::visit(PTree::ThrowExpr *)
{
//   my_type.set_void();
}

void TypeEvaluator::visit(SizeofExpr *)
{
//   my_type.set_int();
}

void TypeEvaluator::visit(PTree::TypeidExpr *)
{
  // FIXME: Should be type (node->third()->second()->encoded_type(), my_scope);
//   my_type.set_int();
}

void TypeEvaluator::visit(PTree::TypeofExpr *) 
{
  // FIXME: Should be type (node->third()->second()->encoded_type(), my_scope);
//   my_type.set_int();
}

void TypeEvaluator::visit(PTree::NewExpr *node)
{
  PTree::Node *p = node;
  PTree::Node *userkey = p->car();
  if(!userkey || !userkey->is_atom()) p = node->cdr(); // user keyword
  if(*p->car() == "::") p = p->cdr();
  PTree::Node *type = PTree::third(p);
//   if(*type->car() == '(')
//     my_type.set(PTree::second(PTree::second(type))->encoded_type(), my_scope);
//   else
//     my_type.set(PTree::second(type)->encoded_type(), my_scope);
//   my_type.reference();
}

void TypeEvaluator::visit(PTree::DeleteExpr *)
{
//   my_type.set_void();
}

void TypeEvaluator::visit(PTree::ArrayExpr *node)
{
  node->car()->accept(this);
//   my_type.dereference();
}

void TypeEvaluator::visit(PTree::FuncallExpr *node)
{
  node->car()->accept(this);
//   if(!my_type.is_function())
//     my_type.dereference(); // maybe a pointer to a function
//   my_type.dereference();
}

void TypeEvaluator::visit(PTree::PostfixExpr *node)
{
  node->car()->accept(this);
}

void TypeEvaluator::visit(PTree::DotMemberExpr *node)
{
  node->car()->accept(this);
//   my_type.set_member(PTree::third(node));
}

void TypeEvaluator::visit(PTree::ArrowMemberExpr *node)
{
  node->car()->accept(this);
//   my_type.dereference();
//   my_type.set_member(PTree::third(node));
}

void TypeEvaluator::visit(ParenExpr *node)
{
  Node *body = second(node);
//   if (!body) my_type.set("v");
//   else body->accept(this);
}

