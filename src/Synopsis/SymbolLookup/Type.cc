//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolLookup/Type.hh>
#include <stdexcept>

using namespace Synopsis;
using namespace SymbolLookup;

Type::Type()
  : my_refcount(0),
    my_scope(0)
{
}

Type::Type(PTree::Encoding const & type, Scope const * s)
  : my_refcount(0),
    my_encoding(type),
    my_scope(s)
{
}

void Type::unknown()
{
  my_refcount = 0;
  my_encoding.clear();
  my_scope = 0;
}

void Type::set(PTree::Encoding const & type, Scope const * s)
{
  my_refcount = 0;
  my_encoding = type;
  my_scope = s;
}

void Type::set_void()
{
  my_refcount = 0;
  my_encoding = "v";
  my_scope = 0;
}

void Type::set_int()
{
  my_refcount = 0;
  my_encoding = "i";
  my_scope = 0;
}

void Type::set_member(PTree::Node *member)
{
//   if(!c) unknown();
//   else
//   {
//     Scope *e = c->GetScope();
//     if(!e) unknown();
//     else e->Lookup(member, *this);
//   }
}

Type::Kind Type::id()
{
  if(my_refcount > 0) return POINTER;

  normalize();

  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  if(name.empty()) return UNDEF;

  switch(*name.begin())
  {
    case 'T' : return TEMPLATE;
    case 'P' : return POINTER;
    case 'R' : return REFERENCE;
    case 'M' : return POINTER_TO_MEMBER;
    case 'A' : return ARRAY;
    case 'F' : return FUNCTION;
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
    case 'v' : return BUILTIN;
    default :
      if(*name.begin() == 'Q' || *name.begin() >= 0x80)
      {
	Type t;
	t.set(name, scope);
 	if(t.is_class()) return CLASS;
	else if(t.is_enum()) return ENUM;
      }
      return UNDEF;
  };
}

bool Type::is_no_return_type()
{
  normalize();
  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  return !name.empty() && *name.begin() == '?';
}

bool Type::is_const()
{
  normalize();
  return(!my_encoding.empty() && *my_encoding.begin() == 'C');
}

bool Type::is_volatile()
{
  normalize();
  if(my_encoding.empty()) return false;
  else if(my_encoding.front() == 'V') return true;
  else if(my_encoding.front() == 'C') return(my_encoding.at(1) == 'V');
  else return false;
}

size_t Type::is_builtin_type()
{
  normalize();
  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  if(name.empty()) return 0;

  size_t result = 0;
  for(PTree::Encoding::iterator i = name.begin(); i != name.end(); ++i)
  {
    switch(*i)
    {
      case 'b' : return result | BOOLEAN;
      case 'c' : return result | CHAR;
      case 'w' : return result | WIDE_CHAR;
      case 'i' : return result | INT;
      case 's' : return result | SHORT;
      case 'l' : return result | LONG;
      case 'j' : return result | LONG_LONG;
      case 'f' : return result | FLOAT;
      case 'd' : return result | DOUBLE;
      case 'r' : return result | LONG_DOUBLE;
      case 'v' : return result | VOID;
      case 'S' :
	result |= SIGNED;
	break;
      case 'U' :
	result |= UNSIGNED;
	break;
      default : return 0;
    }
  }
}

bool Type::is_function()
{
  normalize();
  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  return !name.empty() && name.front() == 'F';
}

bool Type::is_ellipsis()
{
  normalize();
  Scope const *scope = my_scope;
  PTree::Encoding  name = skip_cv(my_encoding, scope);
  return !name.empty() && name.front() == 'e';
}

bool Type::is_pointer_type()
{
  if(my_refcount > 0) return true;

  normalize();
  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  if(!name.empty())
    return name.front() == 'P' || name.front() == 'A' || name.front() == 'M';
  return false;
}

bool Type::is_reference_type()
{
  normalize();
  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  return !name.empty() && name.front() == 'R';
}

bool Type::is_array()
{
  normalize();
  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  return !name.empty() && name.front() == 'A';
}

bool Type::is_pointer_to_member()
{
  normalize();
  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  return !name.empty() && name.front() == 'M';
}

bool Type::is_template_class()
{
  normalize();
  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  return !name.empty() && name.front() == 'T';
}

bool Type::is_class()
{
  normalize();
//   if(my_metaobject != 0)
//   {
//     c = my_metaobject;
//     return true;
//   }
//   else
//   {
//     c = 0;
//     Scope const *e = my_env;
//     PTree::Encoding encode2 = skip_cv(my_encoding, e);
//     if(my_encoding == encode2) return false;

//     Type tinfo;
//     tinfo.set(encode2, e);
//     return tinfo.is_class(c);
//   }
  return false;
}

