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

#ifndef _Member_hh
#define _Member_hh

#include <PTree.hh>

class Class;
class Environment;
class TypeInfo;

class Member : public PTree::LightObject {
public:
    Member();
    Member(const Member&);
    Member(Class*, PTree::Node *);
    void Set(Class*, PTree::Node *, int);

    void Signature(TypeInfo& t) const;
    PTree::Node *Name();
    PTree::Node *Comments();
    int Nth();
    Class* Supplier();
    bool IsConstructor();
    bool IsDestructor();
    bool IsFunction();
    bool IsPublic();
    bool IsProtected();
    bool IsPrivate();
    bool IsStatic();
    bool IsMutable();
    bool IsInline();
    bool IsVirtual();
    bool IsPureVirtual();

    PTree::Node *GetUserMemberModifier();
    PTree::Node *GetUserAccessSpecifier();
  bool GetUserArgumentModifiers(PTree::Array& result);

    void Remove() { removed = true; }
    void SetName(PTree::Node *);
    void SetQualifiedName(PTree::Node *);
    PTree::Node *NewName() { return new_name; }
    PTree::Node *ArgumentList();
    void SetArgumentList(PTree::Node *);
    PTree::Node *NewArgumentList() { return new_args; }
    PTree::Node *MemberInitializers();
    void SetMemberInitializers(PTree::Node *);
    PTree::Node *NewMemberInitializers() { return new_init; }
    PTree::Node *FunctionBody();
    void SetFunctionBody(PTree::Node *);
    PTree::Node *NewFunctionBody() { return new_body; }

    PTree::Node *Arguments();

    static void Copy(Member*, void* /* ChangedMemberList::Cmem* */);

protected:
    bool IsInlineFuncImpl();
    void SetName(PTree::Node *, PTree::Node *);
    PTree::Node *ArgumentList(PTree::Node *decl);
    PTree::Node *Arguments(PTree::Node *, int);
    PTree::Node *MemberInitializers(PTree::Node *decl);

private:
    const char* Name(int&);
    bool Find();

protected:

    // The next function is true if the member is a function
    // with the implementation but it is not inlined.  And if so,
    // the following variables are effective.

    bool IsFunctionImplementation() { return bool(implementation != 0);}
  PTree::Node *implementation;
    PTree::Node *original_decl;

private:
    bool removed;
    PTree::Node *new_name;
    PTree::Node *new_args;
    PTree::Node *new_init;
    PTree::Node *new_body;
    bool arg_name_filled;

    Class* metaobject;
    PTree::Node *declarator;
    int nth;

friend class ChangedMemberList;
};

class MemberFunction : public Member {
public:
    MemberFunction(Class*, PTree::Node *, PTree::Node *);
};

class MemberList : public PTree::LightObject {
public:
    struct Mem {
	Class* supplying;
	PTree::Node *definition;
	PTree::Node *declarator;
	const char* name;
	const char* signature;
	bool is_constructor, is_destructor;
	bool is_virtual, is_static, is_mutable, is_inline;
	int  access;
	PTree::Node *user_access;
	PTree::Node *user_mod;
    };

    MemberList();
    void Make(Class*);
    Mem* Ref(int);
    int Number() { return num; }
    Mem* Lookup(const char*, const char*);
    int Lookup(const char*, int, const char*);
    int Lookup(Environment*, PTree::Node *, int);
    int Lookup(Environment*, const char*, int);

private:
    void AppendThisClass(Class*);
    void Append(PTree::Node *, PTree::Node *, int, PTree::Node *);
    void AppendBaseClass(Environment*, PTree::Node *);
    void CheckHeader(PTree::Node *, Mem*);

    Class* this_class;
    int num;

    int size;
    Mem* array;
};

class ChangedMemberList : public PTree::LightObject {
public:
    struct Cmem {
	PTree::Node *declarator;
	bool removed;
	PTree::Node *name;
	PTree::Node *args;
	PTree::Node *init;
	PTree::Node *body;
	PTree::Node *def;
	int    access;		// used only by Classs::appended_member_list
	bool arg_name_filled;
    };

    ChangedMemberList();
    void Append(Member*, int access);
    static void Copy(Member* src, Cmem* dest, int access);
    Cmem* Lookup(PTree::Node *decl);
    Cmem* Get(int);

private:
    Cmem* Ref(int);

private:
    int num;
    int size;
    Cmem* array;
};

#endif
