//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _PTree_Visitor_hh
#define _PTree_Visitor_hh

namespace PTree
{

 class Node;
 class Atom;
 class List;
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
 class Using;
 class Declaration;
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
  // ...lists...
  virtual void visit(Brace *);
  virtual void visit(Block *);
  virtual void visit(ClassBody *);
  virtual void visit(Typedef *);
  virtual void visit(TemplateDecl *);
  virtual void visit(TemplateInstantiation *);
  virtual void visit(ExternTemplate *);
  virtual void visit(MetaclassDecl *);
  virtual void visit(LinkageSpec *);
  virtual void visit(NamespaceSpec *);
  virtual void visit(NamespaceAlias *);
  virtual void visit(Using *);
  virtual void visit(Declaration *);
  virtual void visit(Declarator *);
  virtual void visit(Name *);
  virtual void visit(FstyleCastExpr *);
  virtual void visit(ClassSpec *);
  virtual void visit(EnumSpec *);
  virtual void visit(AccessSpec *);
  virtual void visit(AccessDecl *);
  virtual void visit(UserAccessSpec *);
  virtual void visit(IfStatement *);
  virtual void visit(SwitchStatement *);
  virtual void visit(WhileStatement *);
  virtual void visit(DoStatement *);
  virtual void visit(ForStatement *);
  virtual void visit(TryStatement *);
  virtual void visit(BreakStatement *);
  virtual void visit(ContinueStatement *);
  virtual void visit(ReturnStatement *);
  virtual void visit(GotoStatement *);
  virtual void visit(CaseStatement *);
  virtual void visit(DefaultStatement *);
  virtual void visit(LabelStatement *);
  virtual void visit(ExprStatement *);
  virtual void visit(CommaExpr *);
  virtual void visit(AssignExpr *);
  virtual void visit(CondExpr *);
  virtual void visit(InfixExpr *);
  virtual void visit(PmExpr *);
  virtual void visit(CastExpr *);
  virtual void visit(UnaryExpr *);
  virtual void visit(ThrowExpr *);
  virtual void visit(SizeofExpr *);
  virtual void visit(TypeidExpr *);
  virtual void visit(TypeofExpr *);
  virtual void visit(NewExpr *);
  virtual void visit(DeleteExpr *);
  virtual void visit(ArrayExpr *);
  virtual void visit(FuncallExpr *);
  virtual void visit(PostfixExpr *);
  virtual void visit(DotMemberExpr *);
  virtual void visit(ArrowMemberExpr *);
  virtual void visit(ParenExpr *);
};

}

#endif
