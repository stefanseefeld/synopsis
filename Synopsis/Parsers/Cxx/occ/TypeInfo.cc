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

#include "TypeInfo.hh"
#include "Encoding.hh"
#include "PTree.hh"
#include "Environment.hh"
#include "Class.hh"

TypeInfo::TypeInfo()
{
    refcount = 0;
    encode = 0;
    metaobject = 0;
    env = 0;
}

void TypeInfo::Unknown()
{
    refcount = 0;
    encode = 0;
    metaobject = 0;
    env = 0;
}

void TypeInfo::Set(char* type, Environment* e)
{
    refcount = 0;
    encode = type;
    metaobject = 0;
    env = e;
}

void TypeInfo::Set(Class* m)
{
    refcount = 0;
    encode = 0;
    metaobject = m;
    env = 0;
}

void TypeInfo::SetVoid()
{
    refcount = 0;
    encode = "v";
    metaobject = 0;
    env = 0;
}

void TypeInfo::SetInt()
{
    refcount = 0;
    encode = "i";
    metaobject = 0;
    env = 0;
}

void TypeInfo::SetMember(PTree::Node *member)
{
    Class* c = ClassMetaobject();
    if(c == 0)
	Unknown();
    else{
	Environment* e = c->GetEnvironment();
	if(e == 0)
	    Unknown();
	else
	    e->Lookup(member, *this);
    }
}

TypeInfoId TypeInfo::WhatIs()
{
    if(refcount > 0)
	return PointerType;

    Normalize();
    if(metaobject != 0)
	return ClassType;

    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    if(ptr == 0)
	return UndefType;

    switch(*ptr){
    case 'T' :
	return TemplateType;
    case 'P' :
	return PointerType;
    case 'R' :
	return ReferenceType;
    case 'M' :
	return PointerToMemberType;
    case 'A' :
	return ArrayType;
    case 'F' :
	return FunctionType;
    case 'S' :
    case 'U' :
    case 'b' :
    case 'c' :
    case 'w' :
    case 'i' :
    case 's' :
    case 'l' :
    case 'j' :
    case 'f' :
    case 'd' :
    case 'r' :
    case 'v' :
	return BuiltInType;
    default :
	if(*ptr == 'Q' || *(unsigned char*)ptr >= 0x80){
	    TypeInfo t;
	    Class* c;
	    t.Set(ptr, e);
	    if(t.IsClass(c))
		return ClassType;
	    else if(t.IsEnum())
		return EnumType;
	}

	return UndefType;
    };
}

bool TypeInfo::IsNoReturnType()
{
    Normalize();
    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    return(ptr != 0 && *ptr == '?');
}

bool TypeInfo::IsConst()
{
    Normalize();
    char* ptr = encode;
    return(ptr != 0 && *ptr == 'C');
}

bool TypeInfo::IsVolatile()
{
    Normalize();
    char* ptr = encode;
    if(ptr == 0)
	return false;
    else if(*ptr == 'V')
	return true;
    else if(*ptr == 'C')
	return(ptr[1] == 'V');
    else
	return false;
}

uint TypeInfo::IsBuiltInType()
{
    Normalize();
    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    if(ptr == 0)
	return 0;

    uint result = 0;
    for(;; ++ptr){
	switch(*ptr){
	case 'b' :
	    return(result | BooleanType);
	case 'c' :
	    return(result | CharType);
	case 'w' :
	    return(result | WideCharType);
	case 'i' :
	    return(result | IntType);
	case 's' :
	    return(result | ShortType);
	case 'l' :
	    return(result | LongType);
	case 'j' :
	    return(result | LongLongType);
	case 'f' :
	    return(result | FloatType);
	case 'd' :
	    return(result | DoubleType);
	case 'r' :
	    return(result | LongDoubleType);
	case 'v' :
	    return(result | VoidType);
	case 'S' :
	    result |= SignedType;
	    break;
	case 'U' :
	    result |= UnsignedType;
	    break;
	default :
	    return 0;
	}
    }
}

bool TypeInfo::IsFunction()
{
    Normalize();
    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    return(ptr != 0 && *ptr == 'F');
}

bool TypeInfo::IsEllipsis()
{
    Normalize();
    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    return(ptr != 0 && *ptr == 'e');
}

bool TypeInfo::IsPointerType()
{
    if(refcount > 0)
	return true;

    Normalize();
    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    if(ptr != 0){
	char c = *ptr;
	return c == 'P' || c == 'A' || c == 'M';
    }

    return false;
}

bool TypeInfo::IsReferenceType()
{
    Normalize();
    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    return(ptr != 0 && *ptr == 'R');
}

bool TypeInfo::IsArray()
{
    Normalize();
    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    return(ptr != 0 && *ptr == 'A');
}

bool TypeInfo::IsPointerToMember()
{
    Normalize();
    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    return(ptr != 0 && *ptr == 'M');
}

bool TypeInfo::IsTemplateClass()
{
    Normalize();
    Environment* e = env;
    char* ptr = SkipCv(encode, e);
    return(ptr != 0 && *ptr == 'T');
}

