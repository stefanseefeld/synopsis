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
#include "Ptree.hh"
#include "ptree.h"
#include "Lexer.hh"
#include "Encoding.hh"
#include "Walker.hh"
#include "TypeInfo.hh"
#include "Class.hh"
#include "Parser.hh"

// class Environment

PtreeArray* Environment::classkeywords = nil;
HashTable* Environment::namespace_table = nil;

void Environment::do_init_static()
{
    namespace_table = new HashTable;
}

Environment::Environment(Walker* w)
: baseclasses(0)
{
    htable = new BigHashTable;		// base environment
    next = nil;
    metaobject = nil;
    walker = w;
}

Environment::Environment(Environment* e)
: baseclasses(0)
{
    htable = new HashTable;
    next = e;
    metaobject = nil;
    walker = e->walker;
}

Environment::Environment(Environment* e, Walker* w)
: baseclasses(0)
{
    htable = new HashTable;
    next = e;
    metaobject = nil;
    walker = w;
}

bool Environment::IsEmpty()
{
    return bool(htable->IsEmpty());
}

Environment* Environment::GetBottom()
{
    Environment* p;
    for(p = this; p->next != nil; p = p->next)
	;

    return p;
}

Class* Environment::LookupClassMetaobject(Ptree* name)
{
    TypeInfo tinfo;
    Bind* bind = nil;

    if(this == nil){
	MopErrorMessage("Environment::LookupClassMetaobject()",
			"nil enviornment");
	return nil;
    }

    if(name == nil)
	return nil;
    else if(name->IsLeaf()){
	if(LookupType(name->GetPosition(), name->GetLength(), bind))
	    if(bind != nil){
		bind->GetType(tinfo, this);
		return tinfo.ClassMetaobject();
	    }

	return nil;
    }
    else{
	int len;
	Environment* e = this;
	char* base = Encoding::GetBaseName(name->GetEncodedName(), len, e);
	if(base != nil && e != nil)
	    if(LookupType(base, len, bind))
		if(bind != nil){
		    bind->GetType(tinfo, this);
		    return tinfo.ClassMetaobject();
		}

	return nil;
    }
}

