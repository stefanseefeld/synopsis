//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Declaration.hh>
#include <Statement.hh>
#include <File.hh>
#include <Expression.hh>
#include <Symbol.hh>
#include <Printer.hh>

namespace
{

std::string extract_symbol(const Symbol *s) { return s ? s->name : "";}

std::string meta_char(char c, bool in)
{
  switch (c)
  {
    case '\'': return in ? "'" : "\\'";
    case '"': return in ? "\\\"" : "\"";
    case '\0': return "\\0";
    case '\\': return "\\\\";
    case '\n': return "\\n";
    case '\t': return "\\t";
    case '\r': return "\\r";
    case '\f': return "\\f";
    case '\b': return "\\b";
    case '\v': return "\\v";
    case '\a': return "\\a";
    case ESC_VAL: return "\\e";
    default:
      // Show low and high ascii as octal
      if ((c < ' ') || (c >= 127))
      {
	char octbuf[8];
	sprintf(octbuf, "%03o", (unsigned char) c);
	return std::string("\\") + octbuf;
      }
      else return std::string(1, c);
  }
}

std::string meta_string(const std::string &str)
{
  std::string retn;
  for(std::string::const_iterator c = str.begin(); c != str.end(); ++c)
    retn += meta_char(*c, true);
  return retn;
}

std::string rel_op(RelOp op)
{
  switch (op)
  {
    case RO_Equal: return "==";
    case RO_NotEqual: return "!=";
    case RO_Less: return "<";
    case RO_LessEql: return "<=";
    case RO_Grtr: return ">";
    case RO_GrtrEql: return ">=";
  }
}

std::string binary_op(BinaryOp op)
{
  switch (op)
  {
    case BO_Plus: return "+";
    case BO_Minus: return "-";
    case BO_Mult: return "*";
    case BO_Div: return "/";
    case BO_Mod: return "%";
    case BO_Shl: return "<<";
    case BO_Shr: return ">>";
    case BO_BitAnd: return "&";
    case BO_BitXor: return "^";
    case BO_BitOr: return "|";
    case BO_And: return "&&";
    case BO_Or: return "||";
    case BO_Comma: return ",";
    case BO_Member: return ".";
    case BO_PtrMember: return "->";
    default:
      //  case BO_Index        // x[y]
    case BO_Assign:
    case BO_Rel:
      return "";
  }
}

std::string unary_op(UnaryOp op)
{
  switch (op)
  {
    case UO_Plus: return "+";
    case UO_Minus: return "-";
    case UO_BitNot: return "~";
    case UO_Not: return "!";
    case UO_PreInc:
    case UO_PostInc: return "++";
    case UO_PreDec:
    case UO_PostDec: return "--";
    case UO_AddrOf: return "&";
    case UO_Deref: return "*";
  }
}

std::string assign_op(AssignOp op)
{
  switch (op)
  {
    case AO_Equal: return "=";
    case AO_PlusEql: return "+=";
    case AO_MinusEql: return "-=";
    case AO_MultEql: return "*=";
    case AO_DivEql: return "/=";
    case AO_ModEql: return "%=";
    case AO_ShlEql: return "<<=";
    case AO_ShrEql: return ">>=";
    case AO_BitAndEql: return "&=";
    case AO_BitXorEql: return "^=";
    case AO_BitOrEql: return "|=";
  }
}

void print_type(Type *type, Symbol *name, std::ostream &os, bool base, size_t level)
{
  if (base)
  {
    TypeBasePrinter bp(os, level);
    type->accept(&bp);
    if (name) os << ' ';
  }
  TypeBeforePrinter bp(os, level, extract_symbol(name));
  type->accept(&bp);
  TypeAfterPrinter ap(os);
  type->accept(&ap);
}

}

