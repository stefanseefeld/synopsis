//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <PTree.hh>
#include <PTree/Display.hh>
#include "Environment.hh"
#include <Walker.hh>
#include <TypeInfo.hh>
#include <Class.hh>
#include <MetaClass.hh>
#include <Parser.hh>
#include <Member.hh>
#include <stdexcept>
#include <iostream>

Parser* Walker::default_parser = 0;
const char* Walker::argument_name = "_arg_%d_";
const char* Walker::default_metaclass = 0;

Walker::Walker(Parser* p)
  : my_result(0)
{
    env = new Environment(this);
    parser = p;
    if(default_parser == 0)
	default_parser = p;
}

Walker::Walker(Parser* p, Environment* e)
  : my_result(0)
{
    env = new Environment(e, this);
    parser = p;
    if(default_parser == 0)
	default_parser = p;
}

Walker::Walker(Environment* e)
  : my_result(0)
{
    env = new Environment(e, this);
    if(!default_parser)
      throw std::runtime_error("Walker::Walker(): no default parser");

    parser = default_parser;
}

Walker::Walker(Walker* w)
  : my_result(0)
{
    env = w->env;
    parser = w->parser;
}

void Walker::new_scope()
{
    env = new Environment(env);
}

void Walker::new_scope(Class* metaobject)
{
    env = new Environment(env);
    if(metaobject != 0)
	metaobject->SetEnvironment(env);
}

Environment* Walker::exit_scope()
{
    Environment* old_env = env;
    env = old_env->GetOuterEnvironment();
    return old_env;
}

void Walker::RecordBaseclassEnv(PTree::Node *bases)
{
    while(bases != 0){
	bases = bases->cdr();		// skip : or ,
	PTree::Node *base_class = PTree::last(bases->car())->car();
	Class* metaobject = env->LookupClassMetaobject(base_class);
	if(metaobject != 0){
	    Environment* e = metaobject->GetEnvironment();
	    if(e != 0)
		env->AddBaseclassEnv(e);
	}

	bases = bases->cdr();
    }
}

Walker::NameScope Walker::change_scope(Environment* e)
{
    NameScope scope;
    scope.walker = e->GetWalker();
    e->SetWalker(this);
    scope.env = env;
    env = e;
    return scope;
}

void Walker::restore_scope(Walker::NameScope& scope)
{
    env->SetWalker(scope.walker);
    env = scope.env;
}

PTree::Node *Walker::translate(PTree::Node *p)
{
  if(p == 0) return p;
  p->accept(this);
  PTree::Node *result = my_result;
  my_result = 0;
  return result;
}

void Walker::visit(PTree::Typedef *node)
{
  PTree::Node *tspec, *tspec2;

  tspec = PTree::second(node);
  tspec2 = translate_type_specifier(tspec);
  env->RecordTypedefName(PTree::third(node));
  if(tspec == tspec2)
    my_result = node;
  else
    my_result = new PTree::Typedef(PTree::first(node),
				   PTree::list(tspec2, PTree::tail(node, 2)));
}

void Walker::visit(PTree::TemplateDecl *node)
{
  PTree::Node *body = PTree::nth(node, 4);
  PTree::ClassSpec *class_spec = get_class_template_spec(body);
  if (class_spec) my_result = translate_template_class(node, class_spec);
  else my_result = translate_template_function(node, body);
}

PTree::TemplateDecl *Walker::translate_template_class(PTree::TemplateDecl *decl,
						      PTree::ClassSpec *class_spec)
{
  PTree::Node *userkey;
  PTree::Node *class_def;

  if(class_spec->car()->is_atom())
  {
    userkey = 0;
    class_def = class_spec;
  }
  else
  {
    userkey = class_spec->car();
    class_def = class_spec->cdr();
  }

  Class *metaobject = 0;
  if(PTree::length(class_def) == 4)
    metaobject = make_template_class_metaobject(decl, userkey, class_def);

  env->RecordTemplateClass(class_spec, metaobject);
  PTree::ClassSpec *class_spec2 = translate_class_spec(class_spec,
						       userkey,
						       class_def,
						       metaobject);
  if(class_spec == class_spec2)
    return decl;
  else
    return new PTree::TemplateDecl(decl->car(),
				   PTree::subst(class_spec2, class_spec,
						decl->cdr()));
}

Class* Walker::make_template_class_metaobject(PTree::Node *def,
					      PTree::Node *userkey,
					      PTree::Node *class_def)
{
  Class* metaobject = LookupMetaclass(def, userkey, class_def, true);
  if(!metaobject)
    metaobject = new TemplateClass;
  else
    if(metaobject->AcceptTemplate())
      return metaobject;
    else
    {
      ErrorMessage("the specified metaclass is not for templates.",
		   0, def);
      metaobject = new TemplateClass;
    }

  metaobject->InitializeInstance(def, 0);
  return metaobject;
}

PTree::TemplateDecl *Walker::translate_template_function(PTree::TemplateDecl *decl,
							 PTree::Node *fun)
{
    env->RecordTemplateFunction(decl, fun);
    return decl;
}

void Walker::visit(PTree::TemplateInstantiation *node)
{
  PTree::Node *userkey;
  PTree::ClassSpec *class_spec;
  PTree::Node *full_class_spec = PTree::first(node);

  if(full_class_spec->car()->is_atom())
  {
    userkey = 0;
    class_spec = dynamic_cast<PTree::ClassSpec *>(full_class_spec);
  }
  else
  {
    userkey = full_class_spec->car();
    class_spec = dynamic_cast<PTree::ClassSpec *>(full_class_spec->cdr());
  }
  assert(class_spec);
  Class *meta = make_template_instantiation_metaobject(full_class_spec,
						       userkey, class_spec);
  my_result = translate_template_instantiation(node, userkey, class_spec, meta);
}

