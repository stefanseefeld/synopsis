/*
  Copyright (C) 1997-2001 Shigeru Chiba, Tokyo Institute of Technology.

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/

#ifndef _Parseer_hh
#define _Parseer_hh

#include "types.h"

class Lexer;
class Token;
class Environment;
namespace PTree {class Node;}
class Encoding;

/*
  Naming conventions for member functions

  r<name>:   grammar rule (terminal or non-terminal)
  opt<name>: optional terminal/non-termianl symbol
  is<name>:  is the following symbol <name>?
*/

class Parser : public Object 
{
public:
  Parser(Lexer*);
  bool ErrorMessage(const char*, PTree::Node * = 0, PTree::Node * = 0);
  void WarningMessage(const char*, PTree::Node * = 0, PTree::Node * = 0);
  int NumOfErrors() { return nerrors; }

  //. Return the origin of the given pointer
  //. (filename and line number)
  unsigned long origin(const char *, std::string &) const;

  bool rProgram(PTree::Node *&);

protected:
  enum DeclKind { kDeclarator, kArgDeclarator, kCastDeclarator };
  enum TemplateDeclKind { tdk_unknown, tdk_decl, tdk_instantiation, 
			  tdk_specialization, num_tdks };

  bool SyntaxError();
  void ShowMessageHead(char*);

  bool rDefinition(PTree::Node *&);
  bool rNullDeclaration(PTree::Node *&);
  bool rTypedef(PTree::Node *&);
  bool rTypeSpecifier(PTree::Node *&, bool, Encoding&);
  bool isTypeSpecifier();
  bool rMetaclassDecl(PTree::Node *&);
  bool rMetaArguments(PTree::Node *&);
  bool rLinkageSpec(PTree::Node *&);
  bool rNamespaceSpec(PTree::Node *&);
  bool rNamespaceAlias(PTree::Node *&);
  bool rUsing(PTree::Node *&);
  bool rLinkageBody(PTree::Node *&);
  bool rTemplateDecl(PTree::Node *&);
  bool rTemplateDecl2(PTree::Node *&, TemplateDeclKind &kind);
  bool rTempArgList(PTree::Node *&);
  bool rTempArgDeclaration(PTree::Node *&);
  bool rExternTemplateDecl(PTree::Node *&);

  bool rDeclaration(PTree::Node *&);
  bool rIntegralDeclaration(PTree::Node *&, Encoding&, PTree::Node *, PTree::Node *, PTree::Node *);
  bool rConstDeclaration(PTree::Node *&, Encoding&, PTree::Node *, PTree::Node *);
  bool rOtherDeclaration(PTree::Node *&, Encoding&, PTree::Node *, PTree::Node *, PTree::Node *);
  bool rCondition(PTree::Node *&);
  bool rSimpleDeclaration(PTree::Node *&);

  bool isConstructorDecl();
  bool isPtrToMember(int);
  bool optMemberSpec(PTree::Node *&);
  bool optStorageSpec(PTree::Node *&);
  bool optCvQualify(PTree::Node *&);
  bool optIntegralTypeOrClassSpec(PTree::Node *&, Encoding&);
  bool rConstructorDecl(PTree::Node *&, Encoding&);
  bool optThrowDecl(PTree::Node *&);
  
  bool rDeclarators(PTree::Node *&, Encoding&, bool, bool = false);
  bool rDeclaratorWithInit(PTree::Node *&, Encoding&, bool, bool);
  bool rDeclarator(PTree::Node *&, DeclKind, bool, Encoding&, Encoding&, bool,
		   bool = false);
  bool rDeclarator2(PTree::Node *&, DeclKind, bool, Encoding&, Encoding&, bool,
		    bool, PTree::Node **);
  bool optPtrOperator(PTree::Node *&, Encoding&);
  bool rMemberInitializers(PTree::Node *&);
  bool rMemberInit(PTree::Node *&);
  
