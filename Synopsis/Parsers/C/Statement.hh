/* $Id: Statement.hh,v 1.5 2003/12/30 07:33:06 stefan Exp $
 *
 * Copyright (C) 1998 Shaun Flisakowski
 * Copyright (C) 2003 Stefan Seefeld
 * All rights reserved.
 * Licensed to the public under the terms of the GNU LGPL (>= 2),
 * see the file COPYING for details.
 */

#ifndef _Statement_hh
#define _Statement_hh

#include <Symbol.hh>
#include <Expression.hh>
#include <Declaration.hh>
#include <Callback.hh>
#include <Location.hh>
#include <Dup.hh>
#include <Traversal.hh>

#include <cstdlib>
#include <iostream>
#include <vector>

class Label
{
  public:
  enum Type
  {
    None=0,              // No label - invalid.
    Default,             // default:
    Case,                // case <expr>:
    CaseRange,           // case <expr> ... <expr>: (gcc extension)
    Goto                 // A named label (goto destination).
  };

  Label(Type);
  Label(Expression *_expr);
  Label(Expression *begin, Expression *end);
  Label(Symbol *);
  ~Label();

  Label *dup0() const;
  Label *dup() const; 
  void print(std::ostream& out, int level) const;

  void    findExpr(fnExprCallback cb);

  Type    type;

  union
  {
    Expression    *begin;   // for Case, CaseRange
    Symbol        *name;    // for Goto
  };

  Expression    *end;       // for CaseRange
  SymEntry      *syment;
};

typedef std::vector<Label *> LabelVector;

class Statement : public Dup<Statement>
{
  public:
  enum Type
  {
    NullStemnt=0,              // The null statement.

    DeclStemnt,
    TypedefStemnt,
    ExpressionStemnt,

    IfStemnt,
    SwitchStemnt,

    ForStemnt,
    WhileStemnt,
    DoWhileStemnt,

    ContinueStemnt,
    BreakStemnt,
    GotoStemnt,
    ReturnStemnt,

    Block,

    FileLineStemnt,       // #line #file
    InclStemnt,           // #include
    EndInclStemnt
  };

  Statement(Type, const Location &);
  virtual ~Statement();

  virtual void accept(Traversal *t) { t->traverse_statement(this);}

  void addLabel(Label *);
  void addHeadLabel(Label *); 

  virtual bool isBlock() const { return false;}
  virtual bool isFuncDef() const { return false;}
  virtual bool isDeclaration() const { return false;}
  virtual bool isTypedef() const { return false;}

  virtual bool needSemicolon() const { return (type != NullStemnt);}

  virtual bool isFileLine() const { return false;}
  virtual bool isInclude() const { return false;}
  virtual bool isEndInclude() const { return false;}

  virtual Statement *dup0() const;
  virtual void print(std::ostream &out, int level) const;

  virtual void findExpr(fnExprCallback cb);
  virtual void find_statement(fnStemntCallback cb ) { (cb)(this);}

  static bool verbose;
  static bool debug;

  Type        type;
  LabelVector labels;
  Location    location;

  Statement  *next;     // For making a list of statements.

  friend std::ostream& operator<< (std::ostream& out, const Statement& stemnt);
};

class FileLineStemnt : public Statement
{
public:
  FileLineStemnt(const std::string &incl, int lino, const Location &location);
  virtual ~FileLineStemnt();

  virtual void accept(Traversal *t) { t->traverse_file_line(this);}

  bool isFileline() const { return true;}
  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;

  std::string  filename;
  int          linenumber;
};

class InclStemnt : public Statement
{
public:
  InclStemnt(const std::string& incl, const Location& location);
  virtual ~InclStemnt();

  virtual void accept(Traversal *t) { t->traverse_include(this);}

  bool isInclude() const { return true;}
  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;

  bool         isStandard;      // use brackets
  std::string  filename;
};

class EndInclStemnt : public Statement
{
public:
  EndInclStemnt(const Location& location);
  virtual ~EndInclStemnt();
  
  virtual void accept(Traversal *t) { t->traverse_end_include(this);}
  
  bool isEndInclude() const { return true;}
  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;
};

class ExpressionStemnt : public Statement
{
public:
  ExpressionStemnt( Expression *expr, const Location& location);
  virtual ~ExpressionStemnt();

  virtual void accept(Traversal *t) { t->traverse_expression(this);}

  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;

  void findExpr(fnExprCallback cb);

  Expression    *expression;
};

class IfStemnt : public Statement
{
public:
  IfStemnt(Expression *cond,
	   Statement *thenBlk, const Location &location,
	   Statement *elseBlk = 0);
  ~IfStemnt();

  virtual void accept(Traversal *t) { t->traverse_if(this);}

  bool needSemicolon() const { return false;}

  Statement *dup0() const;
  void print(std::ostream& out, int level) const;

  void findExpr( fnExprCallback cb);
  void find_statement( fnStemntCallback cb);

  Expression    *cond;

