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

#include <PTree.hh>
#include "Member.hh"
#include "Class.hh"
#include "Lexer.hh"
#include "Environment.hh"
#include "Walker.hh"

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
  if(!declarator) throw std::runtime_error("Member::Signature(): not initialized object.");

  PTree::Encoding type = declarator->encoded_type();
  if(type.empty()) t.unknown();
  else t.set(type, metaobject->GetEnvironment());
}

PTree::Node *Member::Name()
{
  PTree::Encoding name = encoded_name();
  return name.name_to_ptree();
}

PTree::Encoding Member::encoded_name()
{
  if(!declarator) throw std::runtime_error("Member::encoded_name(), not initialized object.");

  PTree::Encoding name = declarator->encoded_name();
  if(!name.empty())
  {
    Environment *e = metaobject->GetEnvironment();
    name = Environment::get_base_name(name, e);
  }
  return name;
}

PTree::Node *Member::Comments()
{
  if(!declarator)
    throw std::runtime_error("Member::Comments(): not initialized object.");

  if (PTree::is_a(declarator, Token::ntDeclarator))
    return ((PTree::Declarator*)declarator)->get_comments();
  else return 0;
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
  if(!declarator) throw std::runtime_error("Member::IsConstructor(): not initialized object.");

  PTree::Encoding name = declarator->encoded_name();
  if(!name.empty())
  {
    Environment *e = metaobject->GetEnvironment();
    name = Environment::get_base_name(name, e);
    if(!name.empty())
    {
      Class *sup = Supplier();
      if (sup)
	return sup->Name() && PTree::equal(*sup->Name(),
					   (const char *)&*name.begin(),
					   name.size());
    }
  }
  return false;
}

bool Member::IsDestructor()
{
  if(!declarator) throw std::runtime_error("Member::IsDestructor(): not initialized object.");

  PTree::Encoding name = declarator->encoded_name();
  if(!name.empty())
  {
    Environment *e = metaobject->GetEnvironment();
    name = Environment::get_base_name(name, e);
    if(!name.empty()) return name.front() == '~';
  }
  return false;
}

bool Member::IsFunction()
{
  TypeInfo t;
  Signature(t);
  return t.is_function();
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
    PTree::Node *header = implementation->car();
    while(header != 0){
	PTree::Node *h = header->car();
	if(PTree::is_a(h, Token::INLINE))
	    return true;

	header = header->cdr();
    }

    return false;
}

bool Member::IsVirtual()
{
  if(Find()) return metaobject->GetMemberList()->Ref(nth)->is_virtual;
  else return false;
}