Class* TypeInfo::ClassMetaobject()
{
    Class* c;
    IsClass(c);
    return c;
}

bool TypeInfo::IsClass(Class*& c)
{
    Normalize();
    if(metaobject != 0){
	c = metaobject;
	return true;
    }
    else{
	c = 0;
	Environment* e = env;
	char* encode2 = SkipCv(encode, e);
	if(encode == encode2)
	    return false;

	TypeInfo tinfo;
	tinfo.Set(encode2, e);
	return tinfo.IsClass(c);
    }
}

bool TypeInfo::IsEnum()
{
    PTree::Node *spec;
    return IsEnum(spec);
}

bool TypeInfo::IsEnum(PTree::Node *&spec)
{
    spec = 0;
    Normalize();
    if(metaobject != 0)
	return false;
    else{
	Bind* bind;
	int len;
	Environment* e = env;
	char* name = Encoding::GetBaseName(encode, len, e);
	if(name != 0 && e != 0)
	    if(e->LookupType(name, len, bind))
		if(bind != 0 && bind->What() == Bind::isEnumName){
		    spec = ((BindEnumName*)bind)->GetSpecification();
		    return true;
		}

	e = env;
	name = SkipCv(encode, e);
	if(name == encode)
	    return false;

	TypeInfo tinfo;
	tinfo.Set(name, e);
	return tinfo.IsEnum(spec);
    }
}

void TypeInfo::Dereference(TypeInfo& t)
{
    t.refcount = refcount - 1;
    t.encode = encode;
    t.metaobject = metaobject;
    t.env = env;
}

void TypeInfo::Reference(TypeInfo& t)
{
    t.refcount = refcount + 1;
    t.encode = encode;
    t.metaobject = metaobject;
    t.env = env;
}

bool TypeInfo::NthArgument(int n, TypeInfo& t)
{
    Environment* e = env;
    Normalize();
    char* ptr = SkipCv(encode, e);
    if(ptr == 0 || *ptr != 'F'){
	t.Unknown();
	return false;
    }

    ++ptr;
    if(*ptr == 'v'){
	t.SetVoid();
	return false;		// no arguments
    }

    while(n-- > 0){
	ptr = SkipType(ptr, e);
	if(ptr == 0 || *ptr == '_'){
	    t.Unknown();
	    return false;
	}
    }

    t.Set(ptr, e);
    return true;
}

int TypeInfo::NumOfArguments()
{
    Environment* e = env;
    Normalize();
    char* ptr = SkipCv(encode, e);
    if(ptr == 0 || *ptr != 'F')
	return -1;	// not a function

    ++ptr;
    if(*ptr == 'v')
	return 0;	// no arguments

    int n = 0;
    for(;;){
	++n;
	ptr = SkipType(ptr, e);
	if(ptr == 0 || *ptr == '_')
	    return n;
    }
}

bool TypeInfo::NthTemplateArgument(int n, TypeInfo& t)
{
    Environment* e = env;
    Normalize();
    char* ptr = SkipCv(encode, e);
    if(ptr == 0 || *ptr != 'T'){
	t.Unknown();
	return false;
    }

    int len;
    ptr = (char*)Encoding::GetTemplateArguments((unsigned char*)++ptr, len);
    char* end = ptr + len;
    while(n-- > 0){
	ptr = SkipType(ptr, e);
	if(ptr == 0 || ptr >= end) {
	    t.Unknown();
	    return false;
	}
    }

    t.Set(ptr, e);
    return true;
}

PTree::Node *TypeInfo::FullTypeName()
{
  PTree::Node *qname, *head;

    Normalize();
    if(metaobject != 0){
	qname = metaobject->Name();
	head = GetQualifiedName2(metaobject);
	if(head == 0)
	    return qname;
	else
	  return PTree::Node::Snoc(head, qname);
    }

    Environment* e = env;
    unsigned char* name = (unsigned char*)SkipCv(encode, e);
    if(name == 0)
	return 0;

    if(IsBuiltInType())
	return Encoding::MakePtree(name, 0)->First();

    if(*name == 'T'){
	++name;
	qname = Encoding::MakeLeaf(name);
	head = GetQualifiedName(e, qname);
	if(head == 0)
	    return qname;
	else
	  return PTree::Node::Snoc(head, qname);
    }
    else if(*name == 'Q'){
	qname = Encoding::MakeQname(++name);
	head = GetQualifiedName(e, qname->Car());
	if(head == 0)
	    return qname;
	else
	  return PTree::Node::Nconc(head, qname);
    }
    else if(Encoding::IsSimpleName(name)){
	qname = Encoding::MakeLeaf(name);
	head = GetQualifiedName(e, qname);
	if(head == 0)
	    return qname;
	else
	  return PTree::Node::Snoc(head, qname);
    }
    else
	return 0;
}

PTree::Node *TypeInfo::GetQualifiedName(Environment* e, PTree::Node *tname)
{
    Class* c = e->LookupClassMetaobject(tname);
    if(c == 0)
	return 0;
    else
	return GetQualifiedName2(c);
}