Class *Walker::make_template_instantiation_metaobject(PTree::Node *full_class_spec,
						      PTree::Node *userkey,
						      PTree::ClassSpec *class_spec)
{
  // [class [foo [< ... >]]] -> [class foo]
  PTree::Node *class_name = PTree::first(PTree::second(class_spec));
  Bind* binding = 0;
  if (!env->Lookup(class_name,binding))
    return 0;

  Class* metaobject = 0;
  if (binding->What() != Bind::isTemplateClass) 
  {
    ErrorMessage("not declarated as a template class?!?",
		 0, full_class_spec);
    metaobject = 0;
  }
  else
    metaobject = binding->ClassMetaobject();

  if (metaobject == 0)
    metaobject = new TemplateClass;
  else
    if(metaobject->AcceptTemplate())
      return metaobject;
    else
    {
      ErrorMessage("the specified metaclass is not for templates.",
		   0, full_class_spec);
      metaobject = new TemplateClass;
    }

  return metaobject;
}

PTree::Node *Walker::translate_template_instantiation(PTree::TemplateInstantiation *inst_spec,
						      PTree::Node *userkey,
						      PTree::ClassSpec *class_spec,
						      Class *metaobject)
{
  if (!metaobject) return inst_spec;
  else 
  {
    class_spec->accept(this);
    PTree::Node *class_spec2 = dynamic_cast<PTree::ClassSpec *>(my_result);
    if (class_spec == class_spec2)
      return inst_spec;
    else
      return class_spec2;
  }
}

void Walker::visit(PTree::MetaclassDecl *node)
{
  env->RecordMetaclassName(node);
  my_result = node;
}

void Walker::visit(PTree::LinkageSpec *node)
{
  PTree::Node *body = PTree::third(node);
  PTree::Node *body2 = translate(body);
  if(body == body2)
    my_result = node;
  else
    my_result = new PTree::LinkageSpec(PTree::first(node),
				       PTree::list(PTree::second(node), body2));
}

void Walker::visit(PTree::NamespaceSpec *node)
{
  PTree::Node *body = PTree::third(node);
  PTree::Node *body2 = translate(body);
  env->RecordNamespace(PTree::second(node));
  if(body == body2)
    my_result = node;
  else
    my_result = new PTree::NamespaceSpec(PTree::first(node),
					 PTree::list(PTree::second(node), body2));
}

void Walker::visit(PTree::NamespaceAlias *node)
{
  my_result = node;
}

void Walker::visit(PTree::Using *node)
{
  my_result = node;
}

void Walker::visit(PTree::Declaration *node)
{
  PTree::Node *decls = PTree::third(node);
  if(PTree::is_a(decls, Token::ntDeclarator))	// if it is a function
    my_result = translate_function_implementation(node);
  else
  {
    // if it is a function prototype or a variable declaration.
    PTree::Node *decls2;
    PTree::Node *sspec = PTree::first(node);
    PTree::Node *sspec2 = translate_storage_specifiers(sspec);
    PTree::Node *tspec = PTree::second(node);
    PTree::Node *tspec2 = translate_type_specifier(tspec);
    if(decls->is_atom())	// if it is ";"
      decls2 = decls;
    else
      decls2 = translate_declarators(decls);

    if(sspec == sspec2 && tspec == tspec2 && decls == decls2)
      my_result = node;
    else if(!decls2)
      my_result = new PTree::Declaration(0, PTree::list(0, Class::semicolon_t));
    else
      my_result = new PTree::Declaration(sspec2,
					 PTree::shallow_subst(tspec2, tspec,
							      decls2, decls,
							      node->cdr()));
  }
}

PTree::Node *Walker::translate_declarators(PTree::Node *decls)
{
  return translate_declarators(decls, true);
}

PTree::Node *Walker::translate_declarators(PTree::Node *decls, bool record)
{
  PTree::Array array;
  bool changed = false;
  PTree::Node *rest = decls;
  while(rest)
  {
    PTree::Node *p, *q;
    int len;
    p = q = rest->car();
    if(PTree::is_a(p, Token::ntDeclarator))
    {
      PTree::Node *exp, *exp2;
      if(record) env->RecordDeclarator(p);
      len = PTree::length(p);
      exp = exp2 = 0;
      if(len >= 2 && *PTree::nth(p, len - 2) == '=')
      {
	exp = PTree::tail(p, len - 2);
	exp2 = translate_assign_initializer((PTree::Declarator*)p, exp);
      }
      else
      {
	PTree::Node *last = PTree::last(p)->car();
	if(last != 0 && !last->is_atom() && *last->car() == '(')
	{
	  exp = last;
	  exp2 = translate_initialize_args((PTree::Declarator*)p, last);
	}
      }
      q = translate_declarator(false, (PTree::Declarator*)p);
      if(exp != exp2)
      {
	// exp2 should be a list, but...
	if(exp2 != 0 && exp2->is_atom())
	  exp2 = PTree::list(exp2);
	if(p == q)
	{
	  q = PTree::subst_sublist(exp2, exp, p->cdr());
	  q = new PTree::Declarator((PTree::Declarator*)p, p->car(), q);
	}
	else if(q != 0 && !q->is_atom())
	  q = new PTree::Declarator((PTree::Declarator*)p, q->car(),
				    PTree::subst(exp2, exp, q->cdr()));
      }
    }
    if(!q)
    {
      changed = true;
      rest = rest->cdr();
      if(rest) rest = rest->cdr();
    }
    else
    {
      array.append(q);
      if(p != q) changed = true;
      rest = rest->cdr();
      if(rest)
      {
	array.append(rest->car());
	rest = rest->cdr();
      }
    }
  }
  if(changed) return array.all();
  else return decls;
}

PTree::Node *Walker::translate_declarator(bool record, PTree::Declarator* decl)
{
  // if record is true, the formal arguments are recorded in the
  // current environment.

  PTree::Node *args;
  if(GetArgDeclList(decl, args))
  {
    PTree::Node *args2 = translate_arg_decl_list(record, decl, args);
    if(args == args2) return decl;
    else return new PTree::Declarator(decl, decl->car(),
				      PTree::subst(args2, args, decl->cdr()));
  }
  else return decl;
}

bool Walker::GetArgDeclList(PTree::Declarator* decl, PTree::Node *& args)
{
    PTree::Node *p = decl;
    while(p != 0){
	PTree::Node *q = p->car();
	if(q != 0)
	    if(q->is_atom()){
		if(*q == '('){
		  args = PTree::cadr(p);
		    return true;
		}
	    }
	    else if(*q->car() == '(')	// e.g. int (*p)[];
	      p = PTree::cadr(q);

	p = p->cdr();
    }

    args = 0;
    return false;
}

