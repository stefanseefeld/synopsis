//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "Environment.hh"
#include "HashTable.hh"
#include <Synopsis/PTree.hh>
#include "Synopsis/Lexer.hh"
#include "occ/Walker.hh"
#include "TypeInfo.hh"
#include "Class.hh"
#include "Synopsis/Parser.hh"
#include <iostream>
#include <cassert>

using namespace Synopsis;

// class Environment

PTree::Array* Environment::classkeywords = 0;
HashTable* Environment::namespace_table = 0;

void Environment::do_init_static()
{
    namespace_table = new HashTable;
}

Environment::Environment(Walker* w)
: baseclasses(0)
{
    htable = new BigHashTable;		// base environment
    next = 0;
    metaobject = 0;
    walker = w;
}

Environment::Environment(Environment* e)
: baseclasses(0)
{
    htable = new HashTable;
    next = e;
    metaobject = 0;
    walker = e->walker;
}

Environment::Environment(Environment* e, Walker* w)
: baseclasses(0)
{
    htable = new HashTable;
    next = e;
    metaobject = 0;
    walker = w;
}

bool Environment::IsEmpty()
{
    return bool(htable->IsEmpty());
}

Environment* Environment::GetBottom()
{
    Environment* p;
    for(p = this; p->next != 0; p = p->next)
	;

    return p;
}

Class* Environment::LookupClassMetaobject(PTree::Node *name)
{
  TypeInfo tinfo;
  Bind* bind = 0;

  assert(this);

  if(name == 0) return 0;
  else if(name->is_atom())
  {
    if(LookupType(PTree::Encoding(name->position(), name->length()), bind))
      if(bind)
      {
	bind->GetType(tinfo, this);
	return tinfo.class_metaobject();
      }
    return 0;
  }
  else
  {
    Environment *e = this;
    PTree::Encoding base = Environment::get_base_name(name->encoded_name(), e);
    if(!base.empty() && e)
      if(LookupType(base, bind))
	if(bind)
	{
	  bind->GetType(tinfo, this);
	  return tinfo.class_metaobject();
	}
    return 0;
  }
}

bool Environment::LookupType(const PTree::Encoding &name, Bind*& t)
{
    Environment* p;

    for(p = this; p != 0; p = p->next){
	int count = 0;
	while(p->htable->LookupEntries((const char *)&*name.begin(), name.size(),
				       (HashValue*)&t, count))
	    if(t != 0)
		switch(t->What()){
		case Bind::isVarName :
		case Bind::isTemplateFunction :
		    break;
		default :
		    return true;
		}

	size_t n = p->baseclasses.Number();
	for(size_t i = 0; i < n; ++i)
	    if(p->baseclasses.Ref(i)->LookupType(name, t))
		return true;
    }

    return false;
}

bool Environment::Lookup(PTree::Node *name, TypeInfo& t)
{
    Bind* bind;

    if(Lookup(name, bind) && bind != 0){
	bind->GetType(t, this);
	return true;
    }
    else{
	t.unknown();
	return false;
    }
}

bool Environment::Lookup(PTree::Node *name, bool& is_type_name, TypeInfo& t)
{
    Bind* bind;

    if(Lookup(name, bind) && bind != 0){
	is_type_name = bind->IsType();
	bind->GetType(t, this);
	return true;
    }
    else{
	t.unknown();
	return false;
    }
}

bool Environment::Lookup(PTree::Node *name, Bind*& bind)
{
  bind = 0;
  assert(this);
  if(name == 0) return false;
  else if(name->is_atom())
    return LookupAll(PTree::Encoding(name->position(), name->length()), bind);
  else
  {
    PTree::Encoding encoding = name->encoded_name();
    if(encoding.empty()) return false;
    else
    {
      Environment* e = this;
      PTree::Encoding base = Environment::get_base_name(encoding, e);
      if(!base.empty() && e) return e->LookupAll(base, bind);
      else return false;
    }
  }
}

bool Environment::LookupTop(PTree::Node *name, Bind*& bind)
{
  bind = 0;
  assert(this);
  if(name == 0) return false;
  else if(name->is_atom())
    return LookupTop(PTree::Encoding(name->position(), name->length()), bind);
  else
  {
    PTree::Encoding encoding = name->encoded_name();
    if(encoding.empty()) return false;
    else
    {
      Environment* e = this;
      PTree::Encoding base = Environment::get_base_name(encoding, e);
      if(!base.empty() && e) return e->LookupTop(base, bind);
      else return false;
    }
  }
}

