//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/PTree/Display.hh>
#include <Synopsis/SymbolTable/Display.hh>
#include <Synopsis/SymbolLookup.hh>
#include <Synopsis/TypeAnalysis/ConstEvaluator.hh>
#include <Synopsis/Trace.hh>
#include <sstream>
#include <iomanip>

#undef TRUE
#undef FALSE

using namespace Synopsis;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;
using namespace TypeAnalysis;

namespace
{
const std::string TRUE = "true";
const std::string FALSE = "false";

//. FIXME: this is only a quick hack that serves
//.        as a proof of concept to allow const expressions
//.        to include sizeof() expressions.
//.        An elaborate system will probably use a Visitor
//.        to calculate the size of compound types
//.
//. returns a negative value in case it couldn't determine the size
long size_of_builtin_type(PT::Encoding::iterator e)
{
  long size = -1;
  switch (*e)
  {
    case 'b': size = 1; break;
    case 'c': size = 1; break;
    case 'w': size = 2; break;
    case 's': size = 2; break;
    case 'i': size = 4; break;
    case 'l': size = 4; break;
    case 'j': size = 8; break;
    case 'f': size = 4; break;
    case 'd': size = 8; break;
    case 'P': size = 4; break;
    case 'A':
    {
      std::string s;
      while (*++e != '_') s.push_back(*e);
      std::istringstream iss(s);
      iss >> size;
      size *= size_of_builtin_type(++e);
      break;
    }
    default: return -1;
  }
  return size;
}

}

bool ConstEvaluator::evaluate(PT::Node const *node, long &value)
{
  const_cast<PT::Node *>(node)->accept(this);
  if (my_valid)
  {
    value = my_value;
    return true;
  }
  else return false;
}

void ConstEvaluator::visit(PT::Literal *node)
{
  Trace trace("ConstEvaluator::visit(Literal)", Trace::TYPEANALYSIS);
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
      else if (TRUE.compare(0, 4, node->position(), node->length()) == 0)
	my_value = 1;
      else if (FALSE.compare(0, 5, node->position(), node->length()) == 0)
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

void ConstEvaluator::visit(PT::Identifier *node)
{
  Trace trace("ConstEvaluator::visit(Identifier)", Trace::TYPEANALYSIS);
  try
  {
    PT::Encoding name(node);
    ST::SymbolSet symbols = my_scope->unqualified_lookup(name, ST::Scope::DEFAULT);
    ST::ConstName const *const_ = 0;
    if (symbols.size() == 1) 
      const_ = dynamic_cast<ST::ConstName const *>(*symbols.begin());
    if (!const_ || !const_->defined()) my_valid = false;
    else
    {
      my_value = const_->value();
      my_valid = true;
    }
  }
  catch (const ST::TypeError &e)
  {
    std::cerr << "Error in ConstName lookup: type was " << e.type << std::endl;
  }
}

void ConstEvaluator::visit(PT::Name *node)
{
  Trace trace("ConstEvaluator::visit(Name)", Trace::TYPEANALYSIS);
  try
  {
    PT::Encoding name = node->encoded_name();
    ST::SymbolSet symbols = lookup(name, my_scope, ST::Scope::DECLARATION);
    ST::ConstName const *const_ = 0;
    if (symbols.size() == 1)
      const_ = dynamic_cast<ST::ConstName const *>(*symbols.begin());
    if (!const_ || !const_->defined()) my_valid = false;
    else
    {
      my_value = const_->value();
      my_valid = true;
    }
  }
  catch (const ST::TypeError &e)
  {
    std::cerr << "Error in ConstName lookup: type was " << e.type << std::endl;
  }
}

void ConstEvaluator::visit(PT::FstyleCastExpr *node)
{
  Trace trace("ConstEvaluator::visit(FstyleCastExpr)", Trace::TYPEANALYSIS);
  my_valid = evaluate(PT::nth<2>(node)->car(), my_value);
}

void ConstEvaluator::visit(PT::InfixExpr *node)
{
  Trace trace("ConstEvaluator::visit(InfixExpr)", Trace::TYPEANALYSIS);
  long left, right;
  if (!evaluate(PT::nth<0>(node), left) ||
      !evaluate(PT::nth<2>(node), right))
    return;

  PT::Node *op = PT::nth<1>(node);
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

void ConstEvaluator::visit(PT::SizeofExpr *node)
{
  Trace trace("ConstEvaluator::visit(SizeofExpr)", Trace::TYPEANALYSIS);
  if (length(node->cdr()) == 3) // '(' typename ')'
  {
    PT::Node *type_decl = PT::nth<1>(node->cdr());
    assert(type_decl && !type_decl->is_atom());
    
    PT::Encoding type = PT::nth<1>(static_cast<PT::List *>(type_decl))->encoded_type();
    // FIXME: TBD
    long size = size_of_builtin_type(type.begin());
    if (size < 0) return;
    else my_value = static_cast<unsigned long>(size);
    my_valid = true;
  }
  else // unary expr
  {
  }
}

void ConstEvaluator::visit(PT::UnaryExpr *node)
{
  Trace trace("ConstEvaluator::visit(UnaryExpr)", Trace::TYPEANALYSIS);
  PT::Node *op = node->car();
  PT::Node *expr = node->cdr()->car();
  assert(op->is_atom() && op->length() <= 2);
  if (!evaluate(expr, my_value)) return;

  if (op->length() == 1)
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
  else if (*op->position() == '+' && op->position()[1] == '+')
    ++my_value;
  else if (*op->position() == '-' && op->position()[1] == '-')
    --my_value;
  else my_valid = false;
}

void ConstEvaluator::visit(PT::CondExpr *node)
{
  Trace trace("ConstEvaluator::visit(CondExpr)", Trace::TYPEANALYSIS);
  long condition;
  if (!evaluate(node->car(), condition)) return;
  if (condition) // interpret as bool
    my_valid = evaluate(PT::tail(node->cdr(), 1)->car(), my_value);
  else
    my_valid = evaluate(PT::tail(node->cdr(), 3)->car(), my_value);
}

void ConstEvaluator::visit(PT::ParenExpr *node)
{
  Trace trace("ConstEvaluator::visit(ParenExpr)", Trace::TYPEANALYSIS);
  PT::Node *body = node->cdr()->car();
  if (!body) my_valid = false;
  else body->accept(this);
}