PTree::Node *Walker::translate_arg_decl_list(bool record, PTree::Node *, PTree::Node *args)
{
    return translate_arg_decl_list2(record, env, false, false, 0, args);
}

// If translate is true, this function eliminates a user-defined keyword.

PTree::Node *Walker::translate_arg_decl_list2(bool record, Environment* e,
					      bool translate,
					      bool fill_args, int arg_name,
					      PTree::Node *args)
{
    PTree::Node *rest;
    PTree::Node *rest2;

    if(args == 0)
	return args;
    else{
      PTree::Node *a, *a2;
	a = a2 = args->car();
	if(args->cdr() == 0)
	    rest = rest2 = 0;
	else{
	    rest = PTree::cddr(args);	// skip ","
	    rest2 = translate_arg_decl_list2(record, e, translate, fill_args,
					     arg_name + 1, rest);
	    if(rest == rest2)
		rest = rest2 = args->cdr();
	    else
		rest2 = PTree::cons(PTree::cadr(args), rest2);
	}

	bool is_ellipsis = a->is_atom();		// a may be "..."
	if(is_ellipsis)
	    /* do nothing */;
	else if(PTree::is_a(a->car(), Token::ntUserdefKeyword)){
	    if(record)
		e->RecordDeclarator(PTree::third(a));

	    if(translate){
		a2 = a->cdr();
		if(fill_args)
		  a2 = fill_argument_name(a2, PTree::second(a2), arg_name);
	    }
	}
	else if(PTree::is_a(a->car(), Token::REGISTER)){
	    if(record)
		e->RecordDeclarator(PTree::third(a));

	    if(translate && fill_args){
		a2 = fill_argument_name(a, PTree::third(a), arg_name);
		if(a != a2)
		    a2 = PTree::cons(PTree::first(a), a2);
	    }
	}
	else{
	    if(record)
		e->RecordDeclarator(PTree::second(a));

	    if(translate && fill_args)
		a2 = fill_argument_name(a, PTree::second(a), arg_name);
	}

	if(a != a2 || rest != rest2)
	    return PTree::cons(a2, rest2);
	else
	    return args;
    }
}

PTree::Node *Walker::fill_argument_name(PTree::Node *arg, PTree::Node *d, int arg_name)
{
  PTree::Declarator* decl = (PTree::Declarator*)d;
  if(decl->name()) return arg;
  else
  {
    PTree::Encoding type = decl->encoded_type();
    return type.make_ptree(PTree::make(argument_name, arg_name));
  }
}

PTree::Node *Walker::translate_assign_initializer(PTree::Declarator*,
						  PTree::Node *init)
{
  PTree::Node *exp = PTree::second(init);
  PTree::Node *exp2 = translate(exp);
  if(exp == exp2) return init;
  else return PTree::list(init->car(), exp2);
}

PTree::Node *Walker::translate_initialize_args(PTree::Declarator*, PTree::Node *init)
{
  return translate_arguments(init);
}

PTree::Node *Walker::translate_function_implementation(PTree::Node *impl)
{
  PTree::Node *sspec = PTree::first(impl);
  PTree::Node *sspec2 = translate_storage_specifiers(sspec);
  PTree::Node *tspec = PTree::second(impl);
  PTree::Node *decl = PTree::third(impl);
  PTree::Node *body = PTree::nth(impl, 3);
  PTree::Node *decl2;
  PTree::Node *body2;

  PTree::Node *tspec2 = translate_type_specifier(tspec);
  Environment* fenv = env->RecordDeclarator(decl);
  if(!fenv)
  {
    // reach here if resolving the qualified name fails. error?
    new_scope();
    decl2 = translate_declarator(true, (PTree::Declarator*)decl);
    body2 = translate(body);
    exit_scope();
  }
  else
  {
    NameScope old_env = change_scope(fenv);
    new_scope();
    decl2 = translate_declarator(true, (PTree::Declarator*)decl);
    body2 = translate(body);
    exit_scope();
    restore_scope(old_env);
  }

  if(sspec == sspec2 && tspec == tspec2 && decl == decl2 && body == body2)
    return impl;
  else
    return new PTree::Declaration(sspec2, PTree::list(tspec2, decl2, body2));
}

PTree::Node *Walker::record_args_and_translate_fbody(Class*, PTree::Node *args, PTree::Node *body)
{
    new_scope();
    translate_arg_decl_list2(true, env, false, false, 0, args);
    PTree::Node *body2 = translate_function_body(body);
    exit_scope();
    return body2;
}

PTree::Node *Walker::translate_function_body(PTree::Node *body)
{
  return translate(body);
}

void Walker::visit(PTree::Brace *node)
{
  PTree::Array array;
  bool changed = false;
  PTree::Node *body = PTree::second(node);
  PTree::Node *rest = body;
  while(rest)
  {
    PTree::Node *p = rest->car();
    PTree::Node *q = translate(p);
    array.append(q);
    if(p != q) changed = true;
    rest = rest->cdr();
  }

  if(changed)
    my_result = new PTree::Brace(PTree::first(node), array.all(), PTree::third(node));
  else
    my_result = node;
}

void Walker::visit(PTree::Block *node)
{
  new_scope();
  PTree::Array array;
  bool changed = false;
  PTree::Node *body = PTree::second(node);
  PTree::Node *rest = body;
  while(rest)
  {
    PTree::Node *p = rest->car();
    PTree::Node *q = translate(p);
    array.append(q);
    if(p != q) changed = true;
    rest = rest->cdr();
  }
  if(changed)
    my_result = new PTree::Block(PTree::first(node), array.all(), PTree::third(node));
  else
    my_result = node;

  exit_scope();
}

