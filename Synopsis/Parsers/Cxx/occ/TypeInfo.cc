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
#include "TypeInfo.hh"
#include "Environment.hh"
#include "Class.hh"
#include <stdexcept>

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

void TypeInfo::Set(const PTree::Encoding &type, Environment* e)
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
    PTree::Encoding ptr = skip_cv(encode, e);
    if(ptr.empty() == 0) return UndefType;

    switch(*ptr.begin())
    {
      case 'T' : return TemplateType;
      case 'P' : return PointerType;
      case 'R' : return ReferenceType;
      case 'M' : return PointerToMemberType;
      case 'A' : return ArrayType;
      case 'F' : return FunctionType;
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
      case 'v' : return BuiltInType;
      default :
	if(*ptr.begin() == 'Q' || *ptr.begin() >= 0x80)
	{
	  TypeInfo t;
	  Class* c;
	  t.Set(ptr, e);
	  if(t.IsClass(c)) return ClassType;
	  else if(t.IsEnum()) return EnumType;
	}
	return UndefType;
    };
}

bool TypeInfo::IsNoReturnType()
{
  Normalize();
  Environment* e = env;
  PTree::Encoding ptr = skip_cv(encode, e);
  return(!ptr.empty() && *ptr.begin() == '?');
}

bool TypeInfo::IsConst()
{
  Normalize();
  return(!encode.empty() && *encode.begin() == 'C');
}

bool TypeInfo::IsVolatile()
{
  Normalize();
  if(encode.empty()) return false;
  else if(encode.front() == 'V') return true;
  else if(encode.front() == 'C') return(encode.at(1) == 'V');
  else return false;
}

uint TypeInfo::IsBuiltInType()
{
  Normalize();
  Environment* e = env;
  PTree::Encoding ptr = skip_cv(encode, e);
  if(ptr.empty()) return 0;

  uint result = 0;
  for(PTree::Encoding::iterator i = ptr.begin(); i != ptr.end(); ++i)
  {
    switch(*i)
    {
      case 'b' : return result | BooleanType;
      case 'c' : return result | CharType;
      case 'w' : return result | WideCharType;
      case 'i' : return result | IntType;
      case 's' : return result | ShortType;
      case 'l' : return result | LongType;
      case 'j' : return result | LongLongType;
      case 'f' : return result | FloatType;
      case 'd' : return result | DoubleType;
      case 'r' : return result | LongDoubleType;
      case 'v' : return result | VoidType;
      case 'S' :
	result |= SignedType;
	break;
      case 'U' :
	result |= UnsignedType;
	break;
      default : return 0;
    }
  }
}

bool TypeInfo::IsFunction()
{
  Normalize();
  Environment* e = env;
  PTree::Encoding ptr = skip_cv(encode, e);
  return !ptr.empty() && ptr.front() == 'F';
}

bool TypeInfo::IsEllipsis()
{
  Normalize();
  Environment *e = env;
  PTree::Encoding  ptr = skip_cv(encode, e);
  return !ptr.empty() && ptr.front() == 'e';
}

bool TypeInfo::IsPointerType()
{
  if(refcount > 0) return true;

  Normalize();
  Environment* e = env;
  PTree::Encoding ptr = skip_cv(encode, e);
  if(!ptr.empty())
    return ptr.front() == 'P' || ptr.front() == 'A' || ptr.front() == 'M';
  return false;
}

bool TypeInfo::IsReferenceType()
{
  Normalize();
  Environment *e = env;
  PTree::Encoding ptr = skip_cv(encode, e);
  return !ptr.empty() && ptr.front() == 'R';
}

bool TypeInfo::IsArray()
{
  Normalize();
  Environment *e = env;
  PTree::Encoding ptr = skip_cv(encode, e);
  return !ptr.empty() && ptr.front() == 'A';
}

bool TypeInfo::IsPointerToMember()
{
  Normalize();
  Environment *e = env;
  PTree::Encoding ptr = skip_cv(encode, e);
  return !ptr.empty() && ptr.front() == 'M';
}

