/* $Id: Statement.cc,v 1.6 2003/12/30 07:33:06 stefan Exp $
 *
 * Copyright (C) 1998 Shaun Flisakowski
 * Copyright (C) 2003 Stefan Seefeld
 * All rights reserved.
 * Licensed to the public under the terms of the GNU LGPL (>= 2),
 * see the file COPYING for details.
 */

#include <Statement.hh>
#include <Symbol.hh>
#include <Declaration.hh>
#include <Grammar.hh>

#include <cstring>
#include <cassert>

bool Statement::debug = false;
bool Statement::verbose = false;


//#define PRINT_LOCATION

//  indent - 2 spaces per level.
void
indent(std::ostream& out, int level)
{
  if (level > 0)
    for (int j=level; j > 0; j--) out << "  ";
}

void
printNull(std::ostream& out, int level)
{
  indent(out,level);
  out << ";\n";
}

void
printBlock(std::ostream& out, int level, Statement *block)
{
  if (block == 0) printNull(out,level+1);
  else if (block->isBlock()) block->print(out,level);
  else
  {
    block->print(out,level+1);
    if (block->needSemicolon()) out << ";";
  }
}

Statement*
ReverseList(Statement *sList)
{
  Statement *head = 0;

  while (sList)
  {
    Statement *ste = sList;
    sList = sList->next;
    ste->next = head;
    head = ste;
  }
  return head; 
}

Label::Label(Type t)
  : type(t),
    begin(0),
    end(0)
{
}   

Label::Label(Expression *expr)
  : type(Case),
    begin(expr),
    end(0)
{
}   

Label::Label(Expression *b, Expression *e)
  : type(CaseRange),
    begin(b),
    end(e)
{
}

Label::Label(Symbol *sym)
  : type(Goto),
    name(sym),
    end(0)
{
}   

Label::~Label()
{
  switch (type)
  {
    case Case:
    case CaseRange:
      delete begin;
      delete end;
      break;
    case Goto:
      delete name;
      break;
    default:
      break;
  }
}

Label*
Label::dup() const
{
  Label *ret = this ? dup0() : 0;
  return ret;
}

Label*
Label::dup0() const
{
    Label *ret = new Label(type);

    switch (type)
    {
      default:
      case None:
      case Default:
        break;

      case CaseRange:
        ret->end = end->dup();
      case Case:
        ret->begin = begin->dup();
        break;

      case Goto:
        ret->name = name->dup();
        break;
    }
    return ret;
}

void
Label::print(std::ostream& out, int level) const
{
    indent(out,level-1);

    switch (type)
    {
      case None:
	assert(0);
	break;

      case Default:
	out << "default";
	break;

      case Case:
	assert(begin);
	out << "case " << *begin;
	break;

      case CaseRange:
	assert(begin);
	assert(end);
	out << "case " << *begin << " ... " << *end;
	break;

      case Goto:
	assert(name);
	out << *name;
	break;
    }
    out << ":\n";
}

void
Label::findExpr(fnExprCallback cb)
{
  switch (type)
  {
    default:
      break;

    case CaseRange:
      end->findExpr(cb);
    case Case:
      begin->findExpr(cb);
      break;
  }
}

Statement::Statement(Type t, const Location &l)
  : type(t),
    location(l),
    next(0)
{
}

Statement::~Statement()
{
  for (LabelVector::iterator i = labels.begin(); i != labels.end(); ++i)
    delete *i;
}

void
Statement::addLabel(Label *lbl)
{
  labels.push_back(lbl);

  // Hook the label's symtable entry back to this statement.
  if (lbl->type == Label::Goto)
  {
    if (lbl->name->entry != 0)
      lbl->name->entry->u2LabelPosition = this;
  }
}

void
Statement::addHeadLabel(Label *lbl)
{
  labels.insert(labels.begin(), lbl);

  // Hook the label's symtable entry back to this statement.
  if (lbl->type == Label::Goto)
  {
    if (lbl->name->entry != 0)
    {
      lbl->name->entry->u2LabelPosition = this;
      lbl->name->entry->uLabelDef = lbl;
    }
  }
}

