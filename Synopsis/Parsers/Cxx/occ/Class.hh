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

#ifndef _Class_hh
#define _Class_hh

#include <iosfwd>
#include <PTree.hh>
#include "Environment.hh"
#include "TypeInfo.hh"
#include "Member.hh"

class ClassArray;
class Member;
class MemberList;
class ChangedMemberList;
struct ChangedMemberList::Cmem;

class Class : public PTree::Object {
public:
    Class() {}
    Class(Environment* e, char* name) { Construct(e, PTree::make(name)); }
    Class(Environment* e, PTree::Node *name) { Construct(e, name); }

    virtual void InitializeInstance(PTree::Node *def, PTree::Node *margs);
    virtual ~Class();

// introspection

    PTree::Node *Comments();
    PTree::Node *Name();
    PTree::Node *BaseClasses();
    PTree::Node *Members();
    PTree::Node *Definition() { return definition; }
    virtual const char* MetaclassName();	// automaticallly implemented
					// by Metaclass
    Class* NthBaseClass(int nth);
    PTree::Node *NthBaseClassName(int nth);
    bool IsSubclassOf(PTree::Node *name);
    bool IsImmediateSubclassOf(PTree::Node *name);

    bool NthMember(int nth, Member& member);
    bool LookupMember(PTree::Node *name);
    bool LookupMember(PTree::Node *name, Member& member, int index = 0);
    bool LookupMember(char* name);
    bool LookupMember(char* name, Member& member, int index = 0);
    MemberList* GetMemberList();

    // These are available only within Finalize()
    static ClassArray& AllClasses();
    int Subclasses(ClassArray& subclasses);
    static int Subclasses(PTree::Node *name, ClassArray& subclasses);
    int ImmediateSubclasses(ClassArray& subclasses);
    static int ImmediateSubclasses(PTree::Node *name, ClassArray& subclasses);
    static int InstancesOf(char* metaclass_name, ClassArray& classes);

    // obsolete
    PTree::Node *NthMemberName(int);
  int IsMember(PTree::Node *);
  bool LookupMemberType(PTree::Node *, TypeInfo&);

// translation

    // these must be consistent with PUBLIC, PRIVATE, PROTECTED in token.h
    enum { Public = 298, Protected = 297, Private = 296, Undefined = 0 };

    virtual void TranslateClass(Environment*);
    void RemoveClass() { removed = true; }
    void AddClassSpecifier(PTree::Node *spec);	// only for MSVC++
    void ChangeName(PTree::Node *name);
  void ChangeBaseClasses(PTree::Node *);
    void RemoveBaseClasses();
    void AppendBaseClass(Class* c, int specifier = Public,
			 bool is_virtual = false);
    void AppendBaseClass(char* class_name, int specifier = Public,
			 bool is_virtual = false);
    void AppendBaseClass(PTree::Node *class_name, int specifier = Public,
			 bool is_virtual = false);

    void ChangeMember(Member& changed_member);
    void AppendMember(Member& added_member, int specifier = Public);
    void AppendMember(PTree::Node *added_member);
    void RemoveMember(Member&);

    virtual void TranslateMemberFunction(Environment*, Member&);

    virtual PTree::Node *TranslateInitializer(Environment*, PTree::Node *var_name,
					PTree::Node *initializer);

