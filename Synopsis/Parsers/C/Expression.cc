//
// Copyright (C) 1998 Shaun Flisakowski
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <config.hh>
#include <Expression.hh>
#include <Declaration.hh>
#include "Grammar.hh"
#include <Token.hh>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <stdlib.h>

//#define SHOW_TYPES

/* Print a char, converting chars to escape sequences. */
static
std::ostream&
MetaChar(std::ostream &out, char c, bool inString)
{
  switch (c)
  {
    case '\'':
      if (inString) 
	out << "'";
      else
	out << "\\'";
      break;

    case '"':
      if (inString) 
	out << "\\\"";
      else
	out << "\"";
      break;
      
    case '\0':
      out << "\\0";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\t':
      out << "\\t";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\f':
      out << "\\f";
      break;
    case '\b':
      out << "\\b";
      break;
    case '\v':
      out << "\\v";
      break;
    case '\a':
      out << "\\a";
      break;
    case ESC_VAL:
      out << "\\e";
      break;

    default:
      // Show low and high ascii as octal
      if ((c < ' ') || (c >= 127))
      {
	char octbuf[8];
	sprintf(octbuf, "%03o", (unsigned char) c);
	out << "\\" << octbuf;
      }
      else
	out << c;
      break;
  }

  return out;
}

/* Print a string, converting chars to escape sequences. */
std::ostream&
MetaString(std::ostream &out, const std::string &string)
{
  for(unsigned i=0; i<string.size(); i++)
    MetaChar(out, string[i], true);
  return out;
}

void
PrintAssignOp(std::ostream &out, AssignOp op)
{
  switch (op)
  {
    case AO_Equal:          //  =
      out << "=";
      break;

    case AO_PlusEql:        // +=
      out << "+=";
      break;

    case AO_MinusEql:       // -=
      out << "-=";
      break;

    case AO_MultEql:        // *=
      out << "*=";
      break;

    case AO_DivEql:         // /=
      out << "/=";
      break;

    case AO_ModEql:         // %=
      out << "%=";
      break;

    case AO_ShlEql:         // <<=
      out << "<<=";
      break;

    case AO_ShrEql:         // >>=
      out << ">>=";
      break;

    case AO_BitAndEql:      // &=
      out << "&=";
      break;

    case AO_BitXorEql:      // ^=
      out << "^=";
      break;

    case AO_BitOrEql:       // |=
      out << "|=";
      break;
  }
}

void
PrintRelOp(std::ostream& out, RelOp op)
{
  switch (op)
  {
    case RO_Equal:          // ==
      out << "==";
      break;

    case RO_NotEqual:       // !=
      out << "!=";
      break;

    case RO_Less:           // < 
      out << "<";
      break;

    case RO_LessEql:        // <=
      out << "<=";
      break;

    case RO_Grtr:           // > 
      out << ">";
      break;

    case RO_GrtrEql:        // >=
      out << ">=";
      break;
  }
}

void
PrintUnaryOp(std::ostream& out, UnaryOp op)
{
  switch (op)
  {
    case UO_Plus:           // +
      out << "+";
      break;
      
    case UO_Minus:          // -
      out << "-";
      break;

    case UO_BitNot:         // ~
      out << "~";
      break;

    case UO_Not:            // !
      out << "!";
      break;

    case UO_PreInc:         // ++x
    case UO_PostInc:        // x++
      out << "++";
      break;

    case UO_PreDec:         // --x
    case UO_PostDec:        // x--
      out << "--";
      break;

    case UO_AddrOf:         // &
      out << "&";
      break;

    case UO_Deref:          // *
      out << "*";
      break;
  }
}