void TypeBasePrinter::traverse_base(BaseType *type)
{
  std::string qualifier = type->qualifier.to_string();
  if (!qualifier.empty()) my_os << qualifier << ' ';
  if (!(type->typemask & (BaseType::Struct |
			  BaseType::Union |
			  BaseType::Enum |
			  BaseType::UserType)))
    my_os << BaseType::to_string(type->typemask) << ' ';

  else if (type->typemask & BaseType::Struct)
  {
    if (type->stDefn) type->stDefn->print(my_os, 0, my_level);
    else
    {
      my_os << "struct ";
      if (type->tag) my_os << *type->tag << ' ';
    }
  }
  else if (type->typemask & BaseType::Union)
  {
    if (type->stDefn) type->stDefn->print(my_os, 0, my_level);
    else
    {
      my_os << "union ";
      if (type->tag) my_os << *type->tag << ' ';
    }
  }
  else if (type->typemask & BaseType::Enum)
  {
    my_os << "enum ";
    if (type->enDefn) type->enDefn->print(my_os, 0, my_level);
    else
    {
      if (type->tag) my_os << *type->tag << ' ';
    }
  }
  else if (type->typemask & BaseType::UserType)
  {
    if (type->typeName) my_os << *type->typeName << ' ';
  }
}

void TypeBasePrinter::traverse_ptr(PtrType *p) { p->subType->accept(this);}
void TypeBasePrinter::traverse_array(ArrayType *a) { a->subType->accept(this);}
void TypeBasePrinter::traverse_bit_field(BitFieldType *b) { b->subType->accept(this);}
void TypeBasePrinter::traverse_function(FunctionType *f) { f->subType->accept(this);}  

void TypeBeforePrinter::traverse_ptr(PtrType *type)
{
  std::string name = my_name;
  my_name = "";

  if (type->subType)
  {
    type->subType->accept(this);
    
    if (!type->subType->isPointer() && !type->subType->isBaseType()) my_os << '(';
    my_os << "*" << type->qualifier.to_string();
  }
  
  my_os << name;
}

void TypeBeforePrinter::traverse_array(ArrayType *type)
{
  if (type->subType) type->subType->accept(this);
}

void TypeBeforePrinter::traverse_bit_field(BitFieldType *type)
{
  std::string name = my_name;
  my_name = "";

  if (type->subType) type->subType->accept(this);
  my_os << name << ':';
  if (type->size) type->size->print(my_os);
}

void TypeBeforePrinter::traverse_function(FunctionType *type)
{
  if (type->subType) type->subType->accept(this);
  else my_os << my_name;
}

void TypeAfterPrinter::traverse_ptr(PtrType *type)
{
  if (type->subType)
  {
    if (!type->subType->isPointer() && !type->subType->isBaseType())
      my_os << ')';
    type->subType->accept(this);
  }
}

void TypeAfterPrinter::traverse_array(ArrayType *type)
{
  my_os << '[';
  if (type->size) type->size->print(my_os);
  my_os << ']';
  
  if (type->subType) type->subType->accept(this);
}

void TypeAfterPrinter::traverse_function(FunctionType *type)
{
  if (type->KnR_decl)
  {
    my_os << '(';
    if (type->nArgs > 0)
    {
      my_os << *(type->args[0]->name);
      for (size_t j = 1; j < type->nArgs; ++j)
	my_os << ", " << *(type->args[j]->name);
    }
    
    my_os << ")\n";

    for (size_t j = 0; j < type->nArgs; ++j)
    {
      type->args[j]->print(my_os, true);
      my_os << ";\n";
    }
  }
  else
  {
    my_os << '(';
    
    if (type->nArgs > 0)
    {
      type->args[0]->print(my_os, true);
      for (size_t j = 1; j < type->nArgs; ++j)
      {
	my_os << ", ";
	type->args[j]->print(my_os, true);
      }
    }
    
    my_os << ')';
  }
  
  if (type->subType) type->subType->accept(this);
}

// Printer::Printer(std::ostream &os, bool d)
//    : out(os), debug(d), level(0), show_base(true)
// {
//   if (debug) Statement::debug = true;
//   else Statement::debug = false;
// }