bool TypeInfo::IsTemplateClass()
{
  Normalize();
  Environment *e = env;
  PTree::Encoding ptr = skip_cv(encode, e);
  return !ptr.empty() && ptr.front() == 'T';
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
  if(metaobject != 0)
  {
    c = metaobject;
    return true;
  }
  else
  {
    c = 0;
    Environment* e = env;
    PTree::Encoding encode2 = skip_cv(encode, e);
    if(encode == encode2) return false;

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
  if(metaobject) return false;
  else
  {
    Bind* bind;
    Environment *e = env;
    PTree::Encoding name = encode.get_base_name(e);
    if(!name.empty() && e != 0)
      if(e->LookupType((const char *)&*name.begin(), name.size(), bind))
	if(bind != 0 && bind->What() == Bind::isEnumName)
	{
	  spec = ((BindEnumName*)bind)->GetSpecification();
	  return true;
	}
    e = env;
    name = skip_cv(encode, e);
    if(name == encode) return false;
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
  PTree::Encoding ptr = skip_cv(encode, e);
  if(ptr.empty() || ptr.front() != 'F')
  {
    t.Unknown();
    return false;
  }

  ptr.pop();
  if(ptr.front() == 'v')
  {
    t.SetVoid();
    return false;		// no arguments
  }

  while(n-- > 0)
  {
    ptr = skip_type(ptr, e);
    if(ptr.empty() || ptr.front() == '_')
    {
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
  PTree::Encoding ptr = skip_cv(encode, e);
  if(ptr.empty() || ptr.front() != 'F') return -1; // not a function
  
  ptr.pop();
  if(ptr.front() == 'v') return 0; // no arguments

  size_t n = 0;
  while(true)
  {
    ++n;
    ptr = skip_type(ptr, e);
    if(ptr.empty() || ptr.front() == '_') return n;
  }
}

bool TypeInfo::NthTemplateArgument(int n, TypeInfo& t)
{
  Environment *e = env;
  Normalize();
  PTree::Encoding ptr = skip_cv(encode, e);
  if(ptr.empty() || ptr.front() != 'T')
  {
    t.Unknown();
    return false;
  }
  ptr.pop();
  ptr = ptr.get_template_arguments();
  while(n-- > 0)
  {
    ptr = skip_type(ptr, e);
    // DANGER !!! : the original code reads:
    // if (ptr == 0 || ptr >= end)
    // see the history if you get into trouble here...
    if(ptr.empty())
    {
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
  if(metaobject != 0)
  {
    qname = metaobject->Name();
    head = GetQualifiedName2(metaobject);
    return head ? PTree::snoc(head, qname) : qname;
  }

  Environment *e = env;
  PTree::Encoding name = skip_cv(encode, e);
  if(name.empty()) return 0;

  if(IsBuiltInType())
    return PTree::first(name.make_ptree(0));

  if(name.front() == 'T')
  {
    name.pop();
    qname = name.make_name();
    head = GetQualifiedName(e, qname);
    if(head == 0)
      return qname;
    else
      return PTree::snoc(head, qname);
  }
  else if(name.front() == 'Q')
  {
    name.pop();
    qname = name.make_qname();
    head = GetQualifiedName(e, qname->car());
    return head ? PTree::nconc(head, qname) : qname;
  }
  else if(name.is_simple_name())
  {
    qname = name.make_name();
    head = GetQualifiedName(e, qname);
    return head ? PTree::snoc(head, qname) : qname;
  }
  else return 0;
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
  if(e) e = e->GetOuterEnvironment();
  for(; e != 0; e = e->GetOuterEnvironment())
  {
    c = e->IsClassEnvironment();
    if(c) qname = PTree::cons(c->Name(), PTree::cons(PTree::Encoding::scope, qname));
  }
  return qname;
}

PTree::Node *TypeInfo::MakePtree(PTree::Node *name)
{
  Normalize();
  if(metaobject != 0)
  {
    PTree::Node *decl;
    if(!name) decl = 0;
    else decl = PTree::list(name);
    return PTree::list(FullTypeName(), decl);
  }
  else if(!encode.empty()) encode.make_ptree(name);
  else return 0;
}

void TypeInfo::Normalize()
{
  if(encode.empty() || refcount > 0) return;

  Environment *e = env;
  PTree::Encoding ptr = encode;
  int r = refcount;

  while(r < 0)
    switch(ptr.front())
    {
      case 'C' :	// const
      case 'V' :	// volatile
	ptr.pop();
	break;
      case 'A' :	// array
      case 'P' :	// pointer *
      case 'R' :	// reference &
	ptr.pop();
	++r;
	break;
      case 'F' :	// function
      case 'M' :	// pointer to member ::*
	{
	  PTree::Encoding tmp(ptr.begin() + 1, ptr.end());
	  PTree::Encoding p = ptr.front() == 'F' ? get_return_type(tmp, e) : skip_name(tmp, e);
	  if(p.empty()) return;
	  else
	  {
	    ptr = p;
	    ++r;
	  }
	  break;
	}
      default :
	if(!resolve_typedef(e, ptr, true)) return;
    }
  while(resolve_typedef(e, ptr, false)) ; // empty
}

bool TypeInfo::resolve_typedef(Environment *&e, PTree::Encoding &ptr, bool resolvable)
{
  Bind *bind;
  Class *c;
  Environment *orig_e = e;
  PTree::Encoding name = ptr.get_base_name(e);
  if(!name.empty() && e && e->LookupType((const char *)&*name.begin(),
					 name.size(), bind))
    switch(bind->What())
    {
      case Bind::isTypedefName :
	ptr = bind->encoded_type();
	return true;
      case Bind::isClassName :
	c = bind->ClassMetaobject();
	if(!c) Set(ptr, orig_e);
	else if (name.front() == 'T') Set(ptr, c->GetEnvironment());
	else Set(c);
	return false;
      case Bind::isTemplateClass :
	c = bind->ClassMetaobject();
	if(!c) Set(ptr, orig_e);
	else Set(ptr, c->GetEnvironment());
	return false;
      default :
	break;
    }

  if(resolvable) Unknown();
  else Set(ptr, orig_e);
  return false;
}

PTree::Encoding TypeInfo::skip_cv(const PTree::Encoding &ptr, Environment *&e)
{
  if(ptr.empty()) return PTree::Encoding();
  PTree::Encoding remainder(ptr);
  while (remainder.front() == 'C' || remainder.front() == 'V') remainder.pop();
  while(true)
  {
    Bind *bind;
    int len;
    PTree::Encoding name = remainder.get_base_name(e);
    if(!name.empty() && e && e->LookupType((const char *)&*name.begin(), name.size(), bind))
      if(bind->What() != Bind::isTypedefName)
	return remainder;
      else
	remainder = bind->encoded_type();
    else
      return remainder;
  }
}

PTree::Encoding TypeInfo::skip_name(const PTree::Encoding &encode, Environment *e)
{
  if(!e) throw std::runtime_error("TypeInfo::skip_name(): nil environment");

  Environment *e2 = e;
  PTree::Encoding ptr = encode.get_base_name(e2);
  if (ptr.empty()) PTree::Encoding();
  else return PTree::Encoding(encode.begin() + ptr.size(), encode.end());
}

PTree::Encoding TypeInfo::get_return_type(const PTree::Encoding &encode, Environment *env)
{
  PTree::Encoding t = encode;
  while (true)
    switch(t.front())
    {
      case '_' : return PTree::Encoding(t.begin() + 1, t.end());
      case '\0' : return PTree::Encoding();
      default :
	t = skip_type(t, env);
	break;
    }
}

PTree::Encoding TypeInfo::skip_type(const PTree::Encoding &ptr, Environment *env)
{
  PTree::Encoding r = ptr;
  while(!r.empty())
    switch(r.front())
    {
      case '\0' :
      case '_' : PTree::Encoding();
      case 'S' :
      case 'U' :
      case 'C' :
      case 'V' :
      case 'P' :
      case 'R' :
      case 'A' :
	r.pop();
	break;
      case 'F' :
	r = get_return_type(PTree::Encoding(r.begin() + 1, r.end()), env);
	break;
      case 'T' :
      case 'Q' :
	return skip_name(r, env);
      case 'M' :
	r = skip_name(PTree::Encoding(r.begin() + 1, r.end()), env);
	break;
      default :
	if(r.front() < 0x80) return PTree::Encoding(r.begin() + 1, r.end());
	else return skip_name(r, env);
    }
  return r;
}