void
PrintBinaryOp(std::ostream& out, BinaryOp op)
{
  switch (op)
  {
    case BO_Plus:        // +
      out << "+";
      break;

    case BO_Minus:       // -
      out << "-";
      break;

    case BO_Mult:        // *
      out << "*";
      break;

    case BO_Div:         // /
      out << "/";
      break;

    case BO_Mod:         // %
      out << "%";
      break;

    case BO_Shl:         // <<
      out << "<<";
      break;

    case BO_Shr:         // >>
      out << ">>";
      break;

    case BO_BitAnd:      // &
      out << "&";
      break;

    case BO_BitXor:      // ^
      out << "^";
      break;

    case BO_BitOr:       // |
      out << "|";
      break;

    case BO_And:         // &&
      out << "&&";
      break;

    case BO_Or:          // ||
      out << "||";
      break;

    case BO_Comma:       // x,y
      out << ",";
      break;

    case BO_Member:      // x.y
      out << ".";
      break;

    case BO_PtrMember:   // x->y
      out << "->";
      break;

    default:
      //  case BO_Index        // x[y]
    case BO_Assign:      // An AssignExpr
    case BO_Rel:         // A RelExpr
      break;
  }
}

Expression::Expression(Expression::Type t, const Location &l)
  : location(l),
    etype(t),
    type(0),
    next(0)
{
}

Expression::~Expression()
{
    //delete type;
}

void
Expression::print(std::ostream& out) const
{
  out << __PRETTY_FUNCTION__ << std::endl;
}

Constant::Constant(Type ct, const Location& l)
  : Expression(Expression::Constant, l),
    ctype(ct)
{
}
 
Constant::~Constant()
{
}

IntConstant::IntConstant(long val, bool b, const Location &l)
  : Constant (Constant::Int, l),
    lng(val),
    L(b)
{
  type = new BaseType(BaseType::Int);
}

IntConstant::~IntConstant()
{
}

Expression*
IntConstant::dup0() const
{
  IntConstant *ret = new IntConstant(lng, L, location);
  ret->type = type;    
  return ret;
}

