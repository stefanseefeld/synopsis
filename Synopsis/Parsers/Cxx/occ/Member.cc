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

#include <cstring>
#include "Member.hh"
#include "Class.hh"
#include "Lexer.hh"
#include "Environment.hh"
#include "Walker.hh"
#include "Encoding.hh"
#include "PTree.hh"

// class Member

Member::Member()
{
    metaobject = 0;
    declarator = 0;
    nth = -1;
    removed = false;
    new_name = 0;
    new_args = 0;
    new_init = 0;
    new_body = 0;
    arg_name_filled = false;

    implementation = 0;
    original_decl = 0;
}

Member::Member(const Member& m)
{
    metaobject = m.metaobject;
    declarator = m.declarator;
    nth = m.nth;
    removed = m.removed;
    new_name = m.new_name;
    new_args = m.new_args;
    new_init = m.new_init;
    new_body = m.new_body;
    arg_name_filled = m.arg_name_filled;

    implementation = m.implementation;
    original_decl = m.original_decl;
}

Member::Member(Class* c, PTree::Node *decl)
{
    metaobject = c;
    declarator = decl;
    nth = -1;
    removed = false;
    new_name = 0;
    new_args = 0;
    new_init = 0;
    new_body = 0;
    arg_name_filled = false;

    implementation = 0;
    original_decl = 0;
}

void Member::Set(Class* c, PTree::Node *decl, int n)
{
    metaobject = c;
    declarator = decl;
    nth = n;
    removed = false;
    new_name = 0;
    new_args = 0;
    new_init = 0;
    new_body = 0;
    arg_name_filled = false;

    implementation = 0;
    original_decl = 0;
}

void Member::Signature(TypeInfo& t) const
{
    if(declarator == 0){
	MopErrorMessage("Member::Signature()", "not initialized object.");
	return;
    }

    char* type = declarator->GetEncodedType();
    if(type == 0)
	t.Unknown();
    else
	t.Set(type, metaobject->GetEnvironment());
}

PTree::Node *Member::Name()
{
    int len;
    char* name = Name(len);
    return Encoding::NameToPtree(name, len);
}

char* Member::Name(int& len)
{
    if(declarator == 0){
	MopErrorMessage("Member::Name()", "not initialized object.");
	return 0;
    }

    char* name = declarator->GetEncodedName();
    if(name != 0){
	Environment* e = metaobject->GetEnvironment();
	name = Encoding::GetBaseName(name, len, e);
    }

    return name;
}

PTree::Node *Member::Comments()
{
    if(declarator == 0){
	MopErrorMessage("Member::Comments()", "not initialized object.");
	return 0;
    }

    if (declarator->IsA(Token::ntDeclarator))
	return ((PTree::Declarator*)declarator)->GetComments();
    else
	return 0;
}

int Member::Nth()
{
    if(Find())
	return nth;
    else
	return -1;
}

Class* Member::Supplier()
{
    if(Find())
	return metaobject->GetMemberList()->Ref(nth)->supplying;
    else
	return 0;
}

bool Member::IsConstructor()
{
    if(declarator == 0){
	MopErrorMessage("Member::IsConstructor()", "not initialized object.");
	return false;
    }

    char* name = declarator->GetEncodedName();
    if(name != 0){
	int len;
	Environment* e = metaobject->GetEnvironment();
	name = Encoding::GetBaseName(name, len, e);
	if(name != 0) {
	    Class* sup = Supplier();
	    if (sup != 0)
		return sup->Name()->Eq(name, len);
	}
    }

    return false;
}

bool Member::IsDestructor()
{
    if(declarator == 0){
	MopErrorMessage("Member::IsDestructor()", "not initialized object.");
	return false;
    }

    char* name = declarator->GetEncodedName();
    if(name != 0){
	int len;
	Environment* e = metaobject->GetEnvironment();
	name = Encoding::GetBaseName(name, len, e);
	if(name != 0)
	    return bool(*name == '~');
    }

    return false;
}