void ExpressionPrinter::traverse_int(IntConstant *node)
{
  my_os << node->lng;
  if (node->L) my_os << 'L';
}

void ExpressionPrinter::traverse_uint(UIntConstant *node)
{
  my_os << node->ulng;
  if (node->L) my_os << 'L';
}

void ExpressionPrinter::traverse_float(FloatConstant *node)
{
  my_os << node->value;
}

void ExpressionPrinter::traverse_char(CharConstant *node)
{
  if (node->wide) my_os << 'L';
  my_os << "'" << meta_char(node->ch, false) << "'";
}

void ExpressionPrinter::traverse_string(StringConstant *node)
{
  if (node->wide) my_os << 'L';
  my_os << '"' << meta_string(node->buff) << '"';
}

void ExpressionPrinter::traverse_array(ArrayConstant *node)
{
  my_os <<  "{ ";

  if (!node->items.empty())
  {
    ExprVector::const_iterator i = node->items.begin();
    (*i)->accept(this);
    for (++i; i != node->items.end(); ++i)
    {
      my_os << ", ";
      (*i)->accept(this);
    }
  }

  my_os <<  " }";
}

void ExpressionPrinter::traverse_enum(EnumConstant *node)
{
  my_os << extract_symbol(node->name);
  if (node->value)
  {
    my_os << " = ";
    node->value->accept(this);
  }
}

void ExpressionPrinter::traverse_variable(Variable *node)
{
  my_os << extract_symbol(node->name);
}

void ExpressionPrinter::traverse_call(FunctionCall *node)
{
  if (node->function->precedence() < node->precedence())
  {
    my_os << '(';
    node->function->accept(this);
    my_os << ')';
  }
  else node->function->accept(this);

  my_os << '(';

  if (!node->args.empty())
  {
    ExprVector::const_iterator i = node->args.begin();
    if (((*i)->etype == Expression::BinaryExpr)
	&& ((static_cast<BinaryExpr*>(*i))->binary_op == BO_Comma))
        {
	  my_os << '(';
	  (*i)->accept(this);
	  my_os << ')';
        }
    else
      (*i)->accept(this);

    for (++i; i != node->args.end(); ++i)
    {
      my_os << ',';

      if (((*i)->etype == Expression::BinaryExpr)
	  && ((static_cast<BinaryExpr*>(*i))->binary_op == BO_Comma))
      {
	my_os << '(';
	(*i)->accept(this);
	my_os << ')';
      }
      else (*i)->accept(this);
    }
  }

  my_os << ')';
}

void ExpressionPrinter::traverse_unary(UnaryExpr *node)
{
  node->print(my_os);
}

void ExpressionPrinter::traverse_binary(BinaryExpr *node)
{
  node->print(my_os);
}

void ExpressionPrinter::traverse_trinary(TrinaryExpr *node)
{
  node->print(my_os);
}

void ExpressionPrinter::traverse_assign(AssignExpr *node)
{
  if (node->left->precedence() < node->precedence())
  {
    my_os << '(';
    node->left->accept(this);
    my_os << ')';
  }
  else node->left->accept(this);

  my_os << ' ' << assign_op(node->assign_op) << ' ';

  if (node->right->precedence() < node->precedence())
  {
    my_os << '(';
    node->right->accept(this);
    my_os << ')';
  }
  else node->right->accept(this);
}

void ExpressionPrinter::traverse_rel(RelExpr *node)
{
  if (node->left->precedence() <= node->precedence())
  {
    my_os << '(';
    node->left->accept(this);
    my_os << ')';
  }
  else node->left->accept(this);

  my_os << ' ' << rel_op(node->relational_op) << ' ';

  if (node->right->precedence() <= node->precedence())
  {
    my_os << '(';
    node->right->accept(this);
    my_os << ')';
  }
  else node->right->accept(this);
}