    virtual PTree::Node *TranslateNew(Environment*,
				PTree::Node *header, PTree::Node *new_operator,
				PTree::Node *placement, PTree::Node *type_name,
				PTree::Node *arglist);
    virtual PTree::Node *TranslateDelete(Environment*, PTree::Node *op, PTree::Node *obj);
    virtual PTree::Node *TranslateAssign(Environment*, PTree::Node *left,
				   PTree::Node *assign_op, PTree::Node *right);
    virtual PTree::Node *TranslateBinary(Environment*, PTree::Node *left,
				   PTree::Node *binary_op, PTree::Node *right);
    virtual PTree::Node *TranslateUnary(Environment*,
				  PTree::Node *unary_op, PTree::Node *object);
    virtual PTree::Node *TranslateSubscript(Environment*,
				      PTree::Node *object, PTree::Node *index);
    virtual PTree::Node *TranslatePostfix(Environment*,
				    PTree::Node *object, PTree::Node *postfix_op);
    virtual PTree::Node *TranslateFunctionCall(Environment*,
					 PTree::Node *object, PTree::Node *arglist);
    virtual PTree::Node *TranslateMemberCall(Environment*,
				       PTree::Node *object, PTree::Node *access_op,
				       PTree::Node *member_name,
				       PTree::Node *arglist);
    virtual PTree::Node *TranslateMemberCall(Environment*,
				       PTree::Node *member_name, PTree::Node *arglist);
    virtual PTree::Node *TranslateMemberRead(Environment*,
				       PTree::Node *object, PTree::Node *access_op,
				       PTree::Node *member_name);
    virtual PTree::Node *TranslateMemberRead(Environment*, PTree::Node *member_name);
    virtual PTree::Node *TranslateMemberWrite(Environment*,
					PTree::Node *object, PTree::Node *access_op,
					PTree::Node *member_name,
					PTree::Node *assign_op, PTree::Node *expr);
    virtual PTree::Node *TranslateMemberWrite(Environment*,
					PTree::Node *member_name,
					PTree::Node *assign_op, PTree::Node *expr);
    virtual PTree::Node *TranslateUnaryOnMember(Environment*, PTree::Node *unary_op,
					  PTree::Node *object, PTree::Node *access_op,
					  PTree::Node *member_name);
    virtual PTree::Node *TranslateUnaryOnMember(Environment*, PTree::Node *unary_op,
					  PTree::Node *member_name);
    virtual PTree::Node *TranslatePostfixOnMember(Environment*,
					    PTree::Node *object, PTree::Node *access_op,
					    PTree::Node *member_name,
					    PTree::Node *postfix_op);
    virtual PTree::Node *TranslatePostfixOnMember(Environment*,
					    PTree::Node *member_name,
					    PTree::Node *postfix_op);
    virtual PTree::Node *TranslatePointer(Environment*, PTree::Node *var_name);

    virtual PTree::Node *TranslateUserStatement(Environment*,
					  PTree::Node *object, PTree::Node *access_op,
					  PTree::Node *keyword, PTree::Node *rest);
    virtual PTree::Node *TranslateStaticUserStatement(Environment*,
						PTree::Node *keyword, PTree::Node *rest);

    static PTree::Node *StripClassQualifier(PTree::Node *qualified_name);

    PTree::Node *TranslateExpression(Environment*, PTree::Node *expr);
    PTree::Node *TranslateExpression(Environment*, PTree::Node *expr, TypeInfo& type);
    PTree::Node *TranslateStatement(Environment* env, PTree::Node *expr);	// obsolete
    PTree::Node *TranslateNewType(Environment* env, PTree::Node *type);
    PTree::Node *TranslateArguments(Environment*, PTree::Node *arglist);
    PTree::Node *TranslateFunctionBody(Environment*, Member& m, PTree::Node *body);

// others

    Environment* GetEnvironment() { return class_environment; }
    virtual bool AcceptTemplate();
    static bool Initialize();
    static void FinalizeAll(std::ostream& out);
    virtual PTree::Node *FinalizeInstance();
    virtual PTree::Node *Finalize();		// obsolete
    static PTree::Node *FinalizeClass();

    static void RegisterNewModifier(char* keyword);
    static void RegisterNewAccessSpecifier(char* keyword);
    static void RegisterNewMemberModifier(char* keyword);
    static void RegisterNewWhileStatement(char* keyword);
    static void RegisterNewForStatement(char* keyword);
    static void RegisterNewClosureStatement(char* keyword);
    static void RegisterMetaclass(char* keyword, char* class_name);

    static void ChangeDefaultMetaclass(char* name);
    static void SetMetaclassForFunctions(char* name);

    static void InsertBeforeStatement(Environment*, PTree::Node *);
    static void AppendAfterStatement(Environment*, PTree::Node *);
    static void InsertBeforeToplevel(Environment*, Class*);
    static void InsertBeforeToplevel(Environment*, Member&);
    static void InsertBeforeToplevel(Environment*, PTree::Node *);
    static void AppendAfterToplevel(Environment*, Class*);
    static void AppendAfterToplevel(Environment*, Member&);
    static void AppendAfterToplevel(Environment*, PTree::Node *);
    bool InsertDeclaration(Environment*, PTree::Node *declaration);
    bool InsertDeclaration(Environment*, PTree::Node *declaration,
			   PTree::Node *key, void* client_data);
    void* LookupClientData(Environment*, PTree::Node *key);