bool Member::IsFunction()
{
    TypeInfo t;
    Signature(t);
    return t.IsFunction();
}

bool Member::IsPublic()
{
    if(Find())
	return bool(metaobject->GetMemberList()->Ref(nth)->access
		    == Token::PUBLIC);
    else
	return false;
}

bool Member::IsProtected()
{
    if(Find())
	return bool(metaobject->GetMemberList()->Ref(nth)->access
		    == Token::PROTECTED);
    else
	return false;
}

bool Member::IsPrivate()
{
    if(Find())
	return bool(metaobject->GetMemberList()->Ref(nth)->access
		    == Token::PRIVATE);
    else
	return false;
}

bool Member::IsStatic()
{
    if(Find())
	return metaobject->GetMemberList()->Ref(nth)->is_static;
    else
	return false;
}

bool Member::IsMutable()
{
    if(Find())
	return metaobject->GetMemberList()->Ref(nth)->is_mutable;
    else
	return false;
}

bool Member::IsInline()
{
    if(Find() && metaobject->GetMemberList()->Ref(nth)->is_inline)
	return true;

    if(IsFunctionImplementation())
	return IsInlineFuncImpl();
    else
	return false;
}

bool Member::IsInlineFuncImpl()
{
    PTree::Node *header = implementation->Car();
    while(header != 0){
	PTree::Node *h = header->Car();
	if(h->IsA(Token::INLINE))
	    return true;

	header = header->Cdr();
    }

    return false;
}

bool Member::IsVirtual()
{
    if(Find())
	return metaobject->GetMemberList()->Ref(nth)->is_virtual;
    else
	return false;
}

bool Member::IsPureVirtual()
{
    if(IsFunction())
	return declarator->Last()->Car()->Eq('0');
    else
	return false;
}

PTree::Node *Member::GetUserAccessSpecifier()
{
    if(Find())
	return metaobject->GetMemberList()->Ref(nth)->user_access;
    else
	return 0;
}

bool Member::GetUserArgumentModifiers(PTree::Array& mods)
{
    PTree::Node *args;

    mods.Clear();
    if(!Find())
	return false;

    if(!Walker::GetArgDeclList((PTree::Declarator*)declarator, args))
	return false;

    while(args != 0){
	PTree::Node *a = args->Car();
	if(!a->IsLeaf() && a->Car()->IsA(Token::ntUserdefKeyword))
	    mods.Append(a->Car());
	else
	    mods.Append(0);
	
	args = PTree::Node::ListTail(args, 2);	// skip ,
    }

    return true;
}

PTree::Node *Member::GetUserMemberModifier()
{
    if(Find())
	return metaobject->GetMemberList()->Ref(nth)->user_mod;
    else
	return 0;
}

bool Member::Find()
{
    if(nth >= 0)
	return true;
    else if(metaobject == 0 || declarator == 0)
	return false;

    MemberList* mlist = metaobject->GetMemberList();

    int len;
    char* name = Name(len);
    char* sig = declarator->GetEncodedType();
    if(mlist == 0 || name == 0 || sig == 0)
	return false;

    nth = mlist->Lookup(name, len, sig);
    if(nth >= 0){
	MemberList::Mem* m = mlist->Ref(nth);
	metaobject = m->supplying;
	declarator = m->declarator;
	return true;
    }
    else
	return false;
}

void Member::SetQualifiedName(PTree::Node *name)
{
    new_name = name;
}

void Member::SetName(PTree::Node *name)
{
    if(IsFunctionImplementation())
	SetName(name, original_decl);
    else
	SetName(name, declarator);
}