void Walker::visit(PTree::ClassBody *node)
{
  new_scope();
  PTree::Array array;
  bool changed = false;
  PTree::Node *body = PTree::second(node);
  PTree::Node *rest = body;
  while(rest)
  {
    PTree::Node *p = rest->car();
    PTree::Node *q = translate(p);
    array.append(q);
    if(p != q) changed = true;
    rest = rest->cdr();
  }
  if(changed)
    my_result = new PTree::ClassBody(PTree::first(node),
				     array.all(),
				     PTree::third(node),
				     node->scope());
  else
    my_result = node;

  exit_scope();
}

PTree::ClassBody *Walker::translate_class_body(PTree::ClassBody *node,
					       PTree::Node *bases,
					       Class* metaobject)
{
  PTree::ClassBody *block2;

  new_scope(metaobject);
  RecordBaseclassEnv(bases);

  PTree::Array array;
  bool changed = false;
  PTree::Node *body = PTree::second(node);
  PTree::Node *rest = body;
  while(rest)
  {
    PTree::Node *p = rest->car();
    PTree::Node *q = translate(p);
    array.append(q);
    if(p != q) changed = true;
    rest = rest->cdr();
  }
  if(changed)
    block2 = new PTree::ClassBody(PTree::first(node), array.all(),
				  PTree::third(node), node->scope());
  else
    block2 = node;

  exit_scope();
  return block2;
}

void Walker::visit(PTree::ClassSpec *node)
{
  PTree::Node *userkey;
  PTree::Node *class_def;

  if(node->car()->is_atom())
  {
    userkey = 0;
    class_def = node;
  }
  else
  {
    userkey = node->car();
    class_def = node->cdr();
  }

  Class *metaobject = 0;
  if(PTree::length(class_def) == 4)
    metaobject = make_class_metaobject(node, userkey, class_def);

  env->RecordClassName(node->encoded_name(), metaobject);
  my_result = translate_class_spec(node, userkey, class_def, metaobject);
}

Class *Walker::make_class_metaobject(PTree::ClassSpec *def, PTree::Node *userkey,
				     PTree::Node *class_def)
{
    Class* metaobject = LookupMetaclass(def, userkey, class_def, false);
    if(metaobject == 0 && default_metaclass != 0){
	metaobject = opcxx_ListOfMetaclass::New(default_metaclass, class_def,
						0);
	if(metaobject == 0)
	    MopErrorMessage2("the default metaclass cannot be loaded: ",
			     default_metaclass);
    }

    if(metaobject == 0)
	metaobject = new Class;
    else{
	if(!metaobject->AcceptTemplate())
	    return metaobject;
	else{
	    ErrorMessage("the specified metaclass is for templates.",
			 0, def);
	    metaobject = new Class;
	}
    }

    metaobject->InitializeInstance(class_def, 0);
    return metaobject;
}

void Walker::ChangeDefaultMetaclass(const char* name)
{
    default_metaclass = name;
}

// LookupMetaclass() returns 0 if no metaclass is found.

Class* Walker::LookupMetaclass(PTree::Node *def, PTree::Node *userkey, PTree::Node *class_def,
			       bool is_template)
{
  PTree::Node *mclass, *margs;
    Class* metaobject;

    PTree::Node *class_name = PTree::second(class_def);

    // for bootstrapping
    if(Metaclass::IsBuiltinMetaclass(class_name)){
	metaobject = new Metaclass;
	metaobject->InitializeInstance(def, 0);
	return metaobject;
    }

    PTree::Node *mdecl = env->LookupMetaclass(class_name);
    if(mdecl != 0){
	mclass = PTree::second(mdecl);
	margs = PTree::nth(mdecl, 4);
	metaobject = opcxx_ListOfMetaclass::New(mclass, def, margs);
	if(metaobject == 0)
	    ErrorMessage("the metaclass is not loaded: ", mclass, class_def);
	else if(userkey != 0)
	    ErrorMessage("the metaclass declaration conflicts"
			 " with the keyword: ", mclass, class_def);

	return metaobject;
    }

    if(userkey != 0){
	mclass = env->LookupClasskeyword(userkey->car());
	if(mclass == 0)
	    ErrorMessage("invalid keyword: ", userkey, class_def);
	else{
	    metaobject = opcxx_ListOfMetaclass::New(mclass, class_def,
						    PTree::third(userkey));
	    if(metaobject == 0)
		ErrorMessage("the metaclass associated with the"
			     " keyword is not loaded: ", userkey, class_def);

	    return metaobject;
	}
    }

    return LookupBaseMetaclass(def, class_def, is_template);
}

Class* Walker::LookupBaseMetaclass(PTree::Node *def, PTree::Node *class_def,
				   bool is_template)
{
    Class* metaobject = 0;
    PTree::Node *bases = PTree::third(class_def);
    while(bases != 0){
	bases = bases->cdr();
	PTree::Node *base = PTree::last(bases->car())->car();
	bases = bases->cdr();
	Class* m = env->LookupClassMetaobject(base);
	if(m != 0){
	    if(metaobject == 0)
		metaobject = m;
	    else if(m == 0 || strcmp(metaobject->MetaclassName(),
				       m->MetaclassName()) != 0){
		ErrorMessage("inherited metaclasses conflict: ",
			     PTree::second(class_def), class_def);
		return 0;
	    }
	}
    }

    if(metaobject == 0)
	return 0;

    bool accept_template = metaobject->AcceptTemplate();
    if((is_template && accept_template) || (!is_template && !accept_template))
	return opcxx_ListOfMetaclass::New(metaobject->MetaclassName(),
					  def, 0);
    else
	return 0;
}

PTree::ClassSpec *Walker::translate_class_spec(PTree::ClassSpec *spec,
					       PTree::Node *, // userkey...used in ClassWalker
					       PTree::Node *class_def,
					       Class *metaobject)
{
  if(!metaobject) return spec;
  else
  {
    // a class body is specified.
    PTree::ClassBody *body = static_cast<PTree::ClassBody *>(PTree::nth(class_def, 3));
    PTree::ClassBody *body2 = translate_class_body(body,
						   PTree::third(class_def),
						   metaobject);
    if(body == body2)
      return spec;
    else
      return new PTree::ClassSpec(spec->encoded_name(),
				  spec->car(),
				  PTree::shallow_subst(body2, body,
						       spec->cdr()),
				  0);
  }
}