void ExpressionPrinter::traverse_cast(CastExpr *node)
{
  my_os << '(';
  node->castTo->printType(my_os, 0, true, 0); 
  my_os << ") ";

  my_os << '(';
  node->expr->accept(this);
  my_os << ')';
}

void ExpressionPrinter::traverse_sizeof(SizeofExpr *node)
{
  my_os << "sizeof(";
  if (node->sizeofType)
    node->sizeofType->printType(my_os, 0, true, 0); 
  else node->expr->accept(this);
  my_os << ") ";
}

void ExpressionPrinter::traverse_index(IndexExpr *node)
{
  if (node->array->precedence() < node->precedence())
  {
    my_os << '(';
    node->array->accept(this);
    my_os << ')';
  }
  else node->array->accept(this);

  my_os << '[';
  node->_subscript->accept(this);
  my_os << ']';
}

void ExpressionPrinter::print_label(Label *node)
{
  decr_indent();
  indent();
  incr_indent();

  switch (node->type)
  {
    case Label::None:
      assert(0);
      break;

    case Label::Default:
      my_os << "default";
      break;

    case Label::Case:
      assert(node->begin);
      my_os << "case ";
      node->begin->accept(this);
      break;

    case Label::CaseRange:
      assert(node->begin);
      assert(node->end);
      my_os << "case ";
      node->begin->accept(this);
      my_os << " ... ";
      node->end->accept(this);
      break;

    case Label::Goto:
      assert(node->name);
      my_os << extract_symbol(node->name);
      break;
  }
  my_os << ":\n";
}

void ExpressionPrinter::print_decl(Decl *decl, bool show_base)
{
  if (show_base)
  {
    std::string storage = decl->storage.to_string();
    if (!storage.empty()) my_os << storage << ' ';
    // Hack to fix K&R non-declarations
    if (!decl->form) my_os << "int ";
  }

  if (decl->form) print_type(decl->form, decl->name, my_os, show_base, my_level);
  else if (decl->name) my_os << extract_symbol(decl->name);

  if (decl->attrib) my_os << " __attribute__ ((" << decl->attrib->to_string() << "))";

  if (decl->initializer)
  {
    my_os << " = ";
    decl->initializer->print(my_os);
  }
}

void StatementPrinter::traverse_statement(Statement *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end(); ++i)
    ep.print_label(*i);

  indent();

  switch (node->type)
  {
    case Statement::NullStemnt: my_os << ';'; break;
    case Statement::ContinueStemnt: my_os << "continue"; break;
    case Statement::BreakStemnt: my_os << "break"; break;
    default:
      my_os << __PRETTY_FUNCTION__ << std::endl;
      my_os << nameOfStatementType(node->type) << std::endl;
      break;
  }
}

void StatementPrinter::traverse_file_line(FileLineStemnt *node)
{
  if (node->linenumber > 0) my_os << "#line " << node->linenumber;
  else my_os << "#file";  
  my_os << " \"" << node->filename << "\"" << std::endl;
}

void StatementPrinter::traverse_include(InclStemnt *node)
{
  my_os << "#include " << (node->isStandard ? '<' : '"');
  my_os << node->filename << (node->isStandard ? '>' : '"') << std::endl;
}

void StatementPrinter::traverse_end_include(EndInclStemnt *node)
{
}

void StatementPrinter::traverse_expression(ExpressionStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);

  ep.indent();
  assert(node->expression);
  node->expression->accept(&ep);
}

void StatementPrinter::traverse_if(IfStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);

  indent();

  my_os << "if (";
  node->cond->accept(&ep);
  my_os << ")\n";
  block(node->thenBlk);
  
  if (node->elseBlk)
  {
    my_os << std::endl;
    indent();
    my_os << "else\n";
    block(node->elseBlk);
  }
}

void StatementPrinter::traverse_switch(SwitchStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);
  
  indent();
  my_os << "switch (";
  node->cond->accept(&ep);
  my_os << ")\n";
  
  block(node->block);
}