bool Type::is_enum()
{
  PTree::Node *spec;
  return is_enum(spec);
}

bool Type::is_enum(PTree::Node *&spec)
{
  // FIXME: TBD
  assert(0);
  return false;
  spec = 0;
  normalize();
//   if(my_metaobject) return false;
//   else
//   {
//     Scope const *scope = my_scope;
//     PTree::Encoding name = Scope::get_base_name(my_encoding, scope);
//     if(!name.empty() && scope)
//     {
//     Symbol *symbol;
//       if(scope->LookupType(name, bind))
// 	if(bind != 0 && bind->What() == Bind::isEnumName)
// 	{
// 	  spec = ((BindEnumName*)bind)->GetSpecification();
// 	  return true;
// 	}
//     }
//     scope = my_scope;
//     name = skip_cv(my_encoding, scope);
//     if(name == my_encoding) return false;
//     Type type;
//     type.set(name, scope);
//     return type.is_enum(spec);
//   }
}

int Type::num_of_arguments()
{
  Scope const *scope = my_scope;
  normalize();
  PTree::Encoding name = skip_cv(my_encoding, scope);
  if(name.empty() || name.front() != 'F') return -1; // not a function
  
  name.pop();
  if(name.front() == 'v') return 0; // no arguments

  size_t n = 0;
  while(true)
  {
    ++n;
    name = skip_type(name, scope);
    if(name.empty() || name.front() == '_') return n;
  }
}

bool Type::nth_argument(int n, Type &t)
{
  Scope const *scope = my_scope;
  normalize();
  PTree::Encoding name = skip_cv(my_encoding, scope);
  if(name.empty() || name.front() != 'F')
  {
    t.unknown();
    return false;
  }

  name.pop();
  if(name.front() == 'v')
  {
    t.set_void();
    return false;		// no arguments
  }

  while(n-- > 0)
  {
    name = skip_type(name, scope);
    if(name.empty() || name.front() == '_')
    {
      t.unknown();
      return false;
    }
  }

  t.set(name, scope);
  return true;
}

bool Type::nth_template_argument(int n, Type &t)
{
  Scope const *scope = my_scope;
  normalize();
  PTree::Encoding name = skip_cv(my_encoding, scope);
  if(name.empty() || name.front() != 'T')
  {
    t.unknown();
    return false;
  }
  name.pop();
  name = name.get_template_arguments();
  while(n-- > 0)
  {
    name = skip_type(name, scope);
    // DANGER !!! : the original code reads:
    // if (ptr == 0 || ptr >= end)
    // see the history if you get into trouble here...
    if(name.empty())
    {
      t.unknown();
      return false;
    }
  }
  t.set(name, scope);
  return true;
}

PTree::Node *Type::full_type_name()
{
  normalize();
//   if(my_metaobject != 0)
//   {
//     PTree::Node *qname = my_metaobject->Name();
//     PTree::Node *head = get_qualified_name2(my_metaobject);
//     return head ? PTree::snoc(head, qname) : qname;
//   }

  Scope const *scope = my_scope;
  PTree::Encoding name = skip_cv(my_encoding, scope);
  if(name.empty()) return 0;

  if(is_builtin_type())
    return PTree::first(name.make_ptree(0));

  if(name.front() == 'T')
  {
    name.pop();
    PTree::Node *qname = name.make_name();
    PTree::Node *head = get_qualified_name(scope, qname);
    return head ? PTree::snoc(head, qname) : qname;
  }
  else if(name.front() == 'Q')
  {
    name.pop();
    PTree::Node *qname = name.make_qname();
    PTree::Node *head = get_qualified_name(scope, qname->car());
    return head ? PTree::nconc(head, qname) : qname;
  }
  else if(name.is_simple_name())
  {
    PTree::Node *qname = name.make_name();
    PTree::Node *head = get_qualified_name(scope, qname);
    return head ? PTree::snoc(head, qname) : qname;
  }
  else return 0;
}

PTree::Node *Type::get_qualified_name(Scope const *scope, PTree::Node *tname)
{
//   Class *c = e->LookupClassMetaobject(tname);
//   return c ? get_qualified_name2(c) : 0;
  return 0;
}

PTree::Node *Type::make_ptree(PTree::Node *name)
{
  normalize();
//   if(my_metaobject != 0)
//   {
//     PTree::Node *decl;
//     if(!name) decl = 0;
//     else decl = PTree::list(name);
//     return PTree::list(full_type_name(), decl);
//   }
//   else if(!my_encoding.empty()) my_encoding.make_ptree(name);
//   else return 0;
  return 0;
}