bool Environment::LookupTop(const PTree::Encoding &name, Bind*& t)
{
  if(htable->Lookup((const char*)&*name.begin(), name.size(), (HashValue*)&t))
    return true;
    else{
	size_t n = baseclasses.Number();
	for(size_t i = 0; i < n; ++i)
	    if(baseclasses.Ref(i)->LookupTop(name, t))
		return true;

	return false;
    }
}

bool Environment::LookupAll(const PTree::Encoding &name, Bind*& t)
{
    Environment* p;

    for(p = this; p != 0; p = p->next)
      if(p->htable->Lookup((const char*)&*name.begin(), name.size(), (HashValue*)&t))
	    return true;
	else{
	    size_t n = p->baseclasses.Number();
	    for(size_t i = 0; i < n; ++i)
		if(p->baseclasses.Ref(i)->LookupAll(name, t))
		    return true;
	}

    return false;
}

bool Environment::RecordVariable(const PTree::Encoding &name, Class* c)
{
  PTree::Encoding encoding;
  encoding.simple_name(c->Name());
  return htable->AddEntry((const char *)&*name.begin(), name.size(),
			  new BindVarName(encoding.copy())) >= 0;
}

bool Environment::RecordPointerVariable(const PTree::Encoding &name, Class* c)
{
  PTree::Encoding encoding;
  encoding.simple_name(c->Name());
  encoding.ptr_operator('*');
  return htable->AddEntry((const char *)&*name.begin(),
			  new BindVarName(encoding.copy())) >= 0;
}

int Environment::AddEntry(const PTree::Encoding &name, Bind* b) 
{
  return htable->AddEntry((const char *)&*name.begin(), name.size(), b);
}

int Environment::AddDupEntry(const PTree::Encoding &name, Bind* b) 
{
  return htable->AddDupEntry((const char *)&*name.begin(), name.size(), b);
}

void Environment::RecordNamespace(PTree::Node *name)
{
  if (name)
    namespace_table->AddEntry(name->position(), name->length(),
			      name);
}

bool Environment::LookupNamespace(const PTree::Encoding &name)
{
  HashValue value;
  return namespace_table->Lookup((const char *)&*name.begin(), name.size(), &value);
}

void Environment::RecordTypedefName(PTree::Node *decls)
{
  while(decls)
  {
    PTree::Node *d = decls->car();
    if(PTree::type_of(d) == Token::ntDeclarator)
    {
      PTree::Encoding name = d->encoded_name();
      PTree::Encoding type = d->encoded_type();
      if(!name.empty() && !type.empty())
      {
	Environment* e = this;
	PTree::Encoding base = Environment::get_base_name(name, e);
	if(!base.empty()) AddEntry(base, new BindTypedefName(type));
      }
    }
    decls = PTree::tail(decls, 2);
  }
}

void Environment::RecordEnumName(PTree::Node *spec)
{
  PTree::Node *tag = PTree::second(spec);
  PTree::Encoding encoding = spec->encoded_name();
  if(tag && tag->is_atom())
    AddEntry(PTree::Encoding(tag->position(), tag->length()),
	     new BindEnumName(encoding.copy(), spec));
  else
  {
    Environment *e = this;
    PTree::Encoding base = Environment::get_base_name(encoding, e);
    if(!base.empty() && e) e->AddEntry(base, new BindEnumName(encoding, spec));
  }
}

void Environment::RecordClassName(const PTree::Encoding &name, Class* metaobject)
{
  Bind* bind;
  Environment *e = this;
  PTree::Encoding base = Environment::get_base_name(name, e);
  if(base.empty() || !e) return; // error?
  if(e->LookupAll(base, bind))
    if(bind && bind->What() == Bind::isClassName)
    {
      if(metaobject) bind->SetClassMetaobject(metaobject);
      return;
    }
  e->AddEntry(base, new BindClassName(metaobject));
}

void Environment::RecordTemplateClass(PTree::Node *spec, Class* metaobject)
{
  Bind *bind;
  Environment *e = this;
  PTree::Encoding name = Environment::get_base_name(spec->encoded_name(), e);
  if(name.empty() || !e) return; // error?

  if(e->LookupAll(name, bind))
    if(bind && bind->What() == Bind::isTemplateClass)
    {
      if(metaobject) bind->SetClassMetaobject(metaobject);
      return;
    }
  e->AddEntry(name, new BindTemplateClass(metaobject));
}

Environment* Environment::RecordTemplateFunction(PTree::Node *def, PTree::Node *body)
{
  PTree::Node *decl = PTree::third(body);
  if(PTree::is_a(decl, Token::ntDeclarator))
  {
    PTree::Encoding name = decl->encoded_name();
    if(!name.empty())
    {
      Environment *e = this;
      PTree::Encoding base = Environment::get_base_name(name, e);
      if(!base.empty() && e) e->AddEntry(base, new BindTemplateFunction(def));
      return e;
    }
  }
  return this;
}