bool Member::IsPureVirtual()
{
  if(IsFunction()) return *PTree::last(declarator)->car() == '0';
  else return false;
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

    mods.clear();
    if(!Find())
	return false;

    if(!Walker::GetArgDeclList((PTree::Declarator*)declarator, args))
	return false;

    while(args != 0){
	PTree::Node *a = args->car();
	if(!a->is_atom() && PTree::is_a(a->car(), Token::ntUserdefKeyword))
	    mods.append(a->car());
	else
	    mods.append(0);
	
	args = PTree::tail(args, 2);	// skip ,
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
  if(nth >= 0) return true;
  else if(!metaobject || !declarator) return false;

  MemberList *mlist = metaobject->GetMemberList();

  PTree::Encoding name = encoded_name();
  PTree::Encoding type = declarator->encoded_type();
  if(!mlist || name.empty() || type.empty()) return false;

  nth = mlist->Lookup((const char *)&*name.begin(), name.size(),
		      (const char *)&*type.begin());
  if(nth >= 0)
  {
    MemberList::Mem *m = mlist->Ref(nth);
    metaobject = m->supplying;
    declarator = m->declarator;
    return true;
  }
  else return false;
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
  if(!decl) throw std::runtime_error("Member::SetName(): not initialized object.");
  
  PTree::Encoding encoded = decl->encoded_name();
  if(!encoded.empty() && encoded.front() == 'Q')
  {
    PTree::Node *qname = ((PTree::Declarator*)decl)->name();
    PTree::Node *old_name = PTree::first(PTree::last(qname));
    new_name = PTree::shallow_subst(name, old_name, qname);
  }
  else new_name = name;
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
	PTree::Node *init = PTree::last(decl)->car();
	if(!init->is_atom() && *init->car() == ':')
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
      return PTree::nth(implementation, 3);
    else if(Find()){
	PTree::Node *def = metaobject->GetMemberList()->Ref(nth)->definition;
	PTree::Node *decls = PTree::third(def);
	if(PTree::is_a(decls, Token::ntDeclarator))
	  return PTree::nth(def, 3);
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

    if(args->cdr() == 0)
	rest = 0;
    else{
	rest = Arguments(PTree::cddr(args), i + 1);	// skip ","
	rest = PTree::cons(PTree::cadr(args), rest);
    }

    PTree::Node *a = args->car();
    PTree::Node *p;
    if(a->is_atom())
	p = a;
    else{
      if(PTree::is_a(a->car(), Token::ntUserdefKeyword, Token::REGISTER))
	    p = PTree::third(a);
	else
	    p = PTree::second(a);

	p = ((PTree::Declarator*)p)->name();
    }

    if(p == 0){
	arg_name_filled = true;
	p = PTree::make(Walker::argument_name, i);
    }

    return PTree::cons(p, rest);
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
	Mem* a = new (PTree::GC) Mem[size];
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
	bases = bases->cdr();		// skip : or ,
	if(bases != 0){
	    AppendBaseClass(env, bases->car());
	    bases = bases->cdr();
	}
    }
}

void MemberList::AppendThisClass(Class* metaobject)
{
    int access = Token::PRIVATE;
    PTree::Node *user_access = 0;
    PTree::Node *members = metaobject->Members();
    while(members != 0){
	PTree::Node *def = members->car();
	if(PTree::is_a(def, Token::ntDeclaration)){
	    PTree::Node *decl;
	    int nth = 0;
	    do{
		int i = nth++;
		decl = Walker::NthDeclarator(def, i);
		if(decl != 0)
		    Append(def, decl, access, user_access);
	    } while(decl != 0);
	}
	else if(PTree::is_a(def, Token::ntAccessSpec)){
	    access = PTree::type_of(def->car());
	    user_access = 0;
	}
	else if(PTree::is_a(def, Token::ntUserAccessSpec))
	    user_access = def;
	else if(PTree::is_a(def, Token::ntAccessDecl))
	    /* not implemented */;

	members = members->cdr();
    }
}

void MemberList::Append(PTree::Node *declaration, PTree::Node *decl,
			int access, PTree::Node *user_access)
{
    int len;
    Mem mem;
    PTree::Encoding name = decl->encoded_name();
    PTree::Encoding type = decl->encoded_type();
    Environment *e = this_class->GetEnvironment();
    name = Environment::get_base_name(name, e);

    CheckHeader(declaration, &mem);

    Mem *m = Ref(num++);
    m->supplying = this_class;
    m->definition = declaration;
    m->declarator = decl;
    m->name = name;
    m->type = type;
    m->is_constructor = PTree::equal(*this_class->Name(),
				     (const char *)&*name.begin(), name.size());
    m->is_destructor = name.front() == '~';
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
    while(base_class->cdr() != 0){
	PTree::Node *p = base_class->car();
	if(PTree::is_a(p, Token::PUBLIC, Token::PROTECTED, Token::PRIVATE))
          access = PTree::type_of(p);

	base_class = base_class->cdr();
    }	

    Class* metaobject = env->LookupClassMetaobject(base_class->car());
    if(metaobject == 0)
	return;

    MemberList* mlist = metaobject->GetMemberList();
    for(int i = 0; i < mlist->num; ++i){
	Mem* m = &mlist->array[i];
	Mem* m2 = Lookup((const char *)&*m->name.begin(),
			 (const char *)&*m->type.begin());
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

MemberList::Mem* MemberList::Lookup(const char *name, const char *type)
{
  for(int i = 0; i < num; ++i)
  {
    Mem* m = Ref(i);
    if(m->name == name && m->type == type) return m;
  }
  return 0;
}

int MemberList::Lookup(const char *name, int len, const char *type)
{
  for(int i = 0; i < num; ++i)
  {
    Mem* m = Ref(i);
    if(m->type == type &&
       m->name.size() == len &&
       strncmp((const char *)&*m->name.begin(), name, len) == 0)
      return i;
  }
  return -1;
}

int MemberList::Lookup(Environment* env, PTree::Node *member, int index)
{
  std::string name;

  if(!member) return -1;
  else if(member->is_atom())
  {
    name = std::string(member->position(), member->length());
  }
  else
  {
    PTree::Encoding enc = member->encoded_name();
    enc = Environment::get_base_name(enc, env);
    name = std::string(enc.begin(), enc.end());
  }

  for(int i = 0; i < num; ++i)
  {
    Mem* m = Ref(i);
    if(m->name == name)
      if(index-- <= 0)
	return i;
  }
  return -1;
}

int MemberList::Lookup(Environment*, const char* name, int index)
{
  if(!name) return -1;
  for(int i = 0; i < num; ++i)
  {
    Mem *m = Ref(i);
    if(m->name == name)
      if(index-- <= 0) return i;
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

  PTree::Node *header = declaration->car();
  while(header)
  {
    PTree::Node *h = header->car();
    if(PTree::is_a(h, Token::VIRTUAL)) m->is_virtual = true;
    else if(PTree::is_a(h, Token::STATIC)) m->is_static = true;
    else if(PTree::is_a(h, Token::MUTABLE)) m->is_mutable = true;
    else if(PTree::is_a(h, Token::INLINE)) m->is_inline = true;
    else if(PTree::is_a(h, Token::ntUserdefKeyword)) m->user_mod = h;
    header = header->cdr();
  }
  
  PTree::Node *d = PTree::third(declaration);
  if(d != 0 && PTree::is_a(d, Token::ntDeclarator)) m->is_inline = true;
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
  
  if(src->Find())
  {
    MemberList::Mem* m = src->metaobject->GetMemberList()->Ref(src->nth);
    dest->def = m->definition;
    if(access == Class::Undefined)
      dest->access = m->access;
    else
      dest->access = access;
  }
  else
  {
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
	Cmem* a = new (PTree::GC) Cmem[size];
	if(old_size > 0)
	    memmove(a, array, old_size * sizeof(Cmem));

	array = a;
    }

    return &array[i];
}
