/*
  Copyright (C) 1997-2000 Shigeru Chiba, University of Tsukuba.

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/

#ifndef _ClassWalker_hh
#define _ClassWalker_hh

#include "PTree.hh"
#include "Walker.hh"

class ClassWalker : public Walker {
public:
    ClassWalker(Parser* p) : Walker(p) { client_data = 0;}
    ClassWalker(Parser* p, Environment* e)
	: Walker(p, e) { client_data = 0;}
    ClassWalker(Environment* e) : Walker(e) { client_data = 0;}
    ClassWalker(Walker* w) : Walker(w) { client_data = 0;}

    bool IsClassWalker();
    void InsertBeforeStatement(PTree::Node *);
    void AppendAfterStatement(PTree::Node *);
    void InsertBeforeToplevel(PTree::Node *);
    void AppendAfterToplevel(PTree::Node *);
    bool InsertDeclaration(PTree::Node *, Class*, PTree::Node *, void*);
    void* LookupClientData(Class*, PTree::Node *);

    PTree::Node *GetInsertedPtree();
    PTree::Node *GetAppendedPtree();

    PTree::Node *TranslateMetaclassDecl(PTree::Node *decl);
    PTree::Node *TranslateClassSpec(PTree::Node *spec, PTree::Node *userkey,
			      PTree::Node *class_def, Class* metaobject);
    PTree::Node *TranslateTemplateInstantiation(PTree::Node *spec, PTree::Node *userkey,
					PTree::Node *class_def, Class* metaobject);
    PTree::Node *ConstructClass(Class* metaobject);

    PTree::Node *ConstructAccessSpecifier(int access);
    PTree::Node *ConstructMember(void* /* i.e. ChangedMemberList::Mem* */);

    PTree::Node *TranslateStorageSpecifiers(PTree::Node *);
    PTree::Node *TranslateTemplateFunction(PTree::Node *temp_def, PTree::Node *impl);
    Class* MakeMetaobjectForCfunctions();
    PTree::Node *TranslateFunctionImplementation(PTree::Node *);
    PTree::Node *MakeMemberDeclarator(bool record,
				void* /* aka ChangedMemberList::Mem* */,
				PTree::Declarator*);
    PTree::Node *RecordArgsAndTranslateFbody(Class*, PTree::Node *args, PTree::Node *body);
    PTree::Node *TranslateFunctionBody(PTree::Node *);
    PTree::Node *TranslateBlock(PTree::Node *);
    PTree::Node *TranslateArgDeclList(bool, PTree::Node *, PTree::Node *);
    PTree::Node *TranslateInitializeArgs(PTree::Declarator*, PTree::Node *);
    PTree::Node *TranslateAssignInitializer(PTree::Declarator*, PTree::Node *);
    PTree::Node *TranslateUserAccessSpec(PTree::Node *);
    PTree::Node *TranslateAssign(PTree::Node *);
    PTree::Node *TranslateInfix(PTree::Node *);
    PTree::Node *TranslateUnary(PTree::Node *);
    PTree::Node *TranslateArray(PTree::Node *);
    PTree::Node *TranslatePostfix(PTree::Node *);
    PTree::Node *TranslateFuncall(PTree::Node *);
    PTree::Node *TranslateDotMember(PTree::Node *);
    PTree::Node *TranslateArrowMember(PTree::Node *);
    PTree::Node *TranslateThis(PTree::Node *);
    PTree::Node *TranslateVariable(PTree::Node *);
    PTree::Node *TranslateUserStatement(PTree::Node *);
    PTree::Node *TranslateStaticUserStatement(PTree::Node *);
    PTree::Node *TranslateNew2(PTree::Node *, PTree::Node *, PTree::Node *, PTree::Node *, PTree::Node *,
			 PTree::Node *, PTree::Node *);
    PTree::Node *TranslateDelete(PTree::Node *);

private:
    static Class* GetClassMetaobject(TypeInfo&);
    PTree::Array* RecordMembers(PTree::Node *, PTree::Node *, Class*);
    void RecordMemberDeclaration(PTree::Node *mem, PTree::Array* tspec_list);
    PTree::Node *TranslateStorageSpecifiers2(PTree::Node *rest);

    static PTree::Node *CheckMemberEquiv(PTree::Node *, PTree::Node *);
    static PTree::Node *CheckEquiv(PTree::Node *p, PTree::Node *q) {
      return PTree::equiv(p, q) ? p : q;
    }

private:
  struct ClientDataLink : public PTree::LightObject {
	ClientDataLink*	next;
	Class*		metaobject;
	PTree::Node *  	key;
	void*		data;
    };

    PTree::Array before_statement, after_statement;
    PTree::Array before_toplevel, after_toplevel;
    PTree::Array inserted_declarations;
    ClientDataLink* client_data;
};

#endif
