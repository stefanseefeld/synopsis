//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <PTree.hh>
#include <TypeInfo.hh>
#include <Environment.hh>
#include <Class.hh>
#include <stdexcept>

TypeInfo::TypeInfo()
  : my_refcount(0),
    my_metaobject(0),
    my_env(0)
{
}

void TypeInfo::unknown()
{
  my_refcount = 0;
  my_encoding.clear();
  my_metaobject = 0;
  my_env = 0;
}

void TypeInfo::set(const PTree::Encoding &type, Environment* e)
{
  my_refcount = 0;
  my_encoding = type;
  my_metaobject = 0;
  my_env = e;
}

void TypeInfo::set(Class *m)
{
  my_refcount = 0;
  my_encoding = 0;
  my_metaobject = m;
  my_env = 0;
}

void TypeInfo::set_void()
{
  my_refcount = 0;
  my_encoding = "v";
  my_metaobject = 0;
  my_env = 0;
}

void TypeInfo::set_int()
{
  my_refcount = 0;
  my_encoding = "i";
  my_metaobject = 0;
  my_env = 0;
}

void TypeInfo::set_member(PTree::Node *member)
{
  Class *c = class_metaobject();
  if(!c) unknown();
  else
  {
    Environment *e = c->GetEnvironment();
    if(!e) unknown();
    else e->Lookup(member, *this);
  }
}

TypeInfoId TypeInfo::id()
{
  if(my_refcount > 0) return PointerType;

  normalize();
  if(my_metaobject) return ClassType;

  Environment *e = my_env;
  PTree::Encoding ptr = skip_cv(my_encoding, e);
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
	Class *c;
	t.set(ptr, e);
	if(t.is_class(c)) return ClassType;
	else if(t.is_enum()) return EnumType;
      }
      return UndefType;
  };
}

bool TypeInfo::is_no_return_type()
{
  normalize();
  Environment *e = my_env;
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  return(!ptr.empty() && *ptr.begin() == '?');
}

bool TypeInfo::is_const()
{
  normalize();
  return(!my_encoding.empty() && *my_encoding.begin() == 'C');
}

bool TypeInfo::is_volatile()
{
  normalize();
  if(my_encoding.empty()) return false;
  else if(my_encoding.front() == 'V') return true;
  else if(my_encoding.front() == 'C') return(my_encoding.at(1) == 'V');
  else return false;
}