void Member::SetName(PTree::Node *name, PTree::Node *decl)
{
    if(decl == 0){
	MopErrorMessage("Member::SetName()", "not initialized object.");
	return;
    }

    char* encoded = decl->GetEncodedName();
    if(encoded != 0 && *encoded == 'Q'){
	PTree::Node *qname = ((PTree::Declarator*)decl)->Name();
	PTree::Node *old_name = qname->Last()->First();
	new_name = PTree::Node::ShallowSubst(name, old_name, qname);
    }
    else
	new_name = name;
}

PTree::Node *Member::ArgumentList()
{
    if(IsFunctionImplementation())
	return ArgumentList(original_decl);
    else
	return ArgumentList(declarator);
}

PTree::Node *Member::ArgumentList(PTree::Node *decl)
{
    PTree::Node *args;
    if(Walker::GetArgDeclList((PTree::Declarator*)decl, args))
	return args;
    else
	return 0;
}

void Member::SetArgumentList(PTree::Node *args)
{
    new_args = args;
}

PTree::Node *Member::MemberInitializers()
{
    if(IsFunctionImplementation())
	return MemberInitializers(original_decl);
    else
	return MemberInitializers(declarator);
}

PTree::Node *Member::MemberInitializers(PTree::Node *decl)
{
    if(IsConstructor()){
	PTree::Node *init = decl->Last()->Car();
	if(!init->IsLeaf() && init->Car()->Eq(':'))
	    return init;
    }

    return 0;
}

void Member::SetMemberInitializers(PTree::Node *init)
{
    new_init = init;
}

PTree::Node *Member::FunctionBody()
{
    if(IsFunctionImplementation())
	return implementation->Nth(3);
    else if(Find()){
	PTree::Node *def = metaobject->GetMemberList()->Ref(nth)->definition;
	PTree::Node *decls = def->Third();
	if(decls->IsA(Token::ntDeclarator))
	    return def->Nth(3);
    }

    return 0;
}

void Member::SetFunctionBody(PTree::Node *body)
{
    new_body = body;
}

PTree::Node *Member::Arguments()
{
    return Arguments(ArgumentList(), 0);
}

PTree::Node *Member::Arguments(PTree::Node *args, int i)
{
    PTree::Node *rest;

    if(args == 0)
	return args;

    if(args->Cdr() == 0)
	rest = 0;
    else{
	rest = Arguments(args->Cddr(), i + 1);	// skip ","
	rest = PTree::Node::Cons(args->Cadr(), rest);
    }

    PTree::Node *a = args->Car();
    PTree::Node *p;
    if(a->IsLeaf())
	p = a;
    else{
	if(a->Car()->IsA(Token::ntUserdefKeyword, Token::REGISTER))
	    p = a->Third();
	else
	    p = a->Second();

	p = ((PTree::Declarator*)p)->Name();
    }

    if(p == 0){
	arg_name_filled = true;
	p = PTree::Node::Make(Walker::argument_name, i);
    }

    return PTree::Node::Cons(p, rest);
}

void Member::Copy(Member* mem, void* ptr)
{
    ChangedMemberList::Cmem* cmem = (ChangedMemberList::Cmem*)ptr;
    ChangedMemberList::Copy(mem, cmem, Class::Undefined);
    if(mem->IsFunctionImplementation()){
	cmem->declarator = mem->original_decl;
	cmem->def = mem->implementation;
    }
}


// class MemberFunction

MemberFunction::MemberFunction(Class* c, PTree::Node *impl, PTree::Node *decl)
: Member(c, decl)
{
    implementation = impl;
    original_decl = decl;
}


// class MemberList

MemberList::MemberList()
{
    this_class = 0;
    num = 0;
    size = -1;
    array = 0;
}

MemberList::Mem* MemberList::Ref(int i)
{
    const int unit = 16;
    if(i >= size){
	int old_size = size;
	size = ((unsigned int)i + unit) & ~(unit - 1);
	Mem* a = new (GC) Mem[size];
	if(old_size > 0)
	    memmove(a, array, old_size * sizeof(Mem));

	array = a;
    }

    return &array[i];
}