void StatementPrinter::traverse_for(ForStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);

  indent();
  my_os << "for (";
  if (node->init) node->init->accept(&ep);
  my_os << "; ";

  if (node->cond) node->cond->accept(&ep);
  my_os << "; ";

  if (node->incr) node->incr->accept(&ep);
  my_os << ")\n";

  block(node->block);
}

void StatementPrinter::traverse_while(WhileStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);

  indent();
  my_os << "while (";
  node->cond->accept(&ep);
  my_os << ")\n";
  
  block(node->block);
}

void StatementPrinter::traverse_do_while(DoWhileStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);
  
  indent();
  my_os << "do ";
  
  if (!node->block->isBlock())
    my_os << std::endl;
  
  block(node->block);
  if (!node->block->isBlock())
    my_os << std::endl;
  
  indent();
  my_os << "while (";
  node->cond->accept(&ep);
  my_os << ")";
}

void StatementPrinter::traverse_goto(GotoStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);

  indent();
  my_os << "goto " << extract_symbol(node->dest);
}

void StatementPrinter::traverse_return(ReturnStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);
  
   indent();
   my_os << "return";
  
   if (node->result)
   {
     my_os << ' ';
     node->result->accept(&ep);
   }
}

void StatementPrinter::traverse_declaration(DeclStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);
  
  indent();
  if (!node->decls.empty())
  {
    DeclVector::const_iterator i = node->decls.begin();
    
    ep.print_decl(*i, true);
    for (++i; i != node->decls.end(); ++i)
    {
      my_os << ", ";
      ep.print_decl(*i, false);
    }
  }

  my_os << ';';
}

void StatementPrinter::traverse_typedef(TypedefStemnt *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);

  indent();
  
  my_os << "typedef ";
 
  if (!node->decls.empty())
  {
    DeclVector::const_iterator i = node->decls.begin();
    ep.print_decl(*i, true);
    for (++i; i != node->decls.end(); ++i)
    {
      my_os << ", ";
      ep.print_decl(*i, false);
    }
  }
  
  my_os << ';';
}

void StatementPrinter::traverse_block(Block *node)
{
  ExpressionPrinter ep(*this);
  for (LabelVector::const_iterator i = node->labels.begin();
       i != node->labels.end();
       ++i)
    ep.print_label(*i);

  indent();
  my_os << "{\n";

  bool isDecl = (node->head != 0) ? node->head->isDeclaration() : false;
  for (Statement *statement = node->head; statement; statement = statement->next)
  {
    if (isDecl && !statement->isDeclaration())
    {
      isDecl = false;
      my_os << std::endl;
    }

    incr_indent();
    statement->accept(this);
    decr_indent();

    if (statement->needSemicolon()) my_os << ";";
    my_os << std::endl;
  }
  indent();
  my_os << "}\n";
}

void StatementPrinter::traverse_function_definition(FunctionDef *node)
{
  ExpressionPrinter ep(*this);
  ep.print_decl(node->decl, true);
  my_os << std::endl;

  traverse_block(node);
}

void StatementPrinter::print_file(File *unit)
{
  int include = 0;
  for (Statement *statement = unit->my_head; statement; statement = statement->next)
  {
    if (include > 0)
    {
      if (statement->isEndInclude()) include--;
      else if (statement->isInclude()) include++;
    }
    else
    {
      if (statement->isInclude()) include++;
      statement->accept(this);
      my_os << std::endl;
    }
  }
}

void StatementPrinter::block(Statement *node)
{
  if (!node)
  {
    incr_indent();
    indent();
    my_os << ";\n";
    decr_indent();
  } 
  else if (node->isBlock()) node->accept(this);
  else
  {
    incr_indent();
    node->accept(this);
    decr_indent();
    
    if (node->needSemicolon()) my_os << ';';
  }
}
