//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <PTree/Display.hh>
#include <PTree/Writer.hh>
#include <PTree/Symbol.hh>
#include <cassert>

using namespace PTree;

Scope::~Scope()
{
}

void Scope::declare(const Encoding &id, const Symbol &s)
{
  my_symbols[id] = s;
}

void Scope::declare(Declaration *d)
{
  Node *decls = third(d);
  if(is_a(decls, Token::ntDeclarator))	// if it is a function
  {
    //...
  }
  else // variable
  {
    //...
  }
}

void Scope::declare(Typedef *td)
{
  Node *declarations = third(td);
  while(declarations)
  {
    Node *d = declarations->car();
    if(type_of(d) == Token::ntDeclarator)
    {
//       Encoding name = d->encoded_name();
//       Encoding type = d->encoded_type();
//       if(!name.empty() && !type.empty())
//       {
// 	Environment *e = this;
// 	Encoding base = Environment::get_base_name(name, e);
// 	if(!base.empty()) my_symbols.insert(base, Symbol(Symbol::TYPE, d));
//       }
    }
    declarations = tail(declarations, 2);
  }
}

void Scope::declare(EnumSpec *spec)
{
  Node *tag = second(spec);
  Encoding encoding = spec->encoded_name();
  if(tag && tag->is_atom()) declare(encoding, Symbol(Symbol::TYPE, spec));
  // else it's an anonymous enum

  Node *body = third(spec);
  display(body, std::cout, true);
  for (Node *e = second(body); e; e = rest(rest(e)))
  {
    Node *enumerator = e->car();
    if (!enumerator->is_atom()) // 'identifier = initializer'
      enumerator = enumerator->car();
    assert(enumerator->is_atom());
    declare(Encoding(enumerator->position(), enumerator->length()),
	    Symbol(Symbol::CONST, enumerator));
  }
}

void Scope::declare(ClassSpec *spec)
{
  Encoding encoding = spec->encoded_name();
  declare(encoding, Symbol(Symbol::TYPE, spec));
}

void Scope::declare(TemplateDecl *, ClassSpec *)
{
}

void Scope::declare(TemplateDecl *, Node *)
{
}

Symbol::Type Scope::lookup(const Encoding &id, Symbol &s) const
{
  SymbolTable::const_iterator i = my_symbols.find(id);
  if (i != my_symbols.end())
  {
    s = i->second;
    return s.type;
  }
  else return Symbol::NONE;
}

void Scope::dump(std::ostream &os) const
{
  for (SymbolTable::const_iterator i = my_symbols.begin(); i != my_symbols.end(); ++i)
  {
    switch (i->second.type)
    {
      case Symbol::VARIABLE:
	os << "Variable : " << reify(i->second.ptree);
	break;
      case Symbol::CONST:
	os << "Const    : " << reify(i->second.ptree);
	break;
      case Symbol::TYPE:
	os << "Type     : " << reify(i->second.ptree);
	break;
    }
  }  
}

//. get_base_name() returns "Foo" if ENCODE is "Q[2][1]X[3]Foo", for example.
//. If an error occurs, the function returns 0.
Encoding Scope::get_base_name(const Encoding &enc, const Scope *&scope)
{
  if(enc.empty()) return enc;
  const Scope *s = scope;
  Encoding::iterator i = enc.begin();
  if(*i == 'Q')
  {
    int n = *(i + 1) - 0x80;
    i += 2;
    while(n-- > 1)
    {
      int m = *i++;
      if(m == 'T') m = get_base_name_if_template(i, s);
      else if(m < 0x80) return Encoding(); // error?
      else
      {	 // class name
	m -= 0x80;
	if(m <= 0)
	{		// if global scope (e.g. ::Foo)
	  if(s) s = s->global();
	}
	else s = lookup_typedef_name(i, m, s);
      }
      i += m;
    }
    scope = s;
  }
  if(*i == 'T')
  {		// template class
    int m = *(i + 1) - 0x80;
    int n = *(i + m + 2) - 0x80;
    return Encoding(i, i + m + n + 3);
  }
  else if(*i < 0x80) return Encoding();
  else return Encoding(i + 1, i + 1 + size_t(*i - 0x80));
}

int Scope::get_base_name_if_template(Encoding::iterator i, const Scope *&scope)
{
  int m = *i - 0x80;
  if(m <= 0) return *(i+1) - 0x80 + 2;

  Symbol symbol;
  if(scope != 0 && 
     scope->lookup(Encoding((const char*)&*(i + 1), m), symbol) != Symbol::NONE)
    // FIXME !! (see Environment)
    return m + (*(i + m + 1) - 0x80) + 2;

  // the template name was not found.
  scope = 0;
  return m + (*(i + m + 1) - 0x80) + 2;
}

const Scope *Scope::lookup_typedef_name(Encoding::iterator i, size_t s,
					const Scope *scope)
{
//   TypeInfo tinfo;
//   Bind *bind;
//   Class *c = 0;

  if(scope)
    ;
//     if (scope->LookupType(Encoding((const char *)&*i, s), bind) && bind)
//       switch(bind->What())
//       {
//         case Bind::isClassName :
// 	  c = bind->ClassMetaobject();
// 	  break;
//         case Bind::isTypedefName :
// 	  bind->GetType(tinfo, env);
// 	  c = tinfo.class_metaobject();
// 	  /* if (c == 0) */
// 	  env = 0;
// 	  break;
//         default :
// 	  break;
//       }
//     else if (env->LookupNamespace(Encoding((const char *)&*i, s)))
//     {
//       /* For the time being, we simply ignore name spaces.
//        * For example, std::T is identical to ::T.
//        */
//       env = env->GetBottom();
//     }
//     else env = 0; // unknown typedef name

//   return c ? c->GetEnvironment() : env;
  return 0;
}

Symbol::Type NestedScope::lookup(const Encoding &id, Symbol &s) const
{
  if (!Scope::lookup(id, s)) return my_outer->lookup(id, s);
  else return s.type;
}

void NestedScope::dump(std::ostream &os) const
{
  Scope::dump(os);
  std::cout << "outer scope:" << std::endl;
  my_outer->dump(os);
}