void MemberList::Make(Class* metaobject)
{
    this_class = metaobject;
    num = 0;

    AppendThisClass(metaobject);

    Environment* env = metaobject->GetEnvironment();
    PTree::Node *bases = metaobject->BaseClasses();
    while(bases != 0){
	bases = bases->Cdr();		// skip : or ,
	if(bases != 0){
	    AppendBaseClass(env, bases->Car());
	    bases = bases->Cdr();
	}
    }
}

void MemberList::AppendThisClass(Class* metaobject)
{
    int access = Token::PRIVATE;
    PTree::Node *user_access = 0;
    PTree::Node *members = metaobject->Members();
    while(members != 0){
	PTree::Node *def = members->Car();
	if(def->IsA(Token::ntDeclaration)){
	    PTree::Node *decl;
	    int nth = 0;
	    do{
		int i = nth++;
		decl = Walker::NthDeclarator(def, i);
		if(decl != 0)
		    Append(def, decl, access, user_access);
	    } while(decl != 0);
	}
	else if(def->IsA(Token::ntAccessSpec)){
	    access = def->Car()->What();
	    user_access = 0;
	}
	else if(def->IsA(Token::ntUserAccessSpec))
	    user_access = def;
	else if(def->IsA(Token::ntAccessDecl))
	    /* not implemented */;

	members = members->Cdr();
    }
}

void MemberList::Append(PTree::Node *declaration, PTree::Node *decl,
			int access, PTree::Node *user_access)
{
    int len;
    Mem mem;
    char* name = decl->GetEncodedName();
    char* signature = decl->GetEncodedType();
    Environment* e = this_class->GetEnvironment();
    name = Encoding::GetBaseName(name, len, e);

    CheckHeader(declaration, &mem);

    Mem* m = Ref(num++);
    m->supplying = this_class;
    m->definition = declaration;
    m->declarator = decl;
    m->name = name;
    m->signature = signature;
    m->is_constructor = bool(this_class->Name()->Eq(name, len));
    m->is_destructor = bool(*name == '~');
    m->is_virtual = mem.is_virtual;
    m->is_static = mem.is_static;
    m->is_mutable = mem.is_mutable;
    m->is_inline = mem.is_inline;
    m->user_mod = mem.user_mod;
    m->access = access;
    m->user_access = user_access;
}

void MemberList::AppendBaseClass(Environment* env, PTree::Node *base_class)
{
    int access = Token::PRIVATE;
    while(base_class->Cdr() != 0){
	PTree::Node *p = base_class->Car();
	if(p->IsA(Token::PUBLIC, Token::PROTECTED, Token::PRIVATE))
	    access = p->What();

	base_class = base_class->Cdr();
    }	

    Class* metaobject = env->LookupClassMetaobject(base_class->Car());
    if(metaobject == 0)
	return;

    MemberList* mlist = metaobject->GetMemberList();
    for(int i = 0; i < mlist->num; ++i){
	Mem* m = &mlist->array[i];
	Mem* m2 = Lookup(m->name, m->signature);
	if(m2 != 0){				// overwrittten
	    if(!m2->is_virtual)
		m2->is_virtual = m->is_virtual;
	}
	else if(m->access != Token::PRIVATE){		// inherited
	    m2 = Ref(num++);
	    *m2 = *m;
	    if(access == Token::PRIVATE)
		m2->access = Token::PRIVATE;
	    else if(access == Token::PROTECTED)
		m2->access = Token::PROTECTED;
	}
    }
}

MemberList::Mem* MemberList::Lookup(char* name, char* signature)
{
    for(int i = 0; i < num; ++i){
	Mem* m = Ref(i);
	if(strcmp(m->name, name) == 0 && strcmp(m->signature, signature) == 0)
	    return m;
    }

    return 0;
}