bool Environment::LookupType(const char* name, int len, Bind*& t)
{
    Environment* p;

    for(p = this; p != nil; p = p->next){
	int count = 0;
	while(p->htable->LookupEntries((char*)name, len, (HashValue*)&t,
				       count))
	    if(t != nil)
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

bool Environment::Lookup(Ptree* name, TypeInfo& t)
{
    Bind* bind;

    if(Lookup(name, bind) && bind != nil){
	bind->GetType(t, this);
	return true;
    }
    else{
	t.Unknown();
	return false;
    }
}

bool Environment::Lookup(Ptree* name, bool& is_type_name, TypeInfo& t)
{
    Bind* bind;

    if(Lookup(name, bind) && bind != nil){
	is_type_name = bind->IsType();
	bind->GetType(t, this);
	return true;
    }
    else{
	t.Unknown();
	return false;
    }
}

bool Environment::Lookup(Ptree* name, Bind*& bind)
{
    bind = nil;
    if(this == nil){
	MopErrorMessage("Environment::Lookup()", "nil enviornment");
	return false;
    }

    if(name == nil)
	return false;
    else if(name->IsLeaf())
	return LookupAll(name->GetPosition(), name->GetLength(), bind);
    else{
	char* encode = name->GetEncodedName();
	if(encode == nil)
	    return false;
	else{
	    int len;
	    Environment* e = this;
	    char* base = Encoding::GetBaseName(encode, len, e);
	    if(base != nil && e != nil)
		return e->LookupAll(base, len, bind);
	    else
		return false;
	}
    }
}

bool Environment::LookupTop(Ptree* name, Bind*& bind)
{
    bind = nil;
    if(this == nil){
	MopErrorMessage("Environment::LookupTop()", "nil enviornment");
	return false;
    }

    if(name == nil)
	return false;
    else if(name->IsLeaf())
	return LookupTop(name->GetPosition(), name->GetLength(), bind);
    else{
	char* encode = name->GetEncodedName();
	if(encode == nil)
	    return false;
	else{
	    int len;
	    Environment* e = this;
	    char* base = Encoding::GetBaseName(encode, len, e);
	    if(base != nil && e != nil)
		return e->LookupTop(base, len, bind);
	    else
		return false;
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

    for(p = this; p != nil; p = p->next)
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

bool Environment::RecordVariable(char* name, Class* c)
{
    Encoding encode;
    encode.SimpleName(c->Name());
    return htable->AddEntry(name, new BindVarName(encode.Get())) >= 0;
}

bool Environment::RecordPointerVariable(char* name, Class* c)
{
    Encoding encode;
    encode.SimpleName(c->Name());
    encode.PtrOperator('*');
    return htable->AddEntry(name, new BindVarName(encode.Get())) >= 0;
}

int Environment::AddEntry(char* key, int len, Bind* b) {
    return htable->AddEntry(key, len, b);
}

int Environment::AddDupEntry(char* key, int len, Bind* b) {
    return htable->AddDupEntry(key, len, b);
}

void Environment::RecordNamespace(Ptree* name)
{
    if (name != nil)
	namespace_table->AddEntry(name->GetPosition(), name->GetLength(),
				  name);
}

bool Environment::LookupNamespace(char* name, int len)
{
    HashValue value;
    return namespace_table->Lookup(name, len, &value);
}

void Environment::RecordTypedefName(Ptree* decls)
{
    while(decls != nil){
	Ptree* d = decls->Car();
	if(d->What() == ntDeclarator){
	    char* name = d->GetEncodedName();
	    char* type = d->GetEncodedType();
	    if(name != nil && type != nil){
		int len;
		Environment* e = this;
		name = Encoding::GetBaseName(name, len, e);
		if(name != nil)
		    AddEntry(name, len, new BindTypedefName(type));
	    }
	}

	decls = Ptree::ListTail(decls, 2);
    }
}

void Environment::RecordEnumName(Ptree* spec)
{
    Ptree* tag = Ptree::Second(spec);
    char* encoded_name = spec->GetEncodedName();
    if(tag != nil && tag->IsLeaf())
	AddEntry(tag->GetPosition(), tag->GetLength(),
		 new BindEnumName(encoded_name, spec));
    else{
	int n;
	Environment* e = this;
	char* name = Encoding::GetBaseName(encoded_name, n, e);
	if(name != nil && e != nil)
	    e->AddEntry(name, n, new BindEnumName(encoded_name, spec));
    }
}

void Environment::RecordClassName(char* encoded_name, Class* metaobject)
{
    int n;
    Environment* e;
    Bind* bind;

    e = this;
    char* name = Encoding::GetBaseName(encoded_name, n, e);
    if(name == nil || e == nil)
	return;		// error?

    if(e->LookupAll(name, n, bind))
	if(bind != nil && bind->What() == Bind::isClassName){
	    if(metaobject != nil)
		bind->SetClassMetaobject(metaobject);

	    return;
	}

    e->AddEntry(name, n, new BindClassName(metaobject));
}

void Environment::RecordTemplateClass(Ptree* spec, Class* metaobject)
{
    int n;
    Environment* e;
    Bind* bind;

    e = this;
    char* name = Encoding::GetBaseName(spec->GetEncodedName(), n, e);
    if(name == nil || e == nil)
	return;		// error?

    if(e->LookupAll(name, n, bind))
	if(bind != nil && bind->What() == Bind::isTemplateClass){
	    if(metaobject != nil)
		bind->SetClassMetaobject(metaobject);

	    return;
	}

    e->AddEntry(name, n, new BindTemplateClass(metaobject));
}

Environment* Environment::RecordTemplateFunction(Ptree* def, Ptree* body)
{
    int n;
    Ptree* decl = Ptree::Third(body);
    if(decl->IsA(ntDeclarator)){
	char* name = decl->GetEncodedName();
	if(name != nil){
	    Environment* e = this;
	    name = Encoding::GetBaseName(name, n, e);
	    if(name != nil && e != nil)
		e->AddEntry(name, n, new BindTemplateFunction(def));

	    return e;
	}
    }

    return this;
}

Environment* Environment::RecordDeclarator(Ptree* decl)
{
    if(decl->What() == ntDeclarator){
	char* name = decl->GetEncodedName();
	char* type = decl->GetEncodedType();
	if(name != nil && type != nil){
	    int len;
	    Environment* e = this;
	    name = Encoding::GetBaseName(name, len, e);

	    // allow a duplicated entry because of overloaded functions
	    if(name != nil && e != nil)
		e->AddDupEntry(name, len, new BindVarName(type));

	    return e;
	}
    }

    return this;
}

Environment* Environment::DontRecordDeclarator(Ptree* decl)
{
    if(decl->What() == ntDeclarator){
	char* name = decl->GetEncodedName();
	if(name != nil){
	    int len;
	    Environment* e = this;
	    Encoding::GetBaseName(name, len, e);
	    return e;
	}
    }

    return this;
}

void Environment::RecordMetaclassName(Ptree* decl)
{
    if(decl->Third() != nil)
	metaclasses.Append(decl);
}

Ptree* Environment::LookupMetaclass(Ptree* name)
{
    uint n = metaclasses.Number();
    for(uint i = 0; i < n; ++i){
	Ptree* d = metaclasses[i];
	if(d->Third()->Eq(name))
	    return d;
    }

    return nil;
}

bool Environment::RecordClasskeyword(char* keyword, char* metaclass_name)
{
    Ptree* keywordp = new Leaf(keyword, strlen(keyword));
    Ptree* metaclassp = new Leaf(metaclass_name, strlen(metaclass_name));

    if(LookupClasskeyword(keywordp) == nil){
	classkeywords->Append(keywordp);
	classkeywords->Append(metaclassp);
	return true;
    }
    else
	return false;
}

Ptree* Environment::LookupClasskeyword(Ptree* keyword)
{
    if(classkeywords == nil)
	classkeywords = new PtreeArray;

    uint n = classkeywords->Number();
    for(uint i = 0; i < n; i += 2){
	Ptree* d = classkeywords->Ref(i);
	if(d->Eq(keyword))
	    return classkeywords->Ref(i + 1);
    }

    return nil;
}

Class* Environment::LookupThis()
{
    Environment* p;
    for(p = this; p != nil; p = p->next)
	if(p->metaobject != nil)
	    return p->metaobject;

    return nil;
}

// IsMember() returns the class environment that the member belongs to.
// If the member is not valid, IsMember() returns nil.

Environment* Environment::IsMember(Ptree* member)
{
    Bind* bind;
    Environment* e;

    if(!member->IsLeaf()){
	char* encode = member->GetEncodedName();
	if(encode != nil){
	    int len;
	    e = this;
	    char* base = Encoding::GetBaseName(encode, len, e);
	    if(base != nil && e != nil && e->metaobject != nil)
		return e;
	}
    }

    for(e = this; e != nil; e = e->next)
	if(e->metaobject != nil)
	    break;
	else if(e->LookupTop(member, bind))
	    if(bind != nil && !bind->IsType())
		return nil;	// the member is overridden.

    if(e != nil && e->LookupTop(member, bind))
       if(bind != nil && !bind->IsType())
	   return e;

    return nil;
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
	if(e->next != nil)
	    e = e->next;
	else{
	    std::cerr << "Environment::Dump(): the bottom is reached.\n";
	    return;
	}

    e->Dump();
}

Ptree* Environment::GetLineNumber(Ptree* p, int& number)
{
    if (walker == nil) {
	number = 0;
	return nil;
    }

    char* fname;
    int fname_len;
    number = (int)walker->GetParser()->LineNumber(p->GetPosition(),
						  fname, fname_len);
    return new Leaf(fname, fname_len);
}


// class Environment::Array

Environment::Array::Array(int s)
{
    num = 0;
    size = s;
    if(s > 0)
	array = new (GC) Environment*[s];
    else
	array = nil;
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
	return nil;
}


// class Bind

char* Bind::GetEncodedType()
{
    return nil;
}

bool Bind::IsType()
{
    return true;
}

Class* Bind::ClassMetaobject()
{
    return nil;
}

void Bind::SetClassMetaobject(Class*) {}

Bind::Kind BindVarName::What()
{
    return isVarName;
}

void BindVarName::GetType(TypeInfo& t, Environment* e)
{
    t.Set(type, e);
}

char* BindVarName::GetEncodedType()
{
    return type;
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
    t.Set(type, e);
}

char* BindTypedefName::GetEncodedType()
{
    return type;
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

BindEnumName::BindEnumName(char* encoded_type, Ptree* spec)
{
    type = encoded_type;
    specification = spec;
}

Bind::Kind BindEnumName::What()
{
    return isEnumName;
}

void BindEnumName::GetType(TypeInfo& t, Environment* e)
{
    t.Set(type, e);
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
