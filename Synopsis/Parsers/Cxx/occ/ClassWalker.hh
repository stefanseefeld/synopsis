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

class TypeInfo;

class ClassWalker : public Walker {
public:
    ClassWalker(Parser* p) : Walker(p) { client_data = 0;}
    ClassWalker(Parser* p, Environment* e)
	: Walker(p, e) { client_data = 0;}
    ClassWalker(Environment* e) : Walker(e) { client_data = 0;}
    ClassWalker(Walker* w) : Walker(w) { client_data = 0;}

  bool is_class_walker() { return true;}
    void InsertBeforeStatement(PTree::Node *);
    void AppendAfterStatement(PTree::Node *);
    void InsertBeforeToplevel(PTree::Node *);
    void AppendAfterToplevel(PTree::Node *);
    bool InsertDeclaration(PTree::Node *, Class*, PTree::Node *, void*);
    void* LookupClientData(Class*, PTree::Node *);

    PTree::Node *GetInsertedPtree();
    PTree::Node *GetAppendedPtree();

    PTree::Node *translate_metaclass_decl(PTree::Node *decl);
    PTree::Node *translate_class_spec(PTree::Node *spec, PTree::Node *userkey,
				      PTree::Node *class_def, Class* metaobject);
    PTree::Node *translate_template_instantiation(PTree::TemplateInstantiation *,
						  PTree::Node *,
						  PTree::ClassSpec *,
						  Class *);
    PTree::Node *ConstructClass(Class* metaobject);

    PTree::Node *ConstructAccessSpecifier(int access);
    PTree::Node *ConstructMember(void* /* i.e. ChangedMemberList::Mem* */);

    PTree::Node *translate_storage_specifiers(PTree::Node *);
    PTree::Node *translate_template_function(PTree::Node *temp_def, PTree::Node *impl);
    Class* MakeMetaobjectForCfunctions();
    PTree::Node *translate_function_implementation(PTree::Node *);
    PTree::Node *MakeMemberDeclarator(bool record,
				void* /* aka ChangedMemberList::Mem* */,
				PTree::Declarator*);
    PTree::Node *record_args_and_translate_fbody(Class*, PTree::Node *args, PTree::Node *body);
    PTree::Node *translate_function_body(PTree::Node *);
    PTree::Node *translate_arg_decl_list(bool, PTree::Node *, PTree::Node *);
    PTree::Node *translate_initialize_args(PTree::Declarator*, PTree::Node *);
    PTree::Node *translate_assign_initializer(PTree::Declarator*, PTree::Node *);
    PTree::Node *translate_new2(PTree::Node *, PTree::Node *, PTree::Node *, PTree::Node *, PTree::Node *,
				PTree::Node *, PTree::Node *);

    PTree::Node *translate_user_accessSpec(PTree::Node *);
    PTree::Node *translate_variable(PTree::Node *);
    void visit(PTree::Block *);
    void visit(PTree::UserAccessSpec *node) { my_result = 0;}
    void visit(PTree::AssignExpr *);
    void visit(PTree::InfixExpr *);
    void visit(PTree::UnaryExpr *);
    void visit(PTree::ArrayExpr *);
    void visit(PTree::PostfixExpr *);
    void visit(PTree::FuncallExpr *);
    void visit(PTree::DotMemberExpr *);
    void visit(PTree::ArrowMemberExpr *);
    void visit(PTree::This *);
    void visit(PTree::Name *node) { my_result = translate_variable(node);}
    void visit(PTree::Identifier *node) { my_result = translate_variable(node);}
    void visit(PTree::UserStatementExpr *);
    void visit(PTree::StaticUserStatementExpr *);
    void visit(PTree::DeleteExpr *);

private:
    static Class *get_class_metaobject(TypeInfo&);
    PTree::Array* RecordMembers(PTree::Node *, PTree::Node *, Class*);
    void RecordMemberDeclaration(PTree::Node *mem, PTree::Array* tspec_list);
    PTree::Node *translate_storage_specifiers2(PTree::Node *rest);

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
