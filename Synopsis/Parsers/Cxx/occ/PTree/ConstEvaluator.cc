//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <PTree/ConstEvaluator.hh>
#include <sstream>
#include <iomanip>

using namespace PTree;

bool ConstEvaluator::evaluate(Node *node, long &value)
{
  node->accept(this);
  if (my_valid)
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
//       std::cout << '"' << *node->position() << node->position()[1] << '"' << std::endl;
      if (*node->position() == '0' && 
	  (node->position()[1] == 'x' || node->position()[1] == 'X'))
      {
	iss.setf(std::ios::hex, std::ios::basefield);
	iss >> my_value;
      }
      else
      {
	double value;
	iss >> value;
	my_value = static_cast<long>(value);
      }
      my_valid = true;
      break;
    }
    default:
      break;
  }
}

void ConstEvaluator::visit(Identifier *node)
{
  try
  {
    Encoding name(node->position(), node->length());
    const ConstName *const_ = my_symbols.lookup<ConstName>(name);
    my_value = const_->value();
    my_valid = true;
  }
  catch (const Scope::TypeError &e)
  {
    std::cerr << "Error in ConstName lookup: type was " << e.type << std::endl;
  }
}

void ConstEvaluator::visit(InfixExpr *node)
{
  long left, right;
  if (!evaluate(first(node), left) ||
      !evaluate(third(node), right))
    my_valid = false;

  Node *op = second(node);
  assert(op->is_atom() && op->length() <= 2);
  my_valid = true;
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
}