void Walker::visit(PTree::EnumSpec *node)
{
  env->RecordEnumName(node);
  my_result = node;
}

void Walker::visit(PTree::IfStatement *node)
{
  PTree::Node *cond = PTree::third(node);
  PTree::Node *cond2 = translate(cond);
  PTree::Node *then_part = PTree::nth(node, 4);
  PTree::Node *then_part2 = translate(then_part);
  PTree::Node *else_part = PTree::nth(node, 6);
  PTree::Node *else_part2 = translate(else_part);

  if(cond == cond2 && then_part == then_part2 && else_part == else_part2)
    my_result = node;
  else
  {
    PTree::Node *rest = PTree::shallow_subst(cond2, cond, then_part2, then_part,
					     else_part2, else_part, node->cdr());
    my_result = new PTree::IfStatement(node->car(), rest);
  }
}

void Walker::visit(PTree::SwitchStatement *node)
{
  PTree::Node *cond = PTree::third(node);
  PTree::Node *cond2 = translate(cond);
  PTree::Node *body = PTree::nth(node, 4);
  PTree::Node *body2 = translate(body);
  if(cond == cond2 && body == body2)
    my_result = node;
  else
  {
    PTree::Node *rest = PTree::shallow_subst(cond2, cond, body2, body, node->cdr());
    my_result = new PTree::SwitchStatement(node->car(), rest);
  }
}

void Walker::visit(PTree::WhileStatement *node)
{
  PTree::Node *cond = PTree::third(node);
  PTree::Node *cond2 = translate(cond);
  PTree::Node *body = PTree::nth(node, 4);
  PTree::Node *body2 = translate(body);
  if(cond == cond2 && body == body2)
    my_result = node;
  else
  {
    PTree::Node *rest = PTree::shallow_subst(cond2, cond, body2, body, node->cdr());
    my_result = new PTree::WhileStatement(node->car(), rest);
  }
}

void Walker::visit(PTree::DoStatement *node)
{
  PTree::Node *body = PTree::second(node);
  PTree::Node *body2 = translate(body);
  PTree::Node *cond = PTree::nth(node, 4);
  PTree::Node *cond2 = translate(cond);
  if(cond == cond2 && body == body2)
    my_result = node;
  else
  {
    PTree::Node *rest = PTree::shallow_subst(body2, body, cond2, cond, node->cdr());
    my_result = new PTree::DoStatement(node->car(), rest);
  }
}

void Walker::visit(PTree::ForStatement *node)
{
  new_scope();
  PTree::Node *exp1 = PTree::third(node);
  PTree::Node *exp1t = translate(exp1);
  PTree::Node *exp2 = PTree::nth(node, 3);
  PTree::Node *exp2t = translate(exp2);
  PTree::Node *exp3 = PTree::nth(node, 5);
  PTree::Node *exp3t = translate(exp3);
  PTree::Node *body = PTree::nth(node, 7);
  PTree::Node *body2 = translate(body);
  exit_scope();

  if(exp1 == exp1t && exp2 == exp2t && exp3 == exp3t && body == body2)
    my_result = node;
  else
  {
    PTree::Node *rest = PTree::shallow_subst(exp1t, exp1, exp2t, exp2,
					     exp3t, exp3, body2, body, node->cdr());
    my_result = new PTree::ForStatement(node->car(), rest);
  }
}

void Walker::visit(PTree::TryStatement *node)
{
  PTree::Node *try_block = PTree::second(node);
  PTree::Node *try_block2 = translate(try_block);

  PTree::Array array;
  PTree::Node *handlers = PTree::cddr(node);
  bool changed = false;

  while(handlers != 0)
  {
    PTree::Node *handle = handlers->car();
    PTree::Node *body = PTree::nth(handle, 4);
    PTree::Node *body2 = translate(body);
    if(body == body2)
      array.append(handle);
    else
    {
      array.append(PTree::shallow_subst(body2, body, handle));
      changed = true;
    }

    handlers = handlers->cdr();
  }

  if(try_block == try_block2 && !changed)
    my_result = node;
  else
    my_result = new PTree::TryStatement(node->car(),
					PTree::cons(try_block2, array.all()));
}

void Walker::visit(PTree::ReturnStatement *node)
{
  if(PTree::length(node) == 2)
    my_result = node;
  else
  {
    PTree::Node *exp = PTree::second(node);
    PTree::Node *exp2 = translate(exp);
    if(exp == exp2)
      my_result = node;
    else
      my_result = new PTree::ReturnStatement(node->car(),
					     PTree::shallow_subst(exp2, exp,
								  node->cdr()));
  }
}

void Walker::visit(PTree::CaseStatement *node)
{
  PTree::Node *st = PTree::nth(node, 3);
  PTree::Node *st2 = translate(st);
  if(st == st2)
    my_result = node;
  else
    my_result = new PTree::CaseStatement(node->car(),
					 PTree::shallow_subst(st2, st, node->cdr()));
}

void Walker::visit(PTree::DefaultStatement *node)
{
  PTree::Node *st = PTree::third(node);
  PTree::Node *st2 = translate(st);
  if(st == st2)
    my_result = node;
  else
    my_result = new PTree::DefaultStatement(node->car(),
					    PTree::list(PTree::cadr(node), st2));
}

void Walker::visit(PTree::LabelStatement *node)
{
  PTree::Node *st = PTree::third(node);
  PTree::Node *st2 = translate(st);
  if(st == st2)
    my_result = node;
  else
    my_result = new PTree::LabelStatement(node->car(),
					  PTree::list(PTree::cadr(node), st2));
}

void Walker::visit(PTree::ExprStatement *node)
{
  PTree::Node *exp = PTree::first(node);
  PTree::Node *exp2 = translate(exp);
  if(exp == exp2)
    my_result = node;
  else
    my_result = new PTree::ExprStatement(exp2, node->cdr());
}

PTree::Node *Walker::translate_type_specifier(PTree::Node *tspec)
{
  PTree::Node *class_spec = get_class_or_enum_spec(tspec);
  PTree::Node *class_spec2;
  if(!class_spec) class_spec2 = 0;
  else class_spec2 = translate(class_spec);

  if(class_spec == class_spec2)
    return tspec;
  else
    return PTree::shallow_subst(class_spec2, class_spec, tspec);
}