Statement*
Statement::dup0() const
{
  Statement *ret = new Statement(type, location);
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
Statement::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* Statement:" ;
    location.printLocation(out) ;
    out << " */";
  }

  for (LabelVector::const_iterator i =labels.begin(); i != labels.end(); ++i)
    (*i)->print(out, level);

  indent(out,level);

  switch (type)
  {
    default:
      out << __PRETTY_FUNCTION__ << std::endl;
      out << nameOfStatementType(type) << std::endl;
      break;

    case NullStemnt:          // The null statement.
      out << ";";
      break;

    case ContinueStemnt:
      out << "continue";
      break;

    case BreakStemnt:
      out << "break";
      break;
  }
}

void
Statement::findExpr(fnExprCallback cb)
{
  for (LabelVector::iterator i =labels.begin(); i != labels.end(); ++i)
    (*i)->findExpr(cb);
}

FileLineStemnt::FileLineStemnt(const std::string &incl, int lino, const Location &l)
  : Statement(Statement::FileLineStemnt, l), filename(incl), linenumber(lino)
{
}

FileLineStemnt::~FileLineStemnt()
{
}

Statement*
FileLineStemnt::dup0() const
{
  FileLineStemnt *ret = new FileLineStemnt(filename, linenumber, location);
    
  for (LabelVector::const_iterator i =labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
FileLineStemnt::print(std::ostream& out, int level) const
{
#ifdef PRINT_LOCATION
  out << "$<FileLineStemnt:" ;
  location.printLocation(out) ;
  out << ">$";
#endif

  if (linenumber > 0)
    out << "#line " << linenumber;
  else
    out << "#file";
      
  out << " \"" << filename << "\"" << std::endl;
}

InclStemnt::InclStemnt(const std::string& incl, const Location& l)
  : Statement(Statement::InclStemnt, l)
{
  static const char  *NrmPath[] = { "./", 0};

  isStandard = false;
  filename = incl;

//   for (int i = 0; StdPath[i]; ++i)
//   {
//     if (strncmp(filename.c_str(), StdPath[i], strlen(StdPath[i])) == 0)
//     {
//       isStandard = true;
//       filename = &(filename.c_str()[strlen(StdPath[i])]);
//     }
//   }

  for (int i = 0; NrmPath[i]; i++)
  {
    if (strncmp(filename.c_str(), NrmPath[i], strlen(NrmPath[i])) == 0)
    {
      filename = &(filename.c_str()[strlen(NrmPath[i])]);
    }
  }
}

InclStemnt::~InclStemnt()
{
}

Statement*
InclStemnt::dup0() const
{
  InclStemnt *ret = new InclStemnt(filename, location);
  ret->isStandard = isStandard;
    
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
InclStemnt::print(std::ostream& out, int level) const
{
#ifdef PRINT_LOCATION
  out << "$<InclStemnt:" ;
  location.printLocation(out) ;
  out << ">$";
#endif

  out << "#include " << (isStandard ? '<' : '"');
  out << filename << (isStandard ? '>' : '"') << std::endl;
}

EndInclStemnt::EndInclStemnt(const Location& l)
  : Statement(Statement::EndInclStemnt, l)
{
}

EndInclStemnt::~EndInclStemnt()
{
}

Statement*
EndInclStemnt::dup0() const
{
  EndInclStemnt *ret = new EndInclStemnt(location);
    
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
EndInclStemnt::print(std::ostream& out, int level) const
{
#ifdef PRINT_LOCATION
  out << "$<EndInclStemnt:" ;
  location.printLocation(out) ;
  out << ">$";
#endif
}

ExpressionStemnt::ExpressionStemnt(Expression *expr, const Location& l)
  : Statement(Statement::ExpressionStemnt, l),
    expression(expr)
{
}

ExpressionStemnt::~ExpressionStemnt()
{
  delete expression;
}

Statement*
ExpressionStemnt::dup0() const
{
  ExpressionStemnt *ret = new ExpressionStemnt(expression->dup(), location);  
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
        
  return ret;
}

void
ExpressionStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* ExpressionStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out, level);

  indent(out,level);
  assert(expression);
  out << *expression;
}

void
ExpressionStemnt::findExpr(fnExprCallback cb)
{
  Statement::findExpr(cb);
  expression->findExpr(cb);
}

IfStemnt::IfStemnt(Expression *c,
		   Statement *t, const Location &l,
		   Statement *e)
  : Statement(Statement::IfStemnt, l),
    cond(c),
    thenBlk(t),
    elseBlk(e)
{
}

IfStemnt::~IfStemnt()
{
  delete elseBlk;
  delete thenBlk;
  delete cond;
}

Statement*
IfStemnt::dup0() const
{
  IfStemnt *ret = new IfStemnt(cond->dup(), thenBlk->dup(),
			       location, elseBlk->dup());
    
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
        
  return ret;
}

void
IfStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* IfStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }

  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out, level);

  indent(out,level);

  out << "if (" << *cond << ")\n";
  printBlock(out,level,thenBlk);

  if (elseBlk)
  {
    out << std::endl;
    indent(out,level);
    out << "else\n";
    printBlock(out,level,elseBlk);
  }
}

