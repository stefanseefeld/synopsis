//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <PTree/Display.hh>
#include <PTree/Writer.hh>
#include <PTree/Symbol.hh>
#include <PTree/ConstEvaluator.hh>
#include <cassert>

using namespace PTree;

Scope::~Scope()
{
}

void Scope::declare(const Encoding &name, const Symbol *s)
{
  my_symbols[name] = s;
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
  const Encoding &name = spec->encoded_name();
  const Encoding &type = spec->encoded_type();
  if(tag && tag->is_atom()) declare(name, new TypeName(type, spec));
  // else it's an anonymous enum

  Node *body = third(spec);
  // The numeric value of an enumerator is either specified
  // by an explicit initializer or it is determined by incrementing
  // by one the value of the previous enumerator.
  // The default value for the first enumerator is 0
  long value = -1;
  for (Node *e = second(body); e; e = rest(rest(e)))
  {
    Node *enumerator = e->car();
    if (enumerator->is_atom()) ++value;
    else  // [identifier = initializer]
    {
      Node *initializer = third(enumerator);
      enumerator = enumerator->car();
      if (!evaluate_const(initializer, *this, value))
      {
	std::cerr << "Error in evaluating enum initializer :\n"
		  << "Expression doesn't evaluate to a constant integral value" << std::endl;
      }
    }
    assert(enumerator->is_atom());
    declare(Encoding(enumerator->position(), enumerator->length()),
	    new ConstName(type, value, enumerator));
  }
}

void Scope::declare(ClassSpec *spec)
{
  const Encoding &name = spec->encoded_name();
  declare(name, new TypeName(spec->encoded_type(), spec));
}

void Scope::declare(TemplateDecl *, ClassSpec *)
{
}

void Scope::declare(TemplateDecl *, Node *)
{
}

const Symbol *Scope::lookup(const Encoding &id) const throw()
{
  SymbolTable::const_iterator i = my_symbols.find(id);
  return i == my_symbols.end() ? 0 : i->second;
}

void Scope::dump(std::ostream &os) const
{
  os << "Scope::dump:" << std::endl;
  for (SymbolTable::const_iterator i = my_symbols.begin(); i != my_symbols.end(); ++i)
  {
    if (const VariableName *variable = dynamic_cast<const VariableName *>(i->second))
      os << "Variable: " << i->first << ' ' << variable->type() << std::endl;
    else if (const ConstName *const_ = dynamic_cast<const ConstName *>(i->second))
      os << "Const:    " << i->first << ' ' << const_->type() << " (" << const_->value() << ')' << std::endl;
    else if (const TypeName *type = dynamic_cast<const TypeName *>(i->second))
      os << "Type: " << i->first << ' ' << type->type() << std::endl;
    else // shouldn't get here
      os << "Symbol: " << i->first << ' ' << i->second->type() << std::endl;
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

  if(scope)
  {
    const Symbol *symbol = scope->lookup(Encoding((const char*)&*(i + 1), m));
    // FIXME !! (see Environment)
    if (symbol) return m + (*(i + m + 1) - 0x80) + 2;
  }
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

const Symbol *NestedScope::lookup(const Encoding &name) const throw()
{
  const Symbol *symbol = Scope::lookup(name);
  return symbol ? symbol : my_outer->lookup(name);
}

void NestedScope::dump(std::ostream &os) const
{
  os << "NestedScope::dump:" << std::endl;
  Scope::dump(os);
  my_outer->dump(os);
}