PTree::Node *Walker::get_class_or_enum_spec(PTree::Node *typespec)
{
  PTree::Node *spec = strip_cv_from_integral_type(typespec);
  if(PTree::is_a(spec, Token::ntClassSpec, Token::ntEnumSpec))
    return spec;
  return 0;
}

PTree::ClassSpec *Walker::get_class_template_spec(PTree::Node *body)
{
  if(*PTree::third(body) == ';')
  {
    PTree::Node *spec = strip_cv_from_integral_type(PTree::second(body));
    return dynamic_cast<PTree::ClassSpec *>(spec);
  }
  return 0;
}

PTree::Node *Walker::strip_cv_from_integral_type(PTree::Node *integral)
{
  if(integral == 0) return 0;

  if(!integral->is_atom())
    if(PTree::is_a(integral->car(), Token::CONST, Token::VOLATILE))
      return PTree::second(integral);
    else if(PTree::is_a(PTree::second(integral), Token::CONST, Token::VOLATILE))
      return integral->car();

  return integral;
}

void Walker::visit(PTree::CommaExpr *node)
{
  PTree::Node *left = PTree::first(node);
  PTree::Node *left2 = translate(left);
  PTree::Node *right = PTree::third(node);
  PTree::Node *right2 = translate(right);
  if(left == left2 && right == right2)
    my_result = node;
  else
    my_result = new PTree::CommaExpr(left2, PTree::list(PTree::second(node), right2));
}

void Walker::visit(PTree::AssignExpr *node)
{
  PTree::Node *left = PTree::first(node);
  PTree::Node *left2 = translate(left);
  PTree::Node *right = PTree::third(node);
  PTree::Node *right2 = translate(right);
  if(left == left2 && right == right2)
    my_result = node;
  else
    my_result = new PTree::AssignExpr(left2, PTree::list(PTree::second(node), right2));
}

void Walker::visit(PTree::CondExpr *node)
{
  PTree::Node *c = PTree::first(node);
  PTree::Node *c2 = translate(c);
  PTree::Node *t = PTree::third(node);
  PTree::Node *t2 = translate(t);
  PTree::Node *e = PTree::nth(node, 4);
  PTree::Node *e2 = translate(e);
  if(c == c2 && t == t2 && e == e2)
    my_result = node;
  else
    my_result = new PTree::CondExpr(c2, PTree::list(PTree::second(node), t2,
						    PTree::nth(node, 3), e2));
}

void Walker::visit(PTree::InfixExpr *node)
{
  PTree::Node *left = PTree::first(node);
  PTree::Node *left2 = translate(left);
  PTree::Node *right = PTree::third(node);
  PTree::Node *right2 = translate(right);
  if(left == left2 && right == right2)
    my_result = node;
  else
    my_result = new PTree::InfixExpr(left2, PTree::list(PTree::second(node), right2));
}

void Walker::visit(PTree::PmExpr *node)
{
  PTree::Node *left = PTree::first(node);
  PTree::Node *left2 = translate(left);
  PTree::Node *right = PTree::third(node);
  PTree::Node *right2 = translate(right);
  if(left == left2 && right == right2)
    my_result = node;
  else
    my_result = new PTree::PmExpr(left2, PTree::list(PTree::second(node), right2));
}

void Walker::visit(PTree::CastExpr *node)
{
  PTree::Node *e = PTree::nth(node, 3);
  PTree::Node *e2 = translate(e);
  if(e == e2)
    my_result = node;
  else
    my_result = new PTree::CastExpr(PTree::first(node),
				    PTree::shallow_subst(e2, e, node->cdr()));
}

void Walker::visit(PTree::UnaryExpr *node)
{
  PTree::Node *oprnd = PTree::second(node);
  PTree::Node *oprnd2 = translate(oprnd);
  if(oprnd == oprnd2)
    my_result = node;
  else
    my_result = new PTree::UnaryExpr(PTree::first(node), PTree::list(oprnd2));
}

void Walker::visit(PTree::ThrowExpr *node)
{
  PTree::Node *oprnd = PTree::second(node);
  PTree::Node *oprnd2 = translate(oprnd);
  if(oprnd == oprnd2)
    my_result = node;
  else
    my_result = new PTree::ThrowExpr(PTree::first(node), PTree::list(oprnd2));
}

void Walker::visit(PTree::SizeofExpr *node)
{
  PTree::Node *e = PTree::second(node);
  if(*e == '(') e = PTree::third(node);

  PTree::Node *e2 = translate(e);
  if(e == e2)
    my_result = node;
  else
    my_result = new PTree::SizeofExpr(PTree::first(node),
				      PTree::shallow_subst(e2, e, node->cdr()));
}

void Walker::visit(PTree::TypeidExpr *node)
{
  PTree::Node *e = PTree::second(node);
  if(*e == '(') e = PTree::third(node);
  
  PTree::Node *e2 = translate(e);
  if(e == e2)
    my_result = node;
  else
    my_result = new PTree::TypeidExpr(PTree::first(node),
				      PTree::shallow_subst(e2, e, node->cdr()));
}

void Walker::visit(PTree::TypeofExpr *node)
{
  PTree::Node *e = PTree::second(node);
  if(*e == '(') e = PTree::third(node);

  PTree::Node *e2 = translate(e);
  if(e == e2)
    my_result = node;
  else
    my_result = new PTree::TypeofExpr(PTree::first(node),
				      PTree::shallow_subst(e2, e, node->cdr()));
}

void Walker::visit(PTree::NewExpr *node)
{
  PTree::Node *p = node;
  PTree::Node *userkey = p->car();
  if(!userkey || !userkey->is_atom())
    p = node->cdr(); // user keyword
  else userkey = 0;

  PTree::Node *scope;
  if(*p->car() == "::")
  {
    scope = p->car();
    p = p->cdr();
  }
  else scope = 0;

  PTree::Node *op = p->car();
  PTree::Node *placement = PTree::cadr(p);
  PTree::Node *type = PTree::third(p);
  PTree::Node *init = PTree::nth(p, 3);
  my_result = translate_new2(node, userkey, scope, op, placement, type, init);
}

