//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/PTree/Visitor.hh>
#include <Synopsis/PTree/Atoms.hh>
#include <Synopsis/PTree/Lists.hh>

using namespace Synopsis;
using namespace PTree;

void Visitor::visit(Literal *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(CommentedAtom *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(DupAtom *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(Identifier *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(Reserved *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(This *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomAUTO *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomBOOLEAN *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomCHAR *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomWCHAR *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomCONST *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomDOUBLE *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomEXTERN *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomFLOAT *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomFRIEND *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomINLINE *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomINT *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomLONG *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomMUTABLE *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomNAMESPACE *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomPRIVATE *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomPROTECTED *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomPUBLIC *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomREGISTER *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomSHORT *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomSIGNED *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomSTATIC *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomUNSIGNED *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomUSING *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomVIRTUAL *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomVOID *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(AtomVOLATILE *a) { visit(static_cast<Atom *>(a));}
void Visitor::visit(Brace *l) { visit(static_cast<List *>(l));}
void Visitor::visit(Block *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ClassBody *l) { visit(static_cast<List *>(l));}
void Visitor::visit(Typedef *l) { visit(static_cast<List *>(l));}
void Visitor::visit(TemplateDecl *l) { visit(static_cast<List *>(l));}
void Visitor::visit(TemplateInstantiation *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ExternTemplate *l) { visit(static_cast<List *>(l));}
void Visitor::visit(MetaclassDecl *l) { visit(static_cast<List *>(l));}
void Visitor::visit(LinkageSpec *l) { visit(static_cast<List *>(l));}
void Visitor::visit(NamespaceSpec *l) { visit(static_cast<List *>(l));}
void Visitor::visit(NamespaceAlias *l) { visit(static_cast<List *>(l));}
void Visitor::visit(Using *l) { visit(static_cast<List *>(l));}
void Visitor::visit(Declaration *l) { visit(static_cast<List *>(l));}
void Visitor::visit(Declarator *l) { visit(static_cast<List *>(l));}
void Visitor::visit(Name *l) { visit(static_cast<List *>(l));}
void Visitor::visit(FstyleCastExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ClassSpec *l) { visit(static_cast<List *>(l));}
void Visitor::visit(EnumSpec *l) { visit(static_cast<List *>(l));}
void Visitor::visit(AccessSpec *l) { visit(static_cast<List *>(l));}
void Visitor::visit(AccessDecl *l) { visit(static_cast<List *>(l));}
void Visitor::visit(UserAccessSpec *l) { visit(static_cast<List *>(l));}
void Visitor::visit(IfStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(SwitchStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(WhileStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(DoStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ForStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(TryStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(BreakStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ContinueStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ReturnStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(GotoStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(CaseStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(DefaultStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(LabelStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ExprStatement *l) { visit(static_cast<List *>(l));}
void Visitor::visit(CommaExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(AssignExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(CondExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(InfixExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(PmExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(CastExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(UnaryExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ThrowExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(SizeofExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(TypeidExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(TypeofExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(NewExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(DeleteExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ArrayExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(FuncallExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(PostfixExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(DotMemberExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ArrowMemberExpr *l) { visit(static_cast<List *>(l));}
void Visitor::visit(ParenExpr *l) { visit(static_cast<List *>(l));}