int MemberList::Lookup(char* name, int len, char* signature)
{
    for(int i = 0; i < num; ++i){
	Mem* m = Ref(i);
	if(strcmp(m->signature, signature) == 0
	   && strncmp(m->name, name, len) == 0 && m->name[len] == '\0')
	    return i;
    }

    return -1;
}

int MemberList::Lookup(Environment* env, PTree::Node *member, int index)
{
    char* name;
    int len;

    if(member == 0)
	return -1;
    else if(member->IsLeaf()){
	name = member->GetPosition();
	len = member->GetLength();
    }
    else
	name = Encoding::GetBaseName(member->GetEncodedName(), len, env);

    for(int i = 0; i < num; ++i){
	Mem* m = Ref(i);
	if(strncmp(m->name, name, len) == 0 && m->name[len] == '\0')
	    if(index-- <= 0)
		return i;
    }

    return -1;
}

int MemberList::Lookup(Environment*, char* name, int index)
{
    if(name == 0)
	return -1;

    for(int i = 0; i < num; ++i){
	Mem* m = Ref(i);
	if(strcmp(m->name, name) == 0)
	    if(index-- <= 0)
		return i;
    }

    return -1;
}

void MemberList::CheckHeader(PTree::Node *declaration, Mem* m)
{
    m->is_virtual = false;
    m->is_static = false;
    m->is_mutable = false;
    m->is_inline = false;
    m->user_mod = 0;

    PTree::Node *header = declaration->Car();
    while(header != 0){
	PTree::Node *h = header->Car();
	if(h->IsA(Token::VIRTUAL))
	    m->is_virtual = true;
	else if(h->IsA(Token::STATIC))
	    m->is_static = true;
	else if(h->IsA(Token::MUTABLE))
	    m->is_mutable = true;
	else if(h->IsA(Token::INLINE))
	    m->is_inline = true;
	else if(h->IsA(Token::ntUserdefKeyword))
	    m->user_mod = h;

	header = header->Cdr();
    }

    PTree::Node *d = declaration->Third();
    if(d != 0 && d->IsA(Token::ntDeclarator))
	m->is_inline = true;
}

// class ChangedMemberList

ChangedMemberList::ChangedMemberList()
{
    num = 0;
    size = -1;
    array = 0;
}

void ChangedMemberList::Append(Member* m, int access)
{
    Cmem* mem = Ref(num++);
    Copy(m, mem, access);
}

void ChangedMemberList::Copy(Member* src, Cmem* dest, int access)
{
    dest->declarator = src->declarator;
    dest->removed = src->removed;
    dest->name = src->new_name;
    dest->args = src->new_args;
    dest->init = src->new_init;
    dest->body = src->new_body;
    dest->arg_name_filled = src->arg_name_filled;

    if(src->Find()){
	MemberList::Mem* m = src->metaobject->GetMemberList()->Ref(src->nth);
	dest->def = m->definition;
	if(access == Class::Undefined)
	    dest->access = m->access;
	else
	    dest->access = access;
    }
    else{
	dest->def = 0;
	if(access == Class::Undefined)
	    dest->access = Class::Public;
	else
	    dest->access = access;
    }
}

ChangedMemberList::Cmem* ChangedMemberList::Lookup(PTree::Node *decl)
{
    for(int i = 0; i < num; ++i){
	Cmem* m = Ref(i);
	if(m->declarator == decl)
	    return m;
    }

    return 0;
}

ChangedMemberList::Cmem* ChangedMemberList::Get(int i)
{
    if(i >= num)
	return 0;
    else
	return Ref(i);
}

ChangedMemberList::Cmem* ChangedMemberList::Ref(int i)
{
    const int unit = 16;
    if(i >= size){
	int old_size = size;
	size = ((unsigned int)i + unit) & ~(unit - 1);
	Cmem* a = new (GC) Cmem[size];
	if(old_size > 0)
	    memmove(a, array, old_size * sizeof(Cmem));

	array = a;
    }

    return &array[i];
}