void
IfStemnt::findExpr(fnExprCallback cb)
{
  Statement::findExpr(cb);

  cond->findExpr(cb);

  thenBlk->findExpr(cb);

  if (elseBlk) elseBlk->findExpr(cb);
}

void
IfStemnt::find_statement(fnStemntCallback cb)
{
  (cb)(this);

  thenBlk->find_statement(cb);

  if (elseBlk) elseBlk->find_statement(cb);
}

SwitchStemnt::SwitchStemnt(Expression *c, Statement *b,
			   const Location &l)
  : Statement(Statement::SwitchStemnt, l),
    cond(c),
    block(b)
{
}

SwitchStemnt::~SwitchStemnt()
{
  delete block;
  delete cond;
}

Statement*
SwitchStemnt::dup0() const
{
  SwitchStemnt *ret = new SwitchStemnt(cond->dup(),block->dup(), location);
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  
  return ret;
}

void
SwitchStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* SwitchStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }

  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out,level);

  indent(out,level);

  out << "switch (" << *cond << ")\n";

  printBlock(out,level,block);
}

void
SwitchStemnt::findExpr(fnExprCallback cb)
{
  Statement::findExpr(cb);
  cond->findExpr(cb);
  block->findExpr(cb);
}

void
SwitchStemnt::find_statement(fnStemntCallback cb)
{
  (cb)(this);

  block->find_statement(cb);
}

ForStemnt::ForStemnt(Expression *i, Expression *c,
		     Expression *inc, const Location &l,
		     Statement *b)
  : Statement(Statement::ForStemnt, l),
    init(i),
    cond(c),
    incr(inc),
    block(b)
{
}

ForStemnt::~ForStemnt()
{
    delete block;
    delete incr;
    delete cond;
    delete init;
}

Statement*
ForStemnt::dup0() const
{
  ForStemnt *ret = new ForStemnt(init->dup(),cond->dup(),incr->dup(),
				 location, block->dup());
  for (LabelVector::const_iterator i =labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
ForStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* ForStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out,level);

  indent(out,level);

  out << "for (";

  if (init) out << *init;
  out << "; ";

  if (cond) out << *cond;
  out << "; ";

  if (incr) out << *incr;
  out << ")\n";

  printBlock(out,level,block);
}

void
ForStemnt::findExpr(fnExprCallback cb)
{
  Statement::findExpr(cb);

  if (init) init->findExpr(cb);
  if (cond) cond->findExpr(cb);
  if (incr) incr->findExpr(cb);
  if (block) block->findExpr(cb);
}