PTree::Node *Walker::translate_new2(PTree::Node *exp, PTree::Node *, PTree::Node *,
				    PTree::Node *, PTree::Node *placement,
				    PTree::Node *type, PTree::Node *init)
{
  PTree::Node *placement2 = translate_arguments(placement);
  PTree::Node *type2 = translate_new3(type);
  PTree::Node *init2 = translate_arguments(init);
  if(placement == placement2 && init == init2)
    return exp;
  else
    return new PTree::NewExpr(exp->car(),
			      PTree::shallow_subst(placement2, placement,
						   type2, type,
						   init2, init,
						   exp->cdr()));
}

PTree::Node *Walker::translate_new3(PTree::Node *type)
{
    PTree::Node *p = type;
    if(*p->car() == '(')
	p = PTree::second(p);

    PTree::Node *decl = PTree::second(p);
    PTree::Node *decl2 = translate_new_declarator(decl);
    if(decl == decl2)
	return type;
    else
	return PTree::subst(decl2, decl, type);
}

void Walker::visit(PTree::DeleteExpr *node)
{
  PTree::Node *obj = PTree::last(node)->car();
  PTree::Node *obj2 = translate(obj);
  if(obj == obj2)
    my_result = node;
  else
    my_result = new PTree::DeleteExpr(node->car(),
				      PTree::shallow_subst(obj2, obj, node->cdr()));
}

/*
  TranslateFstyleCast() deals with function-style casts
  to an integral type
*/
void Walker::visit(PTree::FstyleCastExpr *node)
{
  PTree::Node *args = node->cdr();
  PTree::Node *args2 = translate_arguments(args);
  if(args == args2)
    my_result = node;
  else
    my_result = new PTree::FstyleCastExpr(node->encoded_type(), node->car(), args2);
}

void Walker::visit(PTree::ArrayExpr *node)
{
  PTree::Node *array = node->car();
  PTree::Node *array2 = translate(array);
  PTree::Node *index = PTree::third(node);
  PTree::Node *index2 = translate(index);
  if(array == array2 && index == index2)
    my_result = node;
  else
    my_result = new PTree::ArrayExpr(array2, PTree::subst(index2, index, node->cdr()));
}

/*
  TranslateFuncall() also deals with function-style casts to a class.
*/
void Walker::visit(PTree::FuncallExpr *node)
{
  PTree::Node *fun = node->car();
  PTree::Node *fun2 = translate(fun);
  PTree::Node *args = node->cdr();
  PTree::Node *args2 = translate_arguments(args);
  if(fun == fun2 && args == args2)
    my_result = node;
  else
    my_result = new PTree::FuncallExpr(fun2, args2);
}

void Walker::visit(PTree::PostfixExpr *node)
{
  PTree::Node *left = node->car();
  PTree::Node *left2 = translate(left);
  if(left == left2)
    my_result = node;
  else
    my_result = new PTree::PostfixExpr(left2, node->cdr());
}

void Walker::visit(PTree::DotMemberExpr *node)
{
  PTree::Node *left = node->car();
  PTree::Node *left2 = translate(left);
  if(left == left2)
    my_result = node;
  else
    my_result = new PTree::DotMemberExpr(left2, node->cdr());
}

void Walker::visit(PTree::ArrowMemberExpr *node)
{
  PTree::Node *left = node->car();
  PTree::Node *left2 = translate(left);
  if(left == left2)
    my_result = node;
  else
    my_result = new PTree::ArrowMemberExpr(left2, node->cdr());
}

void Walker::visit(PTree::ParenExpr *node)
{
  PTree::Node *e = PTree::second(node);
  PTree::Node *e2 = translate(e);
  if(e == e2)
    my_result = node;
  else
    my_result = new PTree::ParenExpr(node->car(), PTree::list(e2, PTree::third(node)));
}

PTree::Node *Walker::translate_new_declarator(PTree::Node *decl)
{
    PTree::Node *decl2 = decl;
    PTree::Node *p = decl;
    while(p != 0){
	PTree::Node *head = p->car();
	if(head == 0)
	    return decl;
	else if(*head == '['){
	    PTree::Node *p2 = translate_new_declarator2(p);
	    if(p == p2)
		return decl;
	    else{
		decl2 = PTree::shallow_subst(p2, p, decl);
		break;
	    }
	}
	else if(!head->is_atom() && *head->car() == '('){
	    PTree::Node *d = PTree::cadr(head);
	    PTree::Node *d2 = translate_new_declarator(d);
	    decl2 = PTree::shallow_subst(d2, d, decl);
	    break;
	}

	p = p->cdr();
    }

    if(p == 0)
	return decl;
    else if(PTree::is_a(decl, Token::ntDeclarator))
	return new PTree::Declarator((PTree::Declarator*)decl,
				     decl2->car(), decl2->cdr());
    else
	return decl2;
}

PTree::Node *Walker::translate_new_declarator2(PTree::Node *decl)
{
    for(PTree::Node *p = decl; p != 0; p = p->cdr()){
	PTree::Node *head = p->car();
	if(*head == '['){
	    PTree::Node *size = PTree::cadr(p);
	    PTree::Node *size2 = translate(size);
	    if(size != size2){
		PTree::Node *q = translate_new_declarator2(PTree::tail(p, 3));
		return PTree::nconc(PTree::list(p->car(), size2, PTree::third(p)),
				    q);
	    }
	}
	else if(*head == '(')
	    break;
    }

    return decl;
}

PTree::Node *Walker::translate_arguments(PTree::Node *arglist)
{
    if(arglist == 0)
	return arglist;

    PTree::Array array;
    bool changed = false;
    PTree::Node *body = PTree::second(arglist);
    PTree::Node *args = body;
    while(args != 0){
	PTree::Node *p = args->car();
	PTree::Node *q = translate(p);
	array.append(q);
	if(p != q)
	    changed = true;

	args = args->cdr();
	if(args != 0){
	    array.append(args->car());
	    args = args->cdr();
	}
    }

    if(changed)
	return PTree::shallow_subst(array.all(), body, arglist);
    else
	return arglist;
}