void Type::normalize()
{
  if(my_encoding.empty() || my_refcount > 0) return;
  Scope const *scope = my_scope;
  PTree::Encoding name = my_encoding;
  int r = my_refcount;

  while(r < 0)
    switch(name.front())
    {
      case 'C': // const
      case 'V': // volatile
	name.pop();
	break;
      case 'A': // array
      {
	char c = 'A';
	do
	{
	  c = name.front();
	  name.pop();
	} while (c != '_');
	++r;
	break;
      }
      case 'P': // pointer *
      case 'R': // reference &
	name.pop();
	++r;
	break;
      case 'F': // function
      case 'M': // pointer to member ::*
      {
	PTree::Encoding tmp(name.begin() + 1, name.end());
	PTree::Encoding p = (name.front() == 'F' ? 
			     get_return_type(tmp, scope) :
			     skip_name(tmp, scope));
	if(p.empty()) return;
	else
	{
	  name = p;
	  ++r;
	}
	break;
      }
      default:
	if(!resolve_typedef(scope, name, true)) return;
    }
  while(resolve_typedef(scope, name, false)) ; // empty
}

bool Type::resolve_typedef(Scope const *&scope, PTree::Encoding &name, bool resolvable)
{
  if (name.front() > 0x80 ||
      name.front() == 'Q' ||
      name.front() == 'T') // user defined type ?
  {
    TypeName const *tn = scope->lookup<TypeName>(name);
    if (TypedefName const *td = dynamic_cast<TypedefName const *>(tn))
    {
      name = td->type();
      scope = td->scope();
      return true;
    }
    else // not found
    {
      if(resolvable) unknown();
      else set(name, scope);
    }
  }
  else set(name, scope);

  return false;
}

PTree::Encoding Type::skip_cv(PTree::Encoding const & name, Scope const *& scope)
{
  if(name.empty()) return PTree::Encoding();
  PTree::Encoding remainder(name);
  while (remainder.front() == 'C' || remainder.front() == 'V') remainder.pop();
  while(true)
  {
    if(!name.empty() && scope)
    {
      TypeName const *tn = scope->lookup<TypeName>(name);
      if (TypedefName const *td = dynamic_cast<TypedefName const *>(tn))
	remainder = td->type();
      else
	return remainder;
    }
    else
      return remainder;
  }
}

PTree::Encoding Type::skip_name(PTree::Encoding const & encode, Scope const * scope)
{
  // FIXME: TBD
  std::cout << "skip name " << encode << std::endl;
  assert(0);
//   if(!scope) throw std::runtime_error("Type::skip_name(): nil environment");

//   Scope const *scope2 = scope;
//   PTree::Encoding name = Scope::get_base_name(encode, scope2);
//   if (name.empty()) PTree::Encoding();
//   else return PTree::Encoding(encode.begin() + name.size(), encode.end());
}

PTree::Encoding Type::get_return_type(PTree::Encoding const & encode, Scope const * scope)
{
  PTree::Encoding t = encode;
  while (true)
    switch(t.front())
    {
      case '_' : return PTree::Encoding(t.begin() + 1, t.end());
      case '\0' : return PTree::Encoding();
      default :
	t = skip_type(t, scope);
	break;
    }
}

PTree::Encoding Type::skip_type(PTree::Encoding const & name, Scope const * scope)
{
  PTree::Encoding r = name;
  while(!r.empty())
    switch(r.front())
    {
      case '\0' :
      case '_' : return PTree::Encoding();
      case 'S' :
      case 'U' :
      case 'C' :
      case 'V' :
      case 'P' :
      case 'R' :
      case 'A' :
      {
	char c = 'A';
	do
	{
	  c = r.front();
	  r.pop();
	} while (c != '_');
	break;
      }
      case 'F' :
	r = get_return_type(PTree::Encoding(r.begin() + 1, r.end()), scope);
	break;
      case 'T' :
      case 'Q' :
	return skip_name(r, scope);
      case 'M' :
	r = skip_name(PTree::Encoding(r.begin() + 1, r.end()), scope);
	break;
      default :
	if(r.front() < 0x80) return PTree::Encoding(r.begin() + 1, r.end());
	else return skip_name(r, scope);
    }
  return r;
}

namespace Synopsis
{
namespace SymbolLookup
{

std::ostream &operator << (std::ostream &os, Type const &t)
{
  os << "Type<" << t.my_refcount << ' ' << t.my_encoding << '>';
  return os;
}

}
}