void
ForStemnt::find_statement(fnStemntCallback cb)
{
  (cb)(this);
  if (block) block->find_statement(cb);
}

WhileStemnt::WhileStemnt(Expression *c, Statement *b, const Location &l)
  : Statement(Statement::WhileStemnt, l),
    cond(c),
    block(b)
{
}

WhileStemnt::~WhileStemnt()
{
  delete block;
  delete cond;
}

Statement*
WhileStemnt::dup0() const
{
  WhileStemnt *ret = new WhileStemnt(cond->dup(),block->dup(), location);
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
WhileStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* WhileStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }

  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out,level);

  indent(out,level);

  out << "while (" << *cond << ")\n";

  printBlock(out,level,block);
}

void
WhileStemnt::findExpr(fnExprCallback cb)
{
  Statement::findExpr(cb);
  cond->findExpr(cb);
  if (block) block->findExpr(cb);
}

void
WhileStemnt::find_statement(fnStemntCallback cb)
{
  (cb)(this);
  if (block) block->find_statement(cb);
}

DoWhileStemnt::DoWhileStemnt(Expression *c, Statement *b,
			     const Location &l)
  : Statement(Statement::DoWhileStemnt, l),
    cond(c),
    block(b)
{
}

DoWhileStemnt::~DoWhileStemnt()
{
  delete block;
  delete cond;
}

Statement*
DoWhileStemnt::dup0() const
{
  DoWhileStemnt *ret = new DoWhileStemnt(cond->dup(),block->dup(), location);
  for (LabelVector::const_iterator i =labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
DoWhileStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* DoWhileStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }

  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out,level);

  indent(out,level);
  out << "do ";

  if (!block->isBlock())
    out << std::endl;

  printBlock(out,level,block);

  if (!block->isBlock())
    out << std::endl;

  indent(out,level);
  out << "while (" << *cond << ")";
}

void
DoWhileStemnt::findExpr(fnExprCallback cb)
{
  Statement::findExpr(cb);
  cond->findExpr(cb);
  if (block) block->findExpr(cb);
}

void
DoWhileStemnt::find_statement(fnStemntCallback cb)
{
  (cb)(this);
  if (block) block->find_statement(cb);
}

GotoStemnt::GotoStemnt(Symbol *d, const Location &l)
  : Statement(Statement::GotoStemnt, l),
    dest(d)
{
}

GotoStemnt::~GotoStemnt()
{
  delete dest;
}

Statement*
GotoStemnt::dup0() const
{
  GotoStemnt *ret = new GotoStemnt(dest->dup(), location);
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
GotoStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* GotoStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out,level);

  indent(out,level);
  out << "goto " << *dest;
}

ReturnStemnt::ReturnStemnt(Expression *r, const Location &l)
  : Statement(Statement::ReturnStemnt, l),
    result(r)
{
}

ReturnStemnt::~ReturnStemnt()
{
  delete result;
}

Statement*
ReturnStemnt::dup0() const
{
  ReturnStemnt *ret = new ReturnStemnt(result->dup(), location);
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
ReturnStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* ReturnStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }
  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out,level);

  indent(out,level);
  out << "return";

  if (result) out << " " << *result;
}

void
ReturnStemnt::findExpr(fnExprCallback cb)
{
  Statement::findExpr(cb);
  if (result) result->findExpr(cb);
}

DeclStemnt::DeclStemnt(const Location &l,
		       Statement::Type t /* =ST_DeclStemnt */)
  : Statement(t, l)
{
}

DeclStemnt::~DeclStemnt()
{
  for (DeclVector::iterator i = decls.begin(); i != decls.end(); ++i)
    delete *i;
}

void
DeclStemnt::addDecl(Decl *decl)
{
  decls.push_back(decl);
}

