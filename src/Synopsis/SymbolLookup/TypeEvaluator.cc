//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/SymbolLookup/TypeEvaluator.hh>
#include <cassert>

using namespace Synopsis;
using namespace PTree;
using namespace SymbolLookup;

namespace
{
const std::string TRUE = "true";
const std::string FALSE = "false";
}

Encoding TypeEvaluator::evaluate(Node *node)
{
  return Encoding();
}

void TypeEvaluator::visit(Literal *node)
{
  switch (node->type())
  {
    case Token::Constant:
    default:
      break;
  }
}

void TypeEvaluator::visit(Identifier *node)
{
}

void TypeEvaluator::visit(FstyleCastExpr *node)
{
}

void TypeEvaluator::visit(InfixExpr *node)
{
  Encoding lhs = evaluate(first(node));
  Encoding rhs = evaluate(third(node));

  Node *op = second(node);
  assert(op->is_atom() && op->length() <= 2);
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

void TypeEvaluator::visit(SizeofExpr *node)
{
}

void TypeEvaluator::visit(UnaryExpr *node)
{
  Node *op = node->car();
  Node *expr = node->cdr()->car();
  assert(op->is_atom() && op->length() == 1);
  Encoding type = evaluate(expr);

  /*
  switch (*op->position()) // '*' and '&' do not apply to constant expressions
  {
    case '+': break; 
    case '-':
      my_value = -my_value;
      break;
    case '!': 
      my_value = !my_value;
      break;
    case '~': 
      my_value = ~my_value;
      break;
    default:
      my_valid = false;
  }
  */
}

void TypeEvaluator::visit(CondExpr *node)
{
  my_type = "b";
}

void TypeEvaluator::visit(ParenExpr *node)
{
  Node *body = node->cdr()->car();
  if (!body) my_type = "v";
  else body->accept(this);
}
