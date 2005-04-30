//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_PTree_Visitor_hh_
#define Synopsis_PTree_Visitor_hh_

namespace Synopsis
{
namespace PTree
{

  class Node;
  class Atom;
  class List;
  class Literal;
  class CommentedAtom;
  class DupAtom;
  class Identifier;
  class Reserved;
  class This;
  class AtomAUTO;
  class AtomBOOLEAN;
  class AtomCHAR;
  class AtomWCHAR;
  class AtomCONST;
  class AtomDOUBLE;
  class AtomEXTERN;
  class AtomFLOAT;
  class AtomFRIEND;
  class AtomINLINE;
  class AtomINT;
  class AtomLONG;
  class AtomMUTABLE;
  class AtomNAMESPACE;
  class AtomPRIVATE;
  class AtomPROTECTED;
  class AtomPUBLIC;
  class AtomREGISTER;
  class AtomSHORT;
  class AtomSIGNED;
  class AtomSTATIC;
  class AtomUNSIGNED;
  class AtomUSING;
  class AtomVIRTUAL;
  class AtomVOID;
  class AtomVOLATILE;
  class Brace;
  class Block;
  class ClassBody;
  class Typedef;
  class TemplateDecl;
  class TemplateInstantiation;
  class ExternTemplate;
  class MetaclassDecl;
  class LinkageSpec;
  class NamespaceSpec;
  class NamespaceAlias;
  class UsingDirective;
  class Declaration;
  class UsingDeclaration;
  class Declarator;
  class Name;
  class FstyleCastExpr;
  class ClassSpec;
  class EnumSpec;
  class AccessSpec;
  class AccessDecl;
  class UserAccessSpec;
  class IfStatement;
  class SwitchStatement;
  class WhileStatement;
  class DoStatement;
  class ForStatement;
  class TryStatement;
  class BreakStatement;
  class ContinueStatement;
  class ReturnStatement;
  class GotoStatement;
  class CaseStatement;
  class DefaultStatement;
  class LabelStatement;
  class ExprStatement;
  class CommaExpr;
  class AssignExpr;
  class CondExpr;
  class InfixExpr;
  class PmExpr;
  class CastExpr;
  class UnaryExpr;
  class ThrowExpr;
  class SizeofExpr;
  class TypeidExpr;
  class TypeofExpr;
  class NewExpr;
  class DeleteExpr;
  class ArrayExpr;
  class FuncallExpr;
  class PostfixExpr;
  class DotMemberExpr;
  class ArrowMemberExpr;
  class ParenExpr;

//. The Visitor class is used to dynamically resolve
//. type information about a given Node.
//. The default implementation does nothing, so you
//. only need to implement the methods you actually need.
//. Any types for which no corresponding 'visit' methods
//. exist will be caught by the 'visit' of the closest parent. 
class Visitor
{
public:
  virtual ~Visitor() {}
  virtual void visit(Node *) {}
  virtual void visit(Atom *) {}
  virtual void visit(List *) {}
  // atoms...
  virtual void visit(Literal *);
  virtual void visit(CommentedAtom *);
  virtual void visit(DupAtom *);
  virtual void visit(Identifier *);
  virtual void visit(Reserved *);
  virtual void visit(This *);
  virtual void visit(AtomAUTO *);
  virtual void visit(AtomBOOLEAN *);
  virtual void visit(AtomCHAR *);
  virtual void visit(AtomWCHAR *);
  virtual void visit(AtomCONST *);
  virtual void visit(AtomDOUBLE *);
  virtual void visit(AtomEXTERN *);
  virtual void visit(AtomFLOAT *);
  virtual void visit(AtomFRIEND *);
  virtual void visit(AtomINLINE *);
  virtual void visit(AtomINT *);
  virtual void visit(AtomLONG *);
  virtual void visit(AtomMUTABLE *);
  virtual void visit(AtomNAMESPACE *);
  virtual void visit(AtomPRIVATE *);
  virtual void visit(AtomPROTECTED *);
  virtual void visit(AtomPUBLIC *);
  virtual void visit(AtomREGISTER *);
  virtual void visit(AtomSHORT *);
  virtual void visit(AtomSIGNED *);
  virtual void visit(AtomSTATIC *);
  virtual void visit(AtomUNSIGNED *);
  virtual void visit(AtomUSING *);
  virtual void visit(AtomVIRTUAL *);
  virtual void visit(AtomVOID *);
  virtual void visit(AtomVOLATILE *);
  //. [ { [ <statement>* ] } ]
  virtual void visit(Brace *);
  //. [ { [ <statement>* ] } ]
  virtual void visit(Block *);
  virtual void visit(ClassBody *);
  virtual void visit(Typedef *);
  //. [ template < [types] > [decl] ]
  virtual void visit(TemplateDecl *);
  virtual void visit(TemplateInstantiation *);
  virtual void visit(ExternTemplate *);
  virtual void visit(MetaclassDecl *);
  //. [ extern ["C++"] [{ body }] ]
  virtual void visit(LinkageSpec *);
  //. [ namespace <identifier> [{ body }] ]
  virtual void visit(NamespaceSpec *);
  virtual void visit(NamespaceAlias *);
  //. [ using namespace Foo ; ]
  //. [ using namespace Foo = Bar ; ]
  virtual void visit(UsingDirective *);
  //. either variable, typedef or function
  //. Variables:
  //.  [ [modifiers] name [declarators] ; ]
  //. Function prototype:
  //.  [ [modifiers] name [declarators] ; ]
  //. Function impl:
  //.  [ [modifiers] name declarator [ { ... } ] ]
  //. Typedef:
  //.  ?
  //. Class definition:
  //.  [ [modifiers] [class foo ...] [declarators]? ; ]
  virtual void visit(Declaration *);
  //. [ using Foo :: x ; ]
  virtual void visit(UsingDeclaration *);
  //. [ [ declarator { = <expr> } ] , ... ]
  virtual void visit(Declarator *);
  virtual void visit(Name *);
  //. [ [type] ( [expr] ) ]
  virtual void visit(FstyleCastExpr *);
  virtual void visit(ClassSpec *);
  //. [ enum [name] [{ [name [= value] ]* }] ]
  virtual void visit(EnumSpec *);
  virtual void visit(AccessSpec *);
  virtual void visit(AccessDecl *);
  virtual void visit(UserAccessSpec *);
  //. [ if ( expr ) statement (else statement)? ]
  virtual void visit(IfStatement *);
  //. [ switch ( expr ) statement ]
  virtual void visit(SwitchStatement *);
  //. [ while ( expr ) statement ]
  virtual void visit(WhileStatement *);
  //. [ do [{ ... }] while ( [...] ) ; ]
  virtual void visit(DoStatement *);
  //. [ for ( stmt expr ; expr ) statement ]
  virtual void visit(ForStatement *);
  //. [ try [{}] [catch ( arg ) [{}] ]* ]
  virtual void visit(TryStatement *);
  //. [ break ; ]
  virtual void visit(BreakStatement *);
  virtual void visit(ContinueStatement *);
  virtual void visit(ReturnStatement *);
  virtual void visit(GotoStatement *);
  //. [ case expr : [expr] ]
  virtual void visit(CaseStatement *);
  //. [ default : [expr] ]
  virtual void visit(DefaultStatement *);
  virtual void visit(LabelStatement *);
  virtual void visit(ExprStatement *);
  //. [ expr , expr (, expr)* ]
  virtual void visit(CommaExpr *);
  //. [left = right]
  virtual void visit(AssignExpr *);
  virtual void visit(CondExpr *);
  //. [left op right]
  virtual void visit(InfixExpr *);
  virtual void visit(PmExpr *);
  //. ( type-expr ) expr   ..type-expr is type encoded
  virtual void visit(CastExpr *);
  //. [op expr]
  virtual void visit(UnaryExpr *);
  //. [ throw [expr] ]
  virtual void visit(ThrowExpr *);
  //. [ sizeof ( [type [???] ] ) ]
  virtual void visit(SizeofExpr *);
  virtual void visit(TypeidExpr *);
  virtual void visit(TypeofExpr *);
  virtual void visit(NewExpr *);
  //. [ delete [expr] ]
  virtual void visit(DeleteExpr *);
  //. <postfix> \[ <expr> \]
  virtual void visit(ArrayExpr *);
  //. [ postfix ( args ) ]
  virtual void visit(FuncallExpr *);
  //. [ expr ++ ]
  virtual void visit(PostfixExpr *);
  //. [ postfix . name ]
  virtual void visit(DotMemberExpr *);
  //. [ postfix -> name ]
  virtual void visit(ArrowMemberExpr *);
  //. [ ( expr ) ]
  virtual void visit(ParenExpr *);
};

}
}

#endif