  Statement     *thenBlk;
  Statement     *elseBlk;
};

class SwitchStemnt : public Statement
{
public:
  SwitchStemnt(Expression *cond, Statement *block,
	       const Location &location);
  ~SwitchStemnt();

  virtual void accept(Traversal *t) { t->traverse_switch(this);}

  bool needSemicolon() const { return false;}
  
  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;

  void findExpr(fnExprCallback cb);
  void find_statement(fnStemntCallback cb);

  Expression    *cond;
  Statement     *block;
};

class ForStemnt : public Statement
{
  public:
  ForStemnt(Expression *init, Expression *cond, Expression *incr,
	    const Location &location, Statement *block = 0);
  ~ForStemnt();

  virtual void accept(Traversal *t) { t->traverse_for(this);}

  bool needSemicolon() const { return false;}

  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;

  void findExpr(fnExprCallback cb);
    void find_statement(fnStemntCallback cb);

  Expression    *init;
  Expression    *cond;
  Expression    *incr;

  Statement     *block;
};

class WhileStemnt : public Statement
{
public:
  WhileStemnt(Expression *cond, Statement *block,
	      const Location& location);
  ~WhileStemnt();

  virtual void accept(Traversal *t) { t->traverse_while(this);}

  bool needSemicolon() const { return false;}

  Statement *dup0() const;
  void print(std::ostream& out, int level) const;

  void findExpr(fnExprCallback cb);
  void find_statement(fnStemntCallback cb);

  Expression    *cond;
  Statement     *block;
};

class DoWhileStemnt : public Statement
{
public:
  DoWhileStemnt(Expression *cond, Statement *block,
		const Location &location);
  ~DoWhileStemnt();

  virtual void accept(Traversal *t) { t->traverse_do_while(this);}

  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;

  void findExpr(fnExprCallback cb);
  void find_statement(fnStemntCallback cb);

  Expression    *cond;
  Statement     *block;
};

class GotoStemnt : public Statement
{
public:
  GotoStemnt(Symbol *dest, const Location &location);
  ~GotoStemnt();

  virtual void accept(Traversal *t) { t->traverse_goto(this);}

  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;

  Symbol        *dest;
};

class ReturnStemnt : public Statement
{
public:
  ReturnStemnt(Expression *result, const Location& location);
  ~ReturnStemnt();

  virtual void accept(Traversal *t) { t->traverse_return(this);}

  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;

  void findExpr(fnExprCallback cb);

  Expression    *result;        // Can be NULL.
};

class DeclStemnt : public Statement
{
public:
  DeclStemnt(const Location &location, Type stype = Statement::DeclStemnt);
  virtual ~DeclStemnt();

  virtual void accept(Traversal *t) { t->traverse_declaration(this);}

  // Declarations print their own semicolon.
  bool needSemicolon() const { return false;}
  bool isDeclaration() const { return true;}

  Statement *dup0() const; 
  virtual void print(std::ostream& out, int level) const;

  // Only converts if necessary.
  DeclStemnt *convertToTypedef();

  void addDecl(Decl *decl);
  void addDecls(Decl *decls);

  void findExpr(fnExprCallback cb);

  // The type(s).
  DeclVector    decls;
};

class TypedefStemnt : public DeclStemnt
{
public:
  TypedefStemnt(const Location &location);
  ~TypedefStemnt();

  virtual void accept(Traversal *t) { t->traverse_typedef(this);}

  bool isTypedef() const { return true;}

  void print(std::ostream& out, int level) const;
};

class Block : public Statement
{
public:
  Block(const Location &location);
  virtual ~Block();

  virtual void accept(Traversal *t) { t->traverse_block(this);}

  void add(Statement *stemnt);

  void addStatements(Statement *stemnt);
  void addDecls(Decl *decls);

  virtual bool isBlock() const { return true;}
  bool needSemicolon() const { return false;}

  virtual Statement *dup0() const; 
  virtual void print(std::ostream &out, int level) const;

  virtual void findExpr(fnExprCallback cb);
  void find_statement(fnStemntCallback cb);

  void insert(Statement *stemnt, Statement *after = 0);

  Statement     *head;    // List of statements in this block.
  Statement     *tail;
};

// Not really true, but this is very convienent.
class FunctionDef : public Block
{
public:
  FunctionDef(const Location &location);
  ~FunctionDef();

  virtual void accept(Traversal *t) { t->traverse_function_definition(this);}

  bool isFuncDef() const { return true;}

  Statement *dup0() const; 
  void print(std::ostream& out, int level) const;

  void findExpr(fnExprCallback cb);

  Symbol  *FunctionName() const;

  Decl          *decl;
};

Statement  *ReverseList(Statement *);

void        indent(std::ostream& out, int level);
void        printNull(std::ostream& out, int level);
void        printBlock(std::ostream& out, int level, Statement *block);


// For debugging.
char *nameOfStatementType(Statement::Type type);
char *nameOfLabelType(Label::Type type);

#endif

