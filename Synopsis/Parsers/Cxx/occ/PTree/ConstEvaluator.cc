//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <PTree/ConstEvaluator.hh>
#include <sstream>
#include <iomanip>
#include <PTree/Writer.hh>
#include <PTree/Display.hh>

using namespace PTree;

namespace
{
  const std::string TRUE = "true";
  const std::string FALSE = "false";
}

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
      if (*node->position() == '0' && 
	  (node->position()[1] == 'x' || node->position()[1] == 'X'))
      {
	iss.setf(std::ios::hex, std::ios::basefield);
	iss >> my_value;
      }
      else if (node->length() == 4 && TRUE.compare(0, 4, node->position(), 4) == 0)
	my_value = 1;
      else if (node->length() == 5 && FALSE.compare(0, 5, node->position(), 5) == 0)
	my_value = 0;
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
    if (!const_ || !const_->defined()) my_valid = false;
    else
    {
      my_value = const_->value();
      my_valid = true;
    }
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
    return;

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

void ConstEvaluator::visit(UnaryExpr *node)
{
  Node *op = node->car();
  Node *expr = node->cdr()->car();
  assert(op->is_atom() && op->length() == 1);
  if (!evaluate(expr, my_value)) return;

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
}

void ConstEvaluator::visit(CondExpr *node)
{
  long condition;
  if (!evaluate(node->car(), condition)) return;
  if (condition) // interpret as bool
    my_valid = evaluate(PTree::tail(node->cdr(), 1)->car(), my_value);
  else
    my_valid = evaluate(PTree::tail(node->cdr(), 3)->car(), my_value);
}

void ConstEvaluator::visit(ParenExpr *node)
{
  Node *body = node->cdr()->car();
  if (!body) my_valid = false;
  else body->accept(this);
}