void
IntConstant::print(std::ostream& out) const
{
  out << lng;
  if (L) out << "L";

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

UIntConstant::UIntConstant(ulong val, bool b, const Location &l)
  : Constant (Constant::UInt, l)
{
  ulng = val;
  L = b;
  type = new BaseType(BaseType::Int | BaseType::UnSigned);
}

UIntConstant::~UIntConstant()
{
}

Expression*
UIntConstant::dup0() const
{
  UIntConstant *ret = new UIntConstant(ulng, L, location);
  ret->type = type;    
  return ret;
}

void
UIntConstant::print(std::ostream& out) const
{
  out << ulng;
  if (L) out << "L";

#ifdef    SHOW_TYPES
  if (type != NULL)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

FloatConstant::FloatConstant(const std::string &val, const Location &l)
  : Constant (Constant::Float, l),
    value(val)
{
  type = new BaseType(BaseType::Float);
}

FloatConstant::~FloatConstant()
{
}

Expression*
FloatConstant::dup0() const
{
  FloatConstant *ret = new FloatConstant(value, location);
  ret->type = type;
  return ret;
}

void
FloatConstant::print(std::ostream& out) const
{
  out << value;
  
#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

CharConstant::CharConstant(char chr, const Location& l,
			   bool isWide /* =false */ )
  :Constant(Constant::Char, l),
   ch(chr),
   wide(isWide)
{
  type = new BaseType(BaseType::Char);
}

CharConstant::~CharConstant()
{
}

Expression*
CharConstant::dup0() const
{
  CharConstant *ret = new CharConstant(ch,location,wide);
  ret->type = type;
  return ret;
}

void
CharConstant::print(std::ostream& out) const
{
  if (wide) out << 'L';

  out << "'";
  MetaChar(out,ch,false);
  out << "'";

#ifdef    SHOW_TYPES
  if (type != 0)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

StringConstant::StringConstant(const std::string &str,
                               const Location &l,
			       bool isWide /* =false */ )
  : Constant(Constant::String, l), buff(str),
    wide(isWide)
{
  // Or should this be const char*?
  PtrType *ptrType = new PtrType(TypeQual::None);
  type = ptrType;
  ptrType->subType = new BaseType(BaseType::Char);
}

StringConstant::~StringConstant()
{
}

int
StringConstant::length() const
{
  return buff.size();
}

Expression*
StringConstant::dup0() const
{
  StringConstant *ret = new StringConstant(buff,location,wide);
  ret->type = type;
  return ret;
}

void
StringConstant::print(std::ostream& out) const
{
  if (wide) out << 'L';

  out << '"';
  MetaString(out,buff);
  out << '"';

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

ArrayConstant::ArrayConstant(const Location& l)
  : Constant(Constant::Array, l)
{
}

ArrayConstant::~ArrayConstant()
{
  for (ExprVector::iterator i = items.begin(); i != items.end(); ++i)
    delete *i;
}

void
ArrayConstant::addElement(Expression *expr)
{
  items.push_back(expr);
}

Expression*
ArrayConstant::dup0() const
{
  ArrayConstant *ret = new ArrayConstant(location);
  for (ExprVector::const_iterator i = items.begin(); i != items.end(); ++i)
    ret->addElement((*i)->dup());
  ret->type = type;
  return ret;
}

void
ArrayConstant::print(std::ostream& out) const
{
  out <<  "{ ";

  if (!items.empty())
  {
    ExprVector::const_iterator i = items.begin();
    (*i)->print(out);
    for (++i; i != items.end(); ++i)
    {
      out << ", ";
      (*i)->print(out);
    }
  }

  out <<  " }";

#ifdef    SHOW_TYPES
  if (type != 0)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

void
ArrayConstant::findExpr(fnExprCallback cb)
{
  (cb)(this);
  for (ExprVector::const_iterator i = items.begin(); i != items.end(); ++i)
    (*i)->findExpr(cb);
}

EnumConstant::EnumConstant(Symbol *nme, Expression *val,
                           const Location &l)
  : Constant(Constant::Enum, l),
    name(nme),
    value(val)
{
}

EnumConstant::~EnumConstant()
{
  delete value;
  delete name;
}

Expression*
EnumConstant::dup0() const
{
  EnumConstant *ret = new EnumConstant(name->dup(),value->dup(),location);
  ret->type = type;
  return ret;
}

void
EnumConstant::print(std::ostream& out) const
{
  out << *name;

  if (value)
  {
    out << " = " << *value;
  }

#ifdef    SHOW_TYPES
  if (type != 0)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

Variable::Variable(Symbol *varname, const Location& l)
  : Expression(Expression::Variable, l),
    name(varname)
{
}

Variable::~Variable()
{
  delete name;
}

Expression*
Variable::dup0() const
{
  Variable *ret = new Variable(name->dup(), location);
  ret->type = type;
  return ret;
}

void
Variable::print(std::ostream& out) const
{
  out << *name;

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

FunctionCall::FunctionCall(Expression *func, const Location &l)
  : Expression(Expression::FunctionCall, l),
    function(func)
{
}

FunctionCall::~FunctionCall()
{
  delete function;
  for (ExprVector::iterator i = args.begin(); i != args.end(); ++i)
    delete *i;
}

void
FunctionCall::addArg(Expression *arg)
{
  args.push_back(arg);
}

void
FunctionCall::addArgs(Expression *args)
{
  Expression *arg = args;
  while (args)
  {
    args = args->next;
    arg->next = 0;
    addArg(arg);
    arg = args; 
  }
}

Expression*
FunctionCall::dup0() const
{
  FunctionCall *ret = new FunctionCall(function->dup(), location);
  for (ExprVector::const_iterator i = args.begin(); i != args.end(); ++i)
    ret->addArg((*i)->dup());    
  return ret;
}

void
FunctionCall::print(std::ostream& out) const
{
  if (function->precedence() < precedence())
  {
    out << "(";
    function->print(out);
    out << ")";
  }
  else function->print(out);

  out << "(";

  if (!args.empty())
  {
    ExprVector::const_iterator i = args.begin();

    if (((*i)->etype == Expression::BinaryExpr)
	&& (((::BinaryExpr*)(*i))->binary_op == BO_Comma))
        {
	  out << "(";
	  (*i)->print(out);
	  out << ")";
        }
        else (*i)->print(out);

        for (++i; i != args.end(); ++i)
        {
	  out << ",";

	  if (((*i)->etype == Expression::BinaryExpr)
	      && (((::BinaryExpr*)(*i))->binary_op == BO_Comma))
          {
	    out << "(";
	    (*i)->print(out);
	    out << ")";
	  }
	  else (*i)->print(out);
        }
  }

  out << ")";

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

void
FunctionCall::findExpr(fnExprCallback cb)
{
  (cb)(this);
  function->findExpr(cb);
  for (ExprVector::iterator i = args.begin(); i != args.end(); ++i)
    (*i)->findExpr(cb);
}

UnaryExpr::UnaryExpr(UnaryOp op, Expression *expr, const Location &l)
  : Expression(Expression::UnaryExpr, l),
    unary_op(op),
    operand(expr)
{
}

UnaryExpr::~UnaryExpr()
{
    delete operand;
}

Expression*
UnaryExpr::dup0() const
{
  UnaryExpr *ret = new UnaryExpr(unary_op, operand->dup(), location);
  ret->type = type;
  return ret;
}

void
UnaryExpr::print(std::ostream& out) const
{
  switch (unary_op)
  {
    default:
      PrintUnaryOp(out, unary_op);

      if (operand->precedence() < precedence())
	out << "(" << *operand << ")";
      else
	out << *operand;
      break;

    case UO_Minus:
      PrintUnaryOp(out, unary_op);
      if ( (operand->precedence() < precedence())
	   || ((operand->etype == Expression::UnaryExpr)
	       && (((UnaryExpr*) operand)->unary_op == UO_Minus)) )
	out << "(" << *operand << ")";
      else out << *operand;
      break;

    case UO_PostInc:
    case UO_PostDec:
      if (operand->precedence() < precedence())
	out << "(" << *operand << ")";
      else out << *operand;

      PrintUnaryOp(out, unary_op);
      break;
    }

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
int
UnaryExpr::precedence() const
{
  switch (unary_op)
  {
    case UO_Plus:           // +
    case UO_Minus:          // -
    case UO_BitNot:         // ~
    case UO_Not:            // !
 
    case UO_PreInc:         // ++x
    case UO_PreDec:         // --x

    case UO_AddrOf:         // &
    case UO_Deref:          // *
      return 15;

    case UO_PostInc:        // x++
    case UO_PostDec:        // x--
      return 16;
  }

  /* Not reached */
  return 16;
}

void
UnaryExpr::findExpr(fnExprCallback cb)
{
  (cb)(this);
  operand->findExpr(cb);
}

BinaryExpr::BinaryExpr(BinaryOp op, Expression *lExpr, Expression *rExpr,
		       const Location &l)
  : Expression(Expression::BinaryExpr, l),
    binary_op(op),
    left(lExpr),
    right(rExpr)
{
}

BinaryExpr::~BinaryExpr()
{
    delete right;
    delete left;
}

Expression*
BinaryExpr::dup0() const
{
  BinaryExpr *ret = new BinaryExpr(binary_op, left->dup(), right->dup(), location);
  ret->type = type;
    
  return ret;
}

void
BinaryExpr::print(std::ostream& out) const
{
  assert(left);

  if (left->precedence() < precedence())
    out << "(" << *left << ")";
  else out << *left;

  bool useSpace = !((binary_op == BO_Member) || (binary_op == BO_PtrMember));

  if (useSpace) out << " ";

  PrintBinaryOp(out, binary_op);

  if (useSpace) out << " ";

  assert(right);

  if ((right->precedence() < precedence())
      || ((right->precedence() == precedence())
	  && (right->etype != Expression::Variable)))
  {
    out << "(" << *right << ")";
  }
  else out << *right;

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

int
BinaryExpr::precedence() const
{
  switch (binary_op)
  {
    case BO_Plus:        // +
    case BO_Minus:       // -
      return 12;

    case BO_Mult:        // *
    case BO_Div:         // /
    case BO_Mod:         // %
      return 13;

    case BO_Shl:         // <<
    case BO_Shr:         // >>
      return 11;

    case BO_BitAnd:      // &
      return 8;

    case BO_BitXor:      // ^
      return 7;

    case BO_BitOr:       // |
      return 6;

    case BO_And:         // &&
      return 5;

    case BO_Or:          // ||
      return 4;

    case BO_Comma:       // x,y
      return 1;

    case BO_Member:      // x.y
    case BO_PtrMember:   // x->y
      return 16;

    case BO_Assign:      // An AssignExpr
    case BO_Rel:         // A RelExpr
      break;
  }
  return 1;
}

void
BinaryExpr::findExpr(fnExprCallback cb)
{
  (cb)(this);

  left->findExpr(cb);
  right->findExpr(cb);
}

TrinaryExpr::TrinaryExpr(Expression *cExpr,
			 Expression *tExpr, Expression *fExpr,
			 const Location& l )
  : Expression(Expression::TrinaryExpr, l),
    condExpr(cExpr),
    trueExpr(tExpr),
    falseExpr(fExpr)
{
}

TrinaryExpr::~TrinaryExpr()
{
    delete falseExpr;
    delete trueExpr;
    delete condExpr;
}

Expression*
TrinaryExpr::dup0() const
{
  TrinaryExpr *ret = new TrinaryExpr(condExpr->dup(),
				     trueExpr->dup(),
				     falseExpr->dup(), location);
  ret->type = type;
  return ret;
}

void
TrinaryExpr::print(std::ostream& out) const
{
  out << "(" << *condExpr << ")";

  out << " ? ";
  out << "(" << *trueExpr << ")";
    
  out << " : ";
  out << "(" << *falseExpr << ")";

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

void
TrinaryExpr::findExpr(fnExprCallback cb)
{
  (cb)(this);

  condExpr->findExpr(cb);
  trueExpr->findExpr(cb);
  falseExpr->findExpr(cb);
}

AssignExpr::AssignExpr(AssignOp op, Expression *lExpr, Expression *rExpr,
		       const Location &l)
  : BinaryExpr(BO_Assign, lExpr, rExpr, l),
    assign_op(op)
{
    type = lExpr->type;
}

AssignExpr::~AssignExpr()
{
}

Expression*
AssignExpr::dup0() const
{
  AssignExpr *ret = new AssignExpr(assign_op, left->dup(), right->dup(), location);
  ret->type = type;    
  return ret;
}

void
AssignExpr::print(std::ostream& out) const
{
  if (left->precedence() < precedence())
    out << "(" << *left << ")";
  else out << *left;

  out << " ";
  PrintAssignOp(out, assign_op);
  out << " ";

  if (right->precedence() < precedence())
    out << "(" << *right << ")";
  else out << *right;

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

RelExpr::RelExpr(RelOp op, Expression *lExpr, Expression *rExpr,
		 const Location& l )
  : BinaryExpr(BO_Rel, lExpr, rExpr, l),
    relational_op(op)
{
    type = new BaseType(BaseType::Int);
}

RelExpr::~RelExpr()
{
}

Expression*
RelExpr::dup0() const
{
  RelExpr *ret = new RelExpr(relational_op,
			     left->dup(), right->dup(), location);
  ret->type = type;
  return ret;
}

void
RelExpr::print(std::ostream& out) const
{
  if (left->precedence() <= precedence())
    out << "(" << *left << ")";
  else out << *left;

  out << " ";
  PrintRelOp(out, relational_op);
  out << " ";

  if (right->precedence() <= precedence())
    out << "(" << *right << ")";
  else out << *right;

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

int
RelExpr::precedence() const
{
  switch (relational_op)
  {
    case RO_Equal:
    case RO_NotEqual:
      return 9;

    default:
    case RO_Less:
    case RO_LessEql:
    case RO_Grtr:
    case RO_GrtrEql:
      return 10;
  }
}

CastExpr::CastExpr(::Type *typeExpr, Expression *operand,
                   const Location &l)
  : Expression(Expression::CastExpr, l),
    castTo(typeExpr),
    expr(operand)
{
  type = typeExpr;
}

CastExpr::~CastExpr()
{
    //delete castTo;
    delete expr;
}

Expression*
CastExpr::dup0() const
{
  CastExpr *ret = new CastExpr(castTo, expr->dup(), location);
  ret->type = type;
  return ret;
}

void
CastExpr::print(std::ostream& out) const
{
  out << "(";
  castTo->printType(out, 0, true, 0); 
  out << ") ";

  out << "(";
  expr->print(out);
  out << ")";

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

void
CastExpr::findExpr(fnExprCallback cb)
{
  (cb)(this);
  expr->findExpr(cb);
}

SizeofExpr::SizeofExpr(Expression *operand, const Location &l)
  : Expression(Expression::SizeofExpr, l),
    sizeofType(0),
    expr(operand)
{
  type = new BaseType(BaseType::UnSigned|BaseType::Long);
}

SizeofExpr::SizeofExpr(::Type *operand, const Location &l)
  : Expression(Expression::SizeofExpr, l),
    sizeofType(operand),
    expr(0)
{
  type = new BaseType(BaseType::UnSigned|BaseType::Long);
}

SizeofExpr::~SizeofExpr()
{
  // delete sizeofType;
  delete expr;
}

Expression*
SizeofExpr::dup0() const
{
  SizeofExpr *ret;
    
  if (sizeofType) ret = new SizeofExpr(sizeofType, location);
  else ret = new SizeofExpr(expr->dup(), location);
  ret->type = type;
  return ret;
}

void
SizeofExpr::print(std::ostream& out) const
{
  out << "sizeof(";
  if (sizeofType) sizeofType->printType(out, 0, true, 0); 
  else expr->print(out);
  out << ") ";

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

void
SizeofExpr::findExpr(fnExprCallback cb)
{
  (cb)(this);
  if (expr) expr->findExpr(cb);
}

IndexExpr::IndexExpr(Expression *a, Expression *sub,
		     const Location &l)
  : Expression(Expression::IndexExpr, l),
    array(a),
    _subscript(sub)
{
}

IndexExpr::~IndexExpr()
{
  delete _subscript;
  delete array;
}

Expression*
IndexExpr::dup0() const
{
  IndexExpr *ret = new IndexExpr(array->dup(),_subscript->dup(), location);
  ret->type = type;
  return ret;
}

void
IndexExpr::print(std::ostream& out) const
{
  if (array->precedence() < precedence())
  {
    out << "(";
    array->print(out);
    out << ")";
  }
  else array->print(out);

  out << "[";
  _subscript->print(out);
  out << "]";

#ifdef    SHOW_TYPES
  if (type)
  {
    out << "/* ";
    type->printType(out, 0, true, 0);
    out << " */";
  }
#endif
}

void
IndexExpr::findExpr(fnExprCallback cb)
{
  (cb)(this);
  array->findExpr(cb);
  _subscript->findExpr(cb);
}

std::ostream&
operator<< (std::ostream& out, const Expression& expr)
{
  expr.print(out);
  return out;
}

Expression*
ReverseList(Expression *exprs)
{
  Expression *head = 0;
  while (exprs)
  {
    Expression *exp = exprs;
    exprs = exprs->next;
    exp->next = head;
    head = exp;
  }
  return head; 
}

#define    SHOW(X)    case X: return #X
char*
nameOfExpressionType(Expression::Type type)
{
  switch (type)
  {
    default:
      return "Unknown ExpressionType";

    SHOW(Expression::VoidExpr);

    SHOW(Expression::Constant);
    SHOW(Expression::Variable);
    SHOW(Expression::FunctionCall);
    
    SHOW(Expression::AssignExpr);
    SHOW(Expression::RelExpr);
    
    SHOW(Expression::UnaryExpr);
    SHOW(Expression::BinaryExpr);
    SHOW(Expression::TrinaryExpr);
    
    SHOW(Expression::CastExpr);
    SHOW(Expression::IndexExpr);
  }
}

char*
nameOfConstantType(Constant::Type type)
{
  switch (type)
  {
    default:
      return "Unknown ConstantType";

    SHOW(Constant::Char);
    SHOW(Constant::Int);
    SHOW(Constant::Float);

    SHOW(Constant::String);
    SHOW(Constant::Array);
  }
}

char*
nameOfAssignOp(AssignOp op)
{
  switch (op)
  {
    default:
      return "Unknown AssignOp";

    SHOW(AO_Equal);          //  =

    SHOW(AO_PlusEql);        // +=
    SHOW(AO_MinusEql);       // -=
    SHOW(AO_MultEql);        // *=
    SHOW(AO_DivEql);         // /=
    SHOW(AO_ModEql);         // %=
    
    SHOW(AO_ShlEql);         // <<=
    SHOW(AO_ShrEql);         // >>=
    
    SHOW(AO_BitAndEql);      // &=
    SHOW(AO_BitXorEql);      // ^=
    SHOW(AO_BitOrEql);       // |=
  }
}

char*
nameOfRelOp(RelOp op)
{
  switch (op)
  {
    default:
      return "Unknown RelOp";

    SHOW(RO_Equal);          // ==
    SHOW(RO_NotEqual);       // !=
    
    SHOW(RO_Less);           // < 
    SHOW(RO_LessEql);        // <=
    SHOW(RO_Grtr);           // > 
    SHOW(RO_GrtrEql);        // >=
  }
}

char*
nameOfUnaryOp(UnaryOp op)
{
  switch (op)
  {
    default:
      return "Unknown UnaryOp";

    SHOW(UO_Plus);           // +
    SHOW(UO_Minus);          // -
    SHOW(UO_BitNot);         // ~
    SHOW(UO_Not);            // !
    
    SHOW(UO_PreInc);         // ++x
    SHOW(UO_PreDec);         // --x
    SHOW(UO_PostInc);        // x++
    SHOW(UO_PostDec);        // x--
    
    SHOW(UO_AddrOf);         // &
    SHOW(UO_Deref);          // *
  }
}

char*
nameOfBinaryOp(BinaryOp op)
{
  switch (op)
  {
    default:
      return "Unknown BinaryOp";

    SHOW(BO_Plus);        // +
    SHOW(BO_Minus);       // -
    SHOW(BO_Mult);        // *
    SHOW(BO_Div);         // /
    SHOW(BO_Mod);         // %
    
    SHOW(BO_Shl);         // <<
    SHOW(BO_Shr);         // >>
    SHOW(BO_BitAnd);      // &
    SHOW(BO_BitXor);      // ^
    SHOW(BO_BitOr);       // |
    
    SHOW(BO_And);         // &&
    SHOW(BO_Or);          // ||
    
    SHOW(BO_Comma);       // x,y
    
    SHOW(BO_Member);      // x.y
    SHOW(BO_PtrMember);   // x->y
    
    SHOW(BO_Assign);      // An AssignExpr
    SHOW(BO_Rel);         // A RelExpr
  }
}

#undef SHOW
