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

#include <iostream>
#include <cstring>
#include "Environment.hh"
#include "HashTable.hh"
#include "PTree.hh"
#include "Lexer.hh"
#include "Encoding.hh"
#include "Walker.hh"
#include "TypeInfo.hh"
#include "Class.hh"
#include "Parser.hh"

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

  if(this == 0)
  {
    MopErrorMessage("Environment::LookupClassMetaobject()",
		    "nil enviornment");
    return 0;
  }

  if(name == 0) return 0;
  else if(name->is_atom())
  {
    if(LookupType(name->position(), name->length(), bind))
      if(bind)
      {
	bind->GetType(tinfo, this);
	return tinfo.ClassMetaobject();
      }
    return 0;
  }
  else
  {
    int len;
    Environment* e = this;
    const char* base = Encoding::GetBaseName(name->encoded_name(), len, e);
    if(base != 0 && e != 0)
      if(LookupType(base, len, bind))
	if(bind)
	{
	  bind->GetType(tinfo, this);
	  return tinfo.ClassMetaobject();
	}
    return 0;
  }
}

bool Environment::LookupType(const char* name, int len, Bind*& t)
{
    Environment* p;

    for(p = this; p != 0; p = p->next){
	int count = 0;
	while(p->htable->LookupEntries((char*)name, len, (HashValue*)&t,
				       count))
	    if(t != 0)
		switch(t->What()){
		case Bind::isVarName :
		case Bind::isTemplateFunction :
		    break;
		default :
		    return true;
		}

	uint n = p->baseclasses.Number();
	for(uint i = 0; i < n; ++i)
	    if(p->baseclasses.Ref(i)->LookupType(name, len, t))
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
	t.Unknown();
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
	t.Unknown();
	return false;
    }
}

bool Environment::Lookup(PTree::Node *name, Bind*& bind)
{
  bind = 0;
  if(this == 0)
  {
    MopErrorMessage("Environment::Lookup()", "nil enviornment");
    return false;
  }

  if(name == 0) return false;
  else if(name->is_atom())
    return LookupAll(name->position(), name->length(), bind);
  else
  {
    const char* encode = name->encoded_name();
    if(encode == 0) return false;
    else
    {
      int len;
      Environment* e = this;
      const char* base = Encoding::GetBaseName(encode, len, e);
      if(base && e) return e->LookupAll(base, len, bind);
      else return false;
    }
  }
}

bool Environment::LookupTop(PTree::Node *name, Bind*& bind)
{
  bind = 0;
  if(this == 0)
  {
    MopErrorMessage("Environment::LookupTop()", "nil enviornment");
    return false;
  }

  if(name == 0) return false;
  else if(name->is_atom())
    return LookupTop(name->position(), name->length(), bind);
  else
  {
    const char *encode = name->encoded_name();
    if(encode == 0) return false;
    else
    {
      int len;
      Environment* e = this;
      const char *base = Encoding::GetBaseName(encode, len, e);
      if(base && e) return e->LookupTop(base, len, bind);
      else return false;
    }
  }
}

bool Environment::LookupTop(const char* name, int len, Bind*& t)
{
    if(htable->Lookup((char*)name, len, (HashValue*)&t))
	return true;
    else{
	uint n = baseclasses.Number();
	for(uint i = 0; i < n; ++i)
	    if(baseclasses.Ref(i)->LookupTop(name, len, t))
		return true;

	return false;
    }
}

bool Environment::LookupAll(const char* name, int len, Bind*& t)
{
    Environment* p;

    for(p = this; p != 0; p = p->next)
	if(p->htable->Lookup((char*)name, len, (HashValue*)&t))
	    return true;
	else{
	    uint n = p->baseclasses.Number();
	    for(uint i = 0; i < n; ++i)
		if(p->baseclasses.Ref(i)->LookupAll(name, len, t))
		    return true;
	}

    return false;
}

bool Environment::RecordVariable(const char* name, Class* c)
{
    Encoding encode;
    encode.SimpleName(c->Name());
    return htable->AddEntry(name, new BindVarName(encode.Get())) >= 0;
}

bool Environment::RecordPointerVariable(const char* name, Class* c)
{
    Encoding encode;
    encode.SimpleName(c->Name());
    encode.PtrOperator('*');
    return htable->AddEntry(name, new BindVarName(encode.Get())) >= 0;
}

int Environment::AddEntry(const char* key, int len, Bind* b) {
    return htable->AddEntry(key, len, b);
}

int Environment::AddDupEntry(const char* key, int len, Bind* b) {
    return htable->AddDupEntry(key, len, b);
}

void Environment::RecordNamespace(PTree::Node *name)
{
    if (name != 0)
	namespace_table->AddEntry(name->position(), name->length(),
				  name);
}

bool Environment::LookupNamespace(const char* name, int len)
{
    HashValue value;
    return namespace_table->Lookup(name, len, &value);
}

void Environment::RecordTypedefName(PTree::Node *decls)
{
  while(decls)
  {
    PTree::Node *d = decls->car();
    if(d->What() == Token::ntDeclarator)
    {
      const char *name = d->encoded_name();
      const char *type = d->encoded_type();
      if(name != 0 && type != 0)
      {
	int len;
	Environment* e = this;
	name = Encoding::GetBaseName(name, len, e);
	if(name) AddEntry(name, len, new BindTypedefName(type));
      }
    }
    decls = PTree::tail(decls, 2);
  }
}