PTree::Node *TypeInfo::GetQualifiedName2(Class* c)
{
    PTree::Node *qname = 0;
    Environment* e = c->GetEnvironment();
    if(e != 0)
	e = e->GetOuterEnvironment();

    for(; e != 0; e = e->GetOuterEnvironment()){
	c = e->IsClassEnvironment();
	if(c != 0)
	  qname = PTree::Node::Cons(c->Name(),
				    PTree::Node::Cons(Encoding::scope, qname));
    }

    return qname;
}

PTree::Node *TypeInfo::MakePtree(PTree::Node *name)
{
    Normalize();
    if(metaobject != 0){
	PTree::Node *decl;
	if(name == 0)
	    decl = 0;
	else
	  decl = PTree::Node::List(name);

	return PTree::Node::List(FullTypeName(), decl);
    }
    else if(encode != 0){
	unsigned char* ptr = (unsigned char*)encode;
	return Encoding::MakePtree(ptr, name);
    }
    else
	return 0;
}

void TypeInfo::Normalize()
{
    Environment* e = env;
    char* ptr = encode;
    int r = refcount;

    if(ptr == 0)
	return;
    else if(r > 0)
	return;

    while(r < 0)
	switch(*ptr){
	case 'C' :	// const
	case 'V' :	// volatile
	    ++ptr;
	    break;
	case 'A' :	// array
	case 'P' :	// pointer *
	case 'R' :	// reference &
	    ++ptr;
	    ++r;
	    break;
	case 'F' :	// function
	case 'M' :	// pointer to member ::*
	  {
	    char* p;
	    if(*ptr == 'F')
		p = GetReturnType(ptr + 1, e);
	    else
		p = SkipName(ptr + 1, e);

	    if(p == 0)
		return;
	    else{
		ptr = p;
		++r;
	    }
	    break;
	  }
	default :
	    if(!ResolveTypedef(e, ptr, true))
		return;
	}

    while(ResolveTypedef(e, ptr, false))
	;
}

bool TypeInfo::ResolveTypedef(Environment*& e, char*& ptr, bool resolvable)
{
    Bind* bind;
    int len;
    Class* c;
    Environment* orig_e = e;
    char* name = Encoding::GetBaseName(ptr, len, e);
    if(name != 0 && e != 0 && e->LookupType(name, len, bind))
	switch(bind->What()){
	case Bind::isTypedefName :
	    ptr = bind->GetEncodedType();
	    return true;
	case Bind::isClassName :
	    c = bind->ClassMetaobject();
	    if(c == 0)
		Set(ptr, orig_e);
	    else if (*name == 'T')
		Set(ptr, c->GetEnvironment());
	    else
		Set(c);

	    return false;
	case Bind::isTemplateClass :
	    c = bind->ClassMetaobject();
	    if(c == 0)
		Set(ptr, orig_e);
	    else
		Set(ptr, c->GetEnvironment());

	    return false;
	default :
	    break;
	}

    if(resolvable)
	Unknown();
    else
	Set(ptr, orig_e);

    return false;
}

char* TypeInfo::SkipCv(char* ptr, Environment*& e)
{
    if(ptr == 0)
	return 0;

    for(;;)
	switch(*ptr){
	case 'C' :	// const
	case 'V' :	// volatile
	    ++ptr;
	    break;
	default :
	    goto finish;
	}

finish:
    for(;;){
	Bind* bind;
	int len;
	char* name = Encoding::GetBaseName(ptr, len, e);
	if(name != 0 && e != 0 && e->LookupType(name, len, bind))
	    if(bind->What() != Bind::isTypedefName)
		return ptr;
	    else
		ptr = bind->GetEncodedType();
	else
	    return ptr;
    }
}

char* TypeInfo::SkipName(char* encode, Environment* e)
{
    if(e == 0)
	MopErrorMessage("TypeInfo::SkipName()", "nil environment");

    int len;
    Environment* e2 = e;
    char* ptr = Encoding::GetBaseName(encode, len, e2);
    if(ptr == 0)
	return 0;
    else
	return &ptr[len];
}

char* TypeInfo::GetReturnType(char* encode, Environment* env)
{
    for(;;)
	switch(*encode){
	case '_' :
	    return encode + 1;
	case '\0' :
	    return 0;
	default :
	    encode = SkipType(encode, env);
	    break;
	}
}

char* TypeInfo::SkipType(char* ptr, Environment* env)
{
    while(ptr != 0)
	switch(*ptr){
	case '\0' :
	case '_' :
	    return 0;
	case 'S' :
	case 'U' :
	case 'C' :
	case 'V' :
	case 'P' :
	case 'R' :
	case 'A' :
	    ++ptr;
	    break;
	case 'F' :
	    ptr =  GetReturnType(ptr + 1, env);
	    break;
	case 'T' :
	case 'Q' :
	    return SkipName(ptr, env);
	case 'M' :
	    ptr = SkipName(ptr + 1, env);
	    break;
	default :
	    if(*(unsigned char*)ptr < 0x80)
		return ptr + 1;
	    else
		return SkipName(ptr, env);
	}

    return ptr;
}