void
DeclStemnt::addDecls(Decl *decls)
{
  Decl *decl = decls;
  while (decls)
  {
    decls = decls->next;
    decl->next = 0;
    //std::cout << "Decl is: ";
    //decl->print(std::cout,true);
    //std::cout << std::endl;
    addDecl(decl);
    decl = decls;
  }
}

Statement*
DeclStemnt::dup0() const
{
  DeclStemnt *ret;
  if (type == TypedefStemnt)
    ret = new ::TypedefStemnt(location);
  else ret = new DeclStemnt(location);

  for (DeclVector::const_iterator i = decls.begin(); i != decls.end(); ++i)
    ret->addDecl((*i)->dup());

  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());

  return ret;
}

DeclStemnt*
DeclStemnt::convertToTypedef()
{
  // Nothing to do?
  if (isTypedef()) return this;

  ::TypedefStemnt *ret = new ::TypedefStemnt(location);

  // Since we are really the same thing,
  // let's just steal the insides.
  for (LabelVector::iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel(*i);
  labels.clear();

  ret->next    = next;
  next = 0;

  for (DeclVector::iterator i = decls.begin(); i != decls.end(); ++i)
    ret->addDecl(*i);
  decls.clear();

  delete this;        // Danger - Will Robinson!
  return ret;
}

void
DeclStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* DeclStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }

  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out,level);

  indent(out,level);

  if (!decls.empty())
  {
    DeclVector::const_iterator i = decls.begin();

    (*i)->print(out,true,level);
    for (++i; i != decls.end(); ++i)
    {
      out << ", ";
      (*i)->print(out,false,level);
    }
  }

  out << ";";
}

void
DeclStemnt::findExpr(fnExprCallback cb)
{
  Statement::findExpr(cb);

  for (DeclVector::iterator i = decls.begin(); i != decls.end(); ++i)
    (*i)->findExpr(cb);
}

TypedefStemnt::TypedefStemnt(const Location &l)
  : DeclStemnt(l, Statement::TypedefStemnt)
{
}

TypedefStemnt::~TypedefStemnt()
{
}

void
TypedefStemnt::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* TypedefStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }

  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out,level);

  indent(out,level);

  out << "typedef ";

  if (!decls.empty())
  {
    DeclVector::const_iterator i = decls.begin();
    
    (*i)->print(out,true,level);
    for (++i; i != decls.end(); ++i)
    {
      out << ", ";
      (*i)->print(out,false,level);
    }
  }
  out << ";";
}

Block::Block(const Location &l)
  : Statement(Statement::Block, l),
    head(0),
    tail(0)
{
}

Block::~Block()
{
  Statement *stemnt, *prevStemnt = 0;

  for (stemnt= head; stemnt; stemnt = stemnt->next)
  {
    delete prevStemnt;
    prevStemnt = stemnt;
  }
  delete prevStemnt;
}

void
Block::add(Statement *stemnt)
{
  if (stemnt)
  {
    stemnt->next = 0;
    if (tail) tail->next = stemnt;
    else head = stemnt;
    tail = stemnt;
  }
}

void
Block::addStatements(Statement *stemnts)
{
  Statement *stemnt;
  while (stemnts != 0)
  {
    stemnt = stemnts;
    stemnts = stemnts->next;

    stemnt->next = 0;
    add(stemnt);
  }
}

void
Block::addDecls(Decl *decls)
{
  Decl *decl = decls;
  while (decls != 0)
  {
    ::DeclStemnt *ds = new ::DeclStemnt(location);
    decls = decls->next;
    decl->next = 0;

    ds->addDecl(decl);
    add(ds);        
    decl = decls;
  }
}