//. Helper function to recursively find the first left-most leaf node
PTree::Node *Walker::FindLeftLeaf(PTree::Node *node, PTree::Node *& parent)
{
    if (!node || node->is_atom()) return node;
    // Non-leaf node. So find first leafy child
    PTree::Node *leaf;
    while (node) {
	if (node->car()) {
	    // There is a child here..
	    if (node->car()->is_atom()) {
		// And this child is a leaf! return it and set parent
		parent = node;
		return node->car();
	    }
	    if ((leaf = FindLeftLeaf(node->car(), parent)))
		// Not a leaf so try recursing on it
		return leaf;
	}
	// No leaves from car of this node, so try next cdr
	node = node->cdr();
    }
    return 0;
}

//. Node is never the leaf. Instead we traverse the left side of the tree
//. until we find a leaf, and change the leaf to be a CommentedLeaf.
void Walker::SetLeafComments(PTree::Node *node, PTree::Node *comments)
{
    PTree::Node *parent, *leaf;
    PTree::CommentedAtom* cleaf;

    // Find leaf
    leaf = FindLeftLeaf(node, parent);

    // Sanity
    if (!leaf)
    {
      std::cerr << "Warning: Failed to find leaf when trying to add comments." << std::endl;
      PTree::Display display(std::cerr, false);
      display.display(parent);
      return; 
    }

    if (!(cleaf = dynamic_cast<PTree::CommentedAtom *>(leaf))) {
	// Must change first child of parent to be a commented leaf
        Token tk(leaf->position(), leaf->length(), Token::Comment);
	cleaf = new (PTree::GC) PTree::CommentedAtom(tk, comments);
	parent->set_car(cleaf);
    } else {
	// Already is a commented leaf, so add the comments to it
	comments = PTree::snoc(cleaf->get_comments(), comments);
	cleaf->set_comments(comments);
    }
}

void Walker::SetDeclaratorComments(PTree::Node *def, PTree::Node *comments)
{
  if (def == 0 || !PTree::is_a(def, Token::ntDeclaration))
	return;

    PTree::Node *decl;
    int n = 0;
    for (;;) {
	int i = n++;
	decl = NthDeclarator(def, i);
	if (decl == 0)
	    break;
	else if (PTree::is_a(decl, Token::ntDeclarator))
	    ((PTree::Declarator*)decl)->set_comments(comments);
    }
}

PTree::Node *Walker::NthDeclarator(PTree::Node *def, int& nth)
{
    PTree::Node *decls = PTree::third(def);
    if(decls == 0 || decls->is_atom())
	return 0;

    if(PTree::is_a(decls, Token::ntDeclarator)){	// if it is a function
	if(nth-- == 0)
	    return decls;
    }
    else
	while(decls != 0 && !decls->is_atom()){
	    if(nth-- == 0)
		return decls->car();

	    if((decls = decls->cdr()) != 0)
		decls = decls->cdr();		// skip ,
	}

    return 0;
}

// PTree::Node *Walker::FindDeclarator(PTree::Node *def, const char* name, int len,
// 			      const char* signature, int& nth, Environment* e)
// {
//     PTree::Node *decls = PTree::third(def);
//     if(decls == 0 || decls->is_atom())
// 	return 0;

//     if(PTree::is_a(decls, Token::ntDeclarator)){	// if it is a function
// 	if(MatchedDeclarator(decls, name, len, signature, e))
// 	    return decls;

// 	++nth;
//     }
//     else
// 	while(decls != 0){
// 	    PTree::Node *d = decls->car();
// 	    if(MatchedDeclarator(d, name, len, signature, e))
// 		return d;

// 	    ++nth;
// 	    if((decls = decls->cdr()) != 0)
// 		decls = decls->cdr();		// skip ,
// 	}

//     return 0;
// }

// bool Walker::MatchedDeclarator(PTree::Node *decl, const char* name, int len,
// 			       const char* signature, Environment* e)
// {
//   PTree::Encoding enc = decl->encoded_name();
//   PTree::Encoding sig = decl->encoded_type();
//     const char* str;
//     int strlen;
//     const char* sig;

//     str = decl->encoded_name();
//     sig = decl->encoded_type();
//     if(str == 0 || sig == 0)
// 	return false;

//     str = PTree::Encoding::GetBaseName(str, strlen, e);
//     return bool(len == strlen && memcmp(name, str, len) == 0
// 		&& strcmp(signature, sig) == 0);
// }

// bool Walker::WhichDeclarator(PTree::Node *def, PTree::Node *name, int& nth,
// 			     Environment* env)
// {
//     const char* str;
//     int len;
//     Environment* e;
//     PTree::Node *decls = PTree::third(def);
//     if(decls == 0 || decls->is_atom())
// 	return false;

//     if(PTree::is_a(decls, Token::ntDeclarator)){	// if it is a function
// 	str = decls->encoded_name();
// 	e = env;
// 	str = PTree::Encoding::GetBaseName(str, len, e);
// 	if(equal(*name, str, len))
// 	    return true;

// 	++nth;
//     }
//     else
// 	while(decls != 0){
// 	    str = decls->car()->encoded_name();
// 	    e = env;
// 	    str = PTree::Encoding::GetBaseName(str, len, e);
// 	    if(equal(*name, str, len))
// 		return true;

// 	    ++nth;
// 	    if((decls = decls->cdr()) != 0)
// 		decls = decls->cdr();
// 	}

//     return false;
// }

void Walker::ErrorMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
    parser->error_message(msg, name, where);
}

void Walker::WarningMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
    parser->warning_message(msg, name, where);
}

// InaccurateErrorMessage() may report a wrong line number.

void Walker::InaccurateErrorMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
  if(!default_parser)
    throw std::runtime_error("Walker::InaccurateErrorMessage(): no default parser");
  else
    default_parser->error_message(msg, name, where);
}

void Walker::InaccurateWarningMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
    if(!default_parser)
      throw std::runtime_error("Walker::InaccurateWarningMessage(): no default parser");
    else
	default_parser->warning_message(msg, name, where);
}