Environment* Environment::RecordDeclarator(PTree::Node *decl)
{
  if(PTree::type_of(decl) == Token::ntDeclarator)
  {
    PTree::Encoding name = decl->encoded_name();
    PTree::Encoding type = decl->encoded_type();
    if(!name.empty() && !type.empty())
    {
      Environment *e = this;
      PTree::Encoding base = Environment::get_base_name(name, e);

      // allow a duplicated entry because of overloaded functions
      if(!base.empty() && e) e->AddDupEntry(base, new BindVarName(type));
      return e;
    }
  }
  return this;
}

Environment* Environment::DontRecordDeclarator(PTree::Node *decl)
{
  if(PTree::type_of(decl) == Token::ntDeclarator)
  {
    PTree::Encoding name = decl->encoded_name();
    if(!name.empty())
    {
      Environment *e = this;
      Environment::get_base_name(name, e);
      return e;
    }
  }
  return this;
}

void Environment::RecordMetaclassName(PTree::Node *decl)
{
  if(PTree::third(decl)) metaclasses.append(decl);
}

PTree::Node *Environment::LookupMetaclass(PTree::Node *name)
{
  size_t n = metaclasses.number();
  for(size_t i = 0; i < n; ++i)
  {
    PTree::Node *d = metaclasses[i];
    if(PTree::third(d) && *PTree::third(d) == *name) return d;
  }
  return 0;
}

bool Environment::RecordClasskeyword(const char* keyword, const char* metaclass_name)
{
  PTree::Node *keywordp = new PTree::Atom(keyword, strlen(keyword));
  PTree::Node *metaclassp = new PTree::Atom(metaclass_name, strlen(metaclass_name));

  if(LookupClasskeyword(keywordp) == 0)
  {
    classkeywords->append(keywordp);
    classkeywords->append(metaclassp);
    return true;
  }
  else return false;
}

PTree::Node *Environment::LookupClasskeyword(PTree::Node *keyword)
{
  if(!classkeywords) classkeywords = new PTree::Array;
  size_t n = classkeywords->number();
  for(size_t i = 0; i < n; i += 2)
  {
    PTree::Node *d = classkeywords->ref(i);
    if(d && keyword && *d == *keyword)
      return classkeywords->ref(i + 1);
  }
  return 0;
}

Class* Environment::LookupThis()
{
    Environment* p;
    for(p = this; p != 0; p = p->next)
	if(p->metaobject != 0)
	    return p->metaobject;

    return 0;
}

// IsMember() returns the class environment that the member belongs to.
// If the member is not valid, IsMember() returns 0.

Environment* Environment::IsMember(PTree::Node *member)
{
  Bind* bind;
  Environment* e;

  if(!member->is_atom())
  {
    PTree::Encoding name = member->encoded_name();
    if(!name.empty())
    {
      e = this;
      PTree::Encoding base = Environment::get_base_name(name, e);
      if(!base.empty() && e && e->metaobject) return e;
    }
  }
  for(e = this; e; e = e->next)
    if(e->metaobject) break;
    else if(e->LookupTop(member, bind))
      if(bind && !bind->IsType())
	return 0;	// the member is overridden.
  if(e && e->LookupTop(member, bind))
    if(bind && !bind->IsType())
      return e;
  return 0;
}

//. get_base_name() returns "Foo" if ENCODE is "Q[2][1]X[3]Foo", for example.
//. If an error occurs, the function returns 0.
PTree::Encoding Environment::get_base_name(const PTree::Encoding &enc,
					   Environment *&env)
{
  if(enc.empty()) return enc;
  Environment *e = env;
  PTree::Encoding::iterator i = enc.begin();
  if(*i == 'Q')
  {
    int n = *(i + 1) - 0x80;
    i += 2;
    while(n-- > 1)
    {
      int m = *i++;
      if(m == 'T') m = get_base_name_if_template(i, e);
      else if(m < 0x80) return PTree::Encoding(); // error?
      else
      {	 // class name
	m -= 0x80;
	if(m <= 0)
	{		// if global scope (e.g. ::Foo)
	  if(e) e = e->GetBottom();
	}
	else e = resolve_typedef_name(i, m, e);
      }
      i += m;
    }
    env = e;
  }
  if(*i == 'T')
  {		// template class
    int m = *(i + 1) - 0x80;
    int n = *(i + m + 2) - 0x80;
    return PTree::Encoding(i, i + m + n + 3);
  }
  else if(*i < 0x80) return PTree::Encoding();
  else return PTree::Encoding(i + 1, i + 1 + size_t(*i - 0x80));
}