void Environment::RecordEnumName(PTree::Node *spec)
{
  PTree::Node *tag = PTree::second(spec);
  const char* encoded_name = spec->encoded_name();
    if(tag != 0 && tag->is_atom())
	AddEntry(tag->position(), tag->length(),
		 new BindEnumName(encoded_name, spec));
    else{
	int n;
	Environment* e = this;
	const char* name = Encoding::GetBaseName(encoded_name, n, e);
	if(name != 0 && e != 0)
	    e->AddEntry(name, n, new BindEnumName(encoded_name, spec));
    }
}

void Environment::RecordClassName(const char* encoded_name, Class* metaobject)
{
  int n;
  Bind* bind;

  Environment *e = this;
  const char* name = Encoding::GetBaseName(encoded_name, n, e);
  if(!name || !e) return; // error?
  if(e->LookupAll(name, n, bind))
    if(bind && bind->What() == Bind::isClassName)
    {
      if(metaobject) bind->SetClassMetaobject(metaobject);
      return;
    }
  e->AddEntry(name, n, new BindClassName(metaobject));
}

void Environment::RecordTemplateClass(PTree::Node *spec, Class* metaobject)
{
  int n;
  Environment* e;
  Bind* bind;

  e = this;
  const char* name = Encoding::GetBaseName(spec->encoded_name(), n, e);
  if(!name || !e) return; // error?

  if(e->LookupAll(name, n, bind))
    if(bind && bind->What() == Bind::isTemplateClass)
    {
      if(metaobject) bind->SetClassMetaobject(metaobject);
      return;
    }
  e->AddEntry(name, n, new BindTemplateClass(metaobject));
}

Environment* Environment::RecordTemplateFunction(PTree::Node *def, PTree::Node *body)
{
  int n;
  PTree::Node *decl = PTree::third(body);
  if(decl->IsA(Token::ntDeclarator))
  {
    const char* name = decl->encoded_name();
    if(name)
    {
      Environment* e = this;
      name = Encoding::GetBaseName(name, n, e);
      if(name != 0 && e != 0)
	e->AddEntry(name, n, new BindTemplateFunction(def));
      return e;
    }
  }
  return this;
}

Environment* Environment::RecordDeclarator(PTree::Node *decl)
{
  if(decl->What() == Token::ntDeclarator)
  {
    const char* name = decl->encoded_name();
    const char* type = decl->encoded_type();
    if(name && type)
    {
      int len;
      Environment* e = this;
      name = Encoding::GetBaseName(name, len, e);

      // allow a duplicated entry because of overloaded functions
      if(name && e) e->AddDupEntry(name, len, new BindVarName(type));
      return e;
    }
  }
  return this;
}

Environment* Environment::DontRecordDeclarator(PTree::Node *decl)
{
  if(decl->What() == Token::ntDeclarator)
  {
    const char* name = decl->encoded_name();
    if(name)
    {
      int len;
      Environment* e = this;
      Encoding::GetBaseName(name, len, e);
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
    const char* encode = member->encoded_name();
    if(encode != 0)
    {
      int len;
      e = this;
      const char* base = Encoding::GetBaseName(encode, len, e);
      if(base != 0 && e != 0 && e->metaobject != 0) return e;
    }
  }
  for(e = this; e != 0; e = e->next)
    if(e->metaobject != 0)
      break;
    else if(e->LookupTop(member, bind))
      if(bind != 0 && !bind->IsType())
	return 0;	// the member is overridden.
  if(e != 0 && e->LookupTop(member, bind))
    if(bind != 0 && !bind->IsType())
      return e;
  return 0;
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
	if(e->next != 0)
	    e = e->next;
	else{
	    std::cerr << "Environment::Dump(): the bottom is reached.\n";
	    return;
	}

    e->Dump();
}

// Ptree* Environment::GetLineNumber(Ptree* p, int& number)
// {
//     if (walker == 0) {
// 	number = 0;
// 	return 0;
//     }

//     char* fname;
//     int fname_len;
//     number = (int)walker->GetParser()->LineNumber(p->position(),
// 						  fname, fname_len);
//     return new Leaf(fname, fname_len);
// }


// class Environment::Array

Environment::Array::Array(int s)
{
    num = 0;
    size = s;
    if(s > 0)
	array = new (GC) Environment*[s];
    else
	array = 0;
}

void Environment::Array::Append(Environment* p)
{
    if(num >= size){
	size += 8;
	Environment** a = new (GC) Environment*[size];
	memmove(a, array, size_t(num * sizeof(Environment*)));
	delete [] array;
	array = a;
    }

    array[num++] = p;
}

Environment* Environment::Array::Ref(uint i)
{
    if(i < num)
	return array[i];
    else
	return 0;
}


// class Bind

const char *Bind::encoded_type()
{
  return 0;
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
  t.Set(my_type, e);
}

const char *BindVarName::encoded_type()
{
  return my_type;
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
  t.Set(my_type, e);
}

const char *BindTypedefName::encoded_type()
{
  return my_type;
}

Bind::Kind BindClassName::What()
{
    return isClassName;
}

void BindClassName::GetType(TypeInfo& t, Environment*)
{
    t.Set(metaobject);
}

Class* BindClassName::ClassMetaobject()
{
    return metaobject;
}

void BindClassName::SetClassMetaobject(Class* c)
{
    metaobject = c;
}

BindEnumName::BindEnumName(const char *t, PTree::Node *s)
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
    t.Set(my_type, e);
}

Bind::Kind BindTemplateClass::What()
{
    return isTemplateClass;
}

void BindTemplateClass::GetType(TypeInfo& t, Environment*)
{
    t.Set(metaobject);
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
    t.Unknown();
}

bool BindTemplateFunction::IsType()
{
    return false;
}