Statement*
Block::dup0() const
{
  Block *ret = new Block(location);
  for (Statement *stemnt = head; stemnt; stemnt = stemnt->next)
    ret->add(stemnt->dup());

  for (LabelVector::const_iterator i =labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
Block::print(std::ostream& out, int level) const
{
  if (Statement::debug)
  {
    out << "/* BlockStemnt:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }

  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    (*i)->print(out,level);
  
  indent(out,level);
  out << "{\n";

  bool isDecl = (head != 0) ? head->isDeclaration() : false;
  for (Statement *stemnt = head; stemnt; stemnt = stemnt->next)
  {
    if (isDecl && !stemnt->isDeclaration())
    {
      isDecl = false;
      out << std::endl;
    }

    stemnt->print(out,level+1);

    if (stemnt->needSemicolon())
      out << ";";
    out << std::endl;
  }

  indent(out,level);
  out << "}\n";
}

void
Block::findExpr(fnExprCallback cb)
{
  Statement *stemnt;

  Statement::findExpr(cb);

  for (stemnt=head; stemnt; stemnt=stemnt->next)
    stemnt->findExpr(cb);
}

void
Block::find_statement(fnStemntCallback cb)
{
  (cb)(this);
  for (Statement *stemnt = head; stemnt; stemnt = stemnt->next)
    stemnt->find_statement(cb);
}

void
Block::insert(Statement *stemnt, Statement *after /* =NULL */)
{
  if (stemnt)
  {
    stemnt->next = 0;
    if (tail)
    {
      if (after)
      {
	stemnt->next = after->next;
	after->next = stemnt;
      }
      else
      {
	stemnt->next = head;
	head = stemnt;
      }
      
      if (stemnt->next == 0) tail = stemnt;
    }
    else head = tail = stemnt;
  }
}

FunctionDef::FunctionDef(const Location &l)
  : Block(l),
    decl(0)
{
}

FunctionDef::~FunctionDef()
{
  delete decl;
}

Statement*
FunctionDef::dup0() const
{
  FunctionDef *ret = new FunctionDef(location);
  ret->decl = decl->dup();

  for (Statement *stemnt=head; stemnt; stemnt=stemnt->next)
    ret->add(stemnt->dup());

  for (LabelVector::const_iterator i = labels.begin(); i != labels.end(); ++i)
    ret->addLabel((*i)->dup());
  return ret;
}

void
FunctionDef::print(std::ostream& out, int) const
{
  if (Statement::debug)
  {
    out << "/* FunctionDef:" ;
    location.printLocation(out) ;
    out << " */" << std::endl;
  }

  decl->print(out,true); 
  out << std::endl;

  Block::print(out,0);
}

void
FunctionDef::findExpr(fnExprCallback cb)
{
  decl->findExpr(cb);
  Block::findExpr(cb);
}

Symbol*
FunctionDef::FunctionName() const
{
  return decl->name;
}

std::ostream&
operator<< (std::ostream& out, const Statement& stemnt)
{
  stemnt.print(out,0);
  return out;
}

#define  SHOW(X)  case X: return #X

char*
nameOfStatementType(Statement::Type type)
{
  switch (type)
  {
    default:
      return "Unknown StatementType";

    SHOW(Statement::NullStemnt);
    SHOW(Statement::DeclStemnt);
    SHOW(Statement::ExpressionStemnt);
    SHOW(Statement::IfStemnt);
    SHOW(Statement::SwitchStemnt);
    SHOW(Statement::ForStemnt);
    SHOW(Statement::WhileStemnt);
    SHOW(Statement::DoWhileStemnt);
    SHOW(Statement::ContinueStemnt);
    SHOW(Statement::BreakStemnt);
    SHOW(Statement::GotoStemnt);
    SHOW(Statement::ReturnStemnt);
    SHOW(Statement::Block);
    SHOW(Statement::InclStemnt);
    SHOW(Statement::EndInclStemnt);
  }
}

char*
nameOfLabelType(Label::Type type)
{
  switch (type)
  {
    default:
      return "Unknown LabelType";

    SHOW(Label::None);        // No label - invalid.
    SHOW(Label::Default);     // default:
    SHOW(Label::Case);        // case <expr>:
    SHOW(Label::Goto);        // A named label (goto destination).
  }
}

#undef SHOW