    void ErrorMessage(Environment*, char* message, PTree::Node *name,
		      PTree::Node *where);
    void WarningMessage(Environment*, char* message, PTree::Node *name,
			PTree::Node *where);
    void ErrorMessage(char* message, PTree::Node *name, PTree::Node *where);
    void WarningMessage(char* message, PTree::Node *name, PTree::Node *where);

    static bool RecordCmdLineOption(char* key, char* value);
    static bool LookupCmdLineOption(char* key);
    static bool LookupCmdLineOption(char* key, char*& value);
    static void WarnObsoleteness(char*, char* = 0);

    static void do_init_static();

private:
    void Construct(Environment*, PTree::Node *);

    void SetEnvironment(Environment*);
    PTree::Node *GetClassSpecifier() { return new_class_specifier; }
    PTree::Node *GetNewName() { return new_class_name; }
    PTree::Node *GetBaseClasses() { return new_base_classes; }
    ChangedMemberList::Cmem* GetChangedMember(PTree::Node *);
    ChangedMemberList* GetAppendedMembers() { return appended_member_list; }
    PTree::Node *GetAppendedCode() { return appended_code; }
    void TranslateClassHasFinished() { done_decl_translation = true; }
    void CheckValidity(char*);

private:
    PTree::Node *definition;
    PTree::Node *full_definition;	// including a user keyword
    Environment* class_environment;
    MemberList* member_list;

    bool done_decl_translation;
    bool removed;
    ChangedMemberList* changed_member_list;
    ChangedMemberList* appended_member_list;
    PTree::Node *appended_code;
    PTree::Node *new_base_classes;
    PTree::Node *new_class_specifier;
    PTree::Node *new_class_name;

    static ClassArray* class_list;

    enum { MaxOptions = 8 };
    static int num_of_cmd_options;
    static char* cmd_options[MaxOptions * 2];

    static char* metaclass_for_c_functions;
    static Class* for_c_functions;

    static PTree::Node *class_t;
    static PTree::Node *empty_block_t;
    static PTree::Node *public_t;
    static PTree::Node *protected_t;
    static PTree::Node *private_t;
    static PTree::Node *virtual_t;
    static PTree::Node *colon_t;
    static PTree::Node *comma_t;
    static PTree::Node *semicolon_t;

friend class Walker;
friend class ClassWalker;
friend class ClassBodyWalker;
friend class Member;
};

class TemplateClass : public Class {
public:
    void InitializeInstance(PTree::Node *def, PTree::Node *margs);
    static bool Initialize();
    const char* MetaclassName();

    PTree::Node *TemplateDefinition() { return template_definition; }
    PTree::Node *TemplateArguments();
    bool AcceptTemplate();
    virtual PTree::Node *TranslateInstantiation(Environment*, PTree::Node *);

private:
    static PTree::Node *GetClassInTemplate(PTree::Node *def);

    PTree::Node *template_definition;
};

class ClassArray : public PTree::LightObject {
public:
    ClassArray(size_t = 16);
    size_t Number() { return num; }
    Class*& operator [] (size_t index) { return Ref(index); }
    Class*& Ref(size_t index);
    void Append(Class*);
    void Clear() { num = 0; }

private:
    size_t num, size;
    Class** array;
};


// not documented class --- internal use only

typedef Class* (*opcxx_MetaclassCreator)(PTree::Node *, PTree::Node *);

class opcxx_ListOfMetaclass {
public:
    opcxx_ListOfMetaclass(const char*, opcxx_MetaclassCreator,
			  bool (*)(), PTree::Node *(*)());
    static Class* New(PTree::Node *, PTree::Node *, PTree::Node *);
    static Class* New(const char*, PTree::Node *, PTree::Node *);
    static void FinalizeAll(std::ostream&);
    static bool AlreadyRecorded(const char*);
    static bool AlreadyRecorded(PTree::Node *);
    static void PrintAllMetaclasses();

private:
    opcxx_ListOfMetaclass* next;
    const char* name;
    opcxx_MetaclassCreator proc;
    PTree::Node *(*finalizer)();	// pointer to FinalizeClass()
    static opcxx_ListOfMetaclass* head;
};

#endif