size_t TypeInfo::is_builtin_type()
{
  normalize();
  Environment *e = my_env;
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  if(ptr.empty()) return 0;

  size_t result = 0;
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

bool TypeInfo::is_function()
{
  normalize();
  Environment *e = my_env;
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  return !ptr.empty() && ptr.front() == 'F';
}

bool TypeInfo::is_ellipsis()
{
  normalize();
  Environment *e = my_env;
  PTree::Encoding  ptr = skip_cv(my_encoding, e);
  return !ptr.empty() && ptr.front() == 'e';
}

bool TypeInfo::is_pointer_type()
{
  if(my_refcount > 0) return true;

  normalize();
  Environment* e = my_env;
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  if(!ptr.empty())
    return ptr.front() == 'P' || ptr.front() == 'A' || ptr.front() == 'M';
  return false;
}

bool TypeInfo::is_reference_type()
{
  normalize();
  Environment *e = my_env;
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  return !ptr.empty() && ptr.front() == 'R';
}

bool TypeInfo::is_array()
{
  normalize();
  Environment *e = my_env;
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  return !ptr.empty() && ptr.front() == 'A';
}

bool TypeInfo::is_pointer_to_member()
{
  normalize();
  Environment *e = my_env;
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  return !ptr.empty() && ptr.front() == 'M';
}

bool TypeInfo::is_template_class()
{
  normalize();
  Environment *e = my_env;
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  return !ptr.empty() && ptr.front() == 'T';
}

Class *TypeInfo::class_metaobject()
{
  Class *c;
  is_class(c);
  return c;
}

bool TypeInfo::is_class(Class *&c)
{
  normalize();
  if(my_metaobject != 0)
  {
    c = my_metaobject;
    return true;
  }
  else
  {
    c = 0;
    Environment *e = my_env;
    PTree::Encoding encode2 = skip_cv(my_encoding, e);
    if(my_encoding == encode2) return false;

    TypeInfo tinfo;
    tinfo.set(encode2, e);
    return tinfo.is_class(c);
  }
}

bool TypeInfo::is_enum()
{
  PTree::Node *spec;
  return is_enum(spec);
}

bool TypeInfo::is_enum(PTree::Node *&spec)
{
  spec = 0;
  normalize();
  if(my_metaobject) return false;
  else
  {
    Bind *bind;
    Environment *e = my_env;
    PTree::Encoding name = Environment::get_base_name(my_encoding, e);
    if(!name.empty() && e != 0)
      if(e->LookupType(name, bind))
	if(bind != 0 && bind->What() == Bind::isEnumName)
	{
	  spec = ((BindEnumName*)bind)->GetSpecification();
	  return true;
	}
    e = my_env;
    name = skip_cv(my_encoding, e);
    if(name == my_encoding) return false;
    TypeInfo tinfo;
    tinfo.set(name, e);
    return tinfo.is_enum(spec);
  }
}

void TypeInfo::dereference(TypeInfo &t)
{
  t.my_refcount = my_refcount - 1;
  t.my_encoding = my_encoding;
  t.my_metaobject = my_metaobject;
  t.my_env = my_env;
}

void TypeInfo::reference(TypeInfo &t)
{
  t.my_refcount = my_refcount + 1;
  t.my_encoding = my_encoding;
  t.my_metaobject = my_metaobject;
  t.my_env = my_env;
}

bool TypeInfo::nth_argument(int n, TypeInfo &t)
{
  Environment *e = my_env;
  normalize();
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  if(ptr.empty() || ptr.front() != 'F')
  {
    t.unknown();
    return false;
  }

  ptr.pop();
  if(ptr.front() == 'v')
  {
    t.set_void();
    return false;		// no arguments
  }

  while(n-- > 0)
  {
    ptr = skip_type(ptr, e);
    if(ptr.empty() || ptr.front() == '_')
    {
      t.unknown();
      return false;
    }
  }

  t.set(ptr, e);
  return true;
}

int TypeInfo::num_of_arguments()
{
  Environment *e = my_env;
  normalize();
  PTree::Encoding ptr = skip_cv(my_encoding, e);
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

bool TypeInfo::nth_template_argument(int n, TypeInfo &t)
{
  Environment *e = my_env;
  normalize();
  PTree::Encoding ptr = skip_cv(my_encoding, e);
  if(ptr.empty() || ptr.front() != 'T')
  {
    t.unknown();
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
      t.unknown();
      return false;
    }
  }
  t.set(ptr, e);
  return true;
}

PTree::Node *TypeInfo::full_type_name()
{
  normalize();
  if(my_metaobject != 0)
  {
    PTree::Node *qname = my_metaobject->Name();
    PTree::Node *head = get_qualified_name2(my_metaobject);
    return head ? PTree::snoc(head, qname) : qname;
  }

  Environment *e = my_env;
  PTree::Encoding name = skip_cv(my_encoding, e);
  if(name.empty()) return 0;

  if(is_builtin_type())
    return PTree::first(name.make_ptree(0));

  if(name.front() == 'T')
  {
    name.pop();
    PTree::Node *qname = name.make_name();
    PTree::Node *head = get_qualified_name(e, qname);
    return head ? PTree::snoc(head, qname) : qname;
  }
  else if(name.front() == 'Q')
  {
    name.pop();
    PTree::Node *qname = name.make_qname();
    PTree::Node *head = get_qualified_name(e, qname->car());
    return head ? PTree::nconc(head, qname) : qname;
  }
  else if(name.is_simple_name())
  {
    PTree::Node *qname = name.make_name();
    PTree::Node *head = get_qualified_name(e, qname);
    return head ? PTree::snoc(head, qname) : qname;
  }
  else return 0;
}

PTree::Node *TypeInfo::get_qualified_name(Environment *e, PTree::Node *tname)
{
  Class *c = e->LookupClassMetaobject(tname);
  return c ? get_qualified_name2(c) : 0;
}

PTree::Node *TypeInfo::get_qualified_name2(Class *c)
{
  PTree::Node *qname = 0;
  Environment *e = c->GetEnvironment();
  if(e) e = e->GetOuterEnvironment();
  for(; e != 0; e = e->GetOuterEnvironment())
  {
    c = e->IsClassEnvironment();
    if(c) qname = PTree::cons(c->Name(), PTree::cons(PTree::Encoding::scope, qname));
  }
  return qname;
}

PTree::Node *TypeInfo::make_ptree(PTree::Node *name)
{
  normalize();
  if(my_metaobject != 0)
  {
    PTree::Node *decl;
    if(!name) decl = 0;
    else decl = PTree::list(name);
    return PTree::list(full_type_name(), decl);
  }
  else if(!my_encoding.empty()) my_encoding.make_ptree(name);
  else return 0;
}

void TypeInfo::normalize()
{
  if(my_encoding.empty() || my_refcount > 0) return;

  Environment *e = my_env;
  PTree::Encoding ptr = my_encoding;
  int r = my_refcount;

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
  PTree::Encoding name = Environment::get_base_name(ptr, e);
  if(!name.empty() && e && e->LookupType(name, bind))
    switch(bind->What())
    {
      case Bind::isTypedefName :
	ptr = bind->encoded_type();
	return true;
      case Bind::isClassName :
	c = bind->ClassMetaobject();
	if(!c) set(ptr, orig_e);
	else if (name.front() == 'T') set(ptr, c->GetEnvironment());
	else set(c);
	return false;
      case Bind::isTemplateClass :
	c = bind->ClassMetaobject();
	if(!c) set(ptr, orig_e);
	else set(ptr, c->GetEnvironment());
	return false;
      default :
	break;
    }

  if(resolvable) unknown();
  else set(ptr, orig_e);
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
    PTree::Encoding name = Environment::get_base_name(remainder, e);
    if(!name.empty() && e && e->LookupType(name, bind))
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
  PTree::Encoding ptr = Environment::get_base_name(encode, e2);
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
