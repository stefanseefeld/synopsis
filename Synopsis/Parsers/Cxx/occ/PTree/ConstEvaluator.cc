//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <PTree/ConstEvaluator.hh>
#include <sstream>

using namespace PTree;

bool ConstEvaluator::evaluate(Node *node, unsigned long &value)
{
  node->accept(this);
  if (my_type == Symbol::CONST)
  {
    value = my_value;
    return true;
  }
  else return false;
}

void ConstEvaluator::visit(Literal *node)
{
  std::istringstream iss(std::string(node->position(), node->length()));
  switch (node->type())
  {
    case Token::Constant:
    {
      double value;
      iss >> value;
      my_value = static_cast<unsigned long>(value);
      my_type = Symbol::CONST;
      break;
    }
    default:
      break;
  }
}

void ConstEvaluator::visit(InfixExpr *node)
{
  unsigned long left, right;
  if (!evaluate(first(node), left) ||
      !evaluate(third(node), right))
    my_type = Symbol::NONE;

  Node *op = second(node);
  assert(op->is_atom() && op->length() <= 2);
  my_type = Symbol::CONST;
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
        my_type = Symbol::NONE;
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
    my_type = Symbol::NONE;
}