  bool rName(PTree::Node *&, Encoding&);
  bool rOperatorName(PTree::Node *&, Encoding&);
  bool rCastOperatorName(PTree::Node *&, Encoding&);
  bool rPtrToMember(PTree::Node *&, Encoding&);
  bool rTemplateArgs(PTree::Node *&, Encoding&);
  
  bool rArgDeclListOrInit(PTree::Node *&, bool&, Encoding&, bool);
  bool rArgDeclList(PTree::Node *&, Encoding&);
  bool rArgDeclaration(PTree::Node *&, Encoding&);
  
  bool rFunctionArguments(PTree::Node *&);
  bool rInitializeExpr(PTree::Node *&);
  
  bool rEnumSpec(PTree::Node *&, Encoding&);
  bool rEnumBody(PTree::Node *&);
  bool rClassSpec(PTree::Node *&, Encoding&);
  bool rBaseSpecifiers(PTree::Node *&);
  bool rClassBody(PTree::Node *&);
  bool rClassMember(PTree::Node *&);
  bool rAccessDecl(PTree::Node *&);
  bool rUserAccessSpec(PTree::Node *&);
  
  bool rCommaExpression(PTree::Node *&);
  
  bool rExpression(PTree::Node *&);
  bool rConditionalExpr(PTree::Node *&, bool);
  bool rLogicalOrExpr(PTree::Node *&, bool);
  bool rLogicalAndExpr(PTree::Node *&, bool);
  bool rInclusiveOrExpr(PTree::Node *&, bool);
  bool rExclusiveOrExpr(PTree::Node *&, bool);
  bool rAndExpr(PTree::Node *&, bool);
  bool rEqualityExpr(PTree::Node *&, bool);
  bool rRelationalExpr(PTree::Node *&, bool);
  bool rShiftExpr(PTree::Node *&);
  bool rAdditiveExpr(PTree::Node *&);
  bool rMultiplyExpr(PTree::Node *&);
  bool rPmExpr(PTree::Node *&);
  bool rCastExpr(PTree::Node *&);
  bool rTypeName(PTree::Node *&);
  bool rTypeName(PTree::Node *&, Encoding&);
  bool rUnaryExpr(PTree::Node *&);
  bool rThrowExpr(PTree::Node *&);
  bool rSizeofExpr(PTree::Node *&);
  bool rTypeidExpr(PTree::Node *&);
  bool isAllocateExpr(int);
  bool rAllocateExpr(PTree::Node *&);
  bool rUserdefKeyword(PTree::Node *&);
  bool rAllocateType(PTree::Node *&);
  bool rNewDeclarator(PTree::Node *&, Encoding&);
  bool rAllocateInitializer(PTree::Node *&);
  bool rPostfixExpr(PTree::Node *&);
  bool rPrimaryExpr(PTree::Node *&);
  bool rUserdefStatement(PTree::Node *&);
  bool rVarName(PTree::Node *&);
  bool rVarNameCore(PTree::Node *&, Encoding&);
  bool isTemplateArgs();
  
  bool rFunctionBody(PTree::Node *&);
  bool rCompoundStatement(PTree::Node *&);
  bool rStatement(PTree::Node *&);
  bool rIfStatement(PTree::Node *&);
  bool rSwitchStatement(PTree::Node *&);
  bool rWhileStatement(PTree::Node *&);
  bool rDoStatement(PTree::Node *&);
  bool rForStatement(PTree::Node *&);
  bool rTryStatement(PTree::Node *&);
  
  bool rExprStatement(PTree::Node *&);
  bool rDeclarationStatement(PTree::Node *&);
  bool rIntegralDeclStatement(PTree::Node *&, Encoding&, PTree::Node *, PTree::Node *, PTree::Node *);
  bool rOtherDeclStatement(PTree::Node *&, Encoding&, PTree::Node *, PTree::Node *);
  
  bool MaybeTypeNameOrClassTemplate(Token&);
  void SkipTo(int token);
  
private:
  bool moreVarName();

private:
  Lexer *lex;
  int nerrors;
  PTree::Node *comments;
};

#endif