void Environment::Dump()
{
    htable->Dump(std::cerr);
    std::cerr << '\n';
}

void Environment::Dump(int level)
{
    Environment* e = this;
    while(level-- > 0)
      if(e->next) e = e->next;
      else
      {
	std::cerr << "Environment::Dump(): the bottom is reached.\n";
	return;
      }
    e->Dump();
}

Environment *Environment::resolve_typedef_name(PTree::Encoding::iterator i, size_t s,
					       Environment *env)
{
  TypeInfo tinfo;
  Bind *bind;
  Class *c = 0;

  if(env)
    if (env->LookupType(PTree::Encoding((const char *)&*i, s), bind) && bind)
      switch(bind->What())
      {
        case Bind::isClassName :
	  c = bind->ClassMetaobject();
	  break;
        case Bind::isTypedefName :
	  bind->GetType(tinfo, env);
	  c = tinfo.class_metaobject();
	  /* if (c == 0) */
	  env = 0;
	  break;
        default :
	  break;
      }
    else if (env->LookupNamespace(PTree::Encoding((const char *)&*i, s)))
    {
      /* For the time being, we simply ignore name spaces.
       * For example, std::T is identical to ::T.
       */
      env = env->GetBottom();
    }
    else env = 0; // unknown typedef name

  return c ? c->GetEnvironment() : env;
}

int Environment::get_base_name_if_template(PTree::Encoding::iterator i,
					   Environment *&e)
{
  int m = *i - 0x80;
  if(m <= 0) return *(i+1) - 0x80 + 2;

  Bind* b;
  if(e != 0 && e->LookupType(PTree::Encoding((const char*)&*(i + 1), m), b))
    if(b != 0 && b->What() == Bind::isTemplateClass)
    {
      Class* c = b->ClassMetaobject();
      if(c)
      {
	e = c->GetEnvironment();
	return m + (*(i + m + 1) - 0x80) + 2;
      }
    }
  // the template name was not found.
  e = 0;
  return m + (*(i + m + 1) - 0x80) + 2;
}

Environment::Array::Array(size_t s)
{
  num = 0;
  size = s;
  if(s > 0) array = new (PTree::GC) Environment*[s];
  else array = 0;
}

void Environment::Array::Append(Environment* p)
{
  if(num >= size)
  {
    size += 8;
    Environment** a = new (PTree::GC) Environment*[size];
    memmove(a, array, size_t(num * sizeof(Environment*)));
    delete [] array;
    array = a;
  }
  array[num++] = p;
}

Environment* Environment::Array::Ref(size_t i)
{
  if(i < num) return array[i];
  else return 0;
}

bool Bind::IsType()
{
  return true;
}

Class* Bind::ClassMetaobject()
{
  return 0;
}

void Bind::SetClassMetaobject(Class*) {}

Bind::Kind BindVarName::What()
{
  return isVarName;
}

void BindVarName::GetType(TypeInfo& t, Environment* e)
{
  t.set(my_type, e);
}

bool BindVarName::IsType()
{
  return false;
}

Bind::Kind BindTypedefName::What()
{
  return isTypedefName;
}

void BindTypedefName::GetType(TypeInfo& t, Environment* e)
{
  t.set(my_type, e);
}

Bind::Kind BindClassName::What()
{
  return isClassName;
}

void BindClassName::GetType(TypeInfo& t, Environment*)
{
  t.set(metaobject);
}

Class* BindClassName::ClassMetaobject()
{
  return metaobject;
}

void BindClassName::SetClassMetaobject(Class* c)
{
  metaobject = c;
}

BindEnumName::BindEnumName(const PTree::Encoding &t, PTree::Node *s)
  : my_type(t),
    my_spec(s)
{
}

Bind::Kind BindEnumName::What()
{
  return isEnumName;
}

void BindEnumName::GetType(TypeInfo& t, Environment* e)
{
  t.set(my_type, e);
}

Bind::Kind BindTemplateClass::What()
{
  return isTemplateClass;
}

void BindTemplateClass::GetType(TypeInfo& t, Environment*)
{
  t.set(metaobject);
}

Class* BindTemplateClass::ClassMetaobject()
{
  return metaobject;
}

void BindTemplateClass::SetClassMetaobject(Class* c)
{
  metaobject = c;
}

Bind::Kind BindTemplateFunction::What()
{
  return isTemplateFunction;
}

void BindTemplateFunction::GetType(TypeInfo& t, Environment*)
{
    t.unknown();
}

bool BindTemplateFunction::IsType()
{
  return false;
}
