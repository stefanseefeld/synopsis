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
#include "PTree.hh"
#include "Walker.hh"
#include "TypeInfo.hh"
#include "Class.hh"
#include "MetaClass.hh"
#include "Parser.hh"
#include "Encoding.hh"
#include "Member.hh"

Parser* Walker::default_parser = 0;
const char* Walker::argument_name = "_arg_%d_";
const char* Walker::default_metaclass = 0;

Walker::Walker(Parser* p)
{
    env = new Environment(this);
    parser = p;
    if(default_parser == 0)
	default_parser = p;
}

Walker::Walker(Parser* p, Environment* e)
{
    env = new Environment(e, this);
    parser = p;
    if(default_parser == 0)
	default_parser = p;
}

Walker::Walker(Environment* e)
{
    env = new Environment(e, this);
    if(default_parser == 0)
	MopErrorMessage("Walker::Walker()", "no default parser");

    parser = default_parser;
}

Walker::Walker(Walker* w)
{
    env = w->env;
    parser = w->parser;
}

void Walker::NewScope()
{
    env = new Environment(env);
}

void Walker::NewScope(Class* metaobject)
{
    env = new Environment(env);
    if(metaobject != 0)
	metaobject->SetEnvironment(env);
}

Environment* Walker::ExitScope()
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

Walker::NameScope Walker::ChangeScope(Environment* e)
{
    NameScope scope;
    scope.walker = e->GetWalker();
    e->SetWalker(this);
    scope.env = env;
    env = e;
    return scope;
}

void Walker::RestoreScope(Walker::NameScope& scope)
{
    env->SetWalker(scope.walker);
    env = scope.env;
}

bool Walker::IsClassWalker()
{
    return false;
}

PTree::Node *Walker::Translate(PTree::Node *p)
{
    if(p == 0)
	return p;
    else
	return p->Translate(this);
}

void Walker::Typeof(PTree::Node *p, TypeInfo& t)
{
    if(p != 0)
	p->Typeof(this, t);
}

// default translation

PTree::Node *Walker::TranslatePtree(PTree::Node *p)
{
    return p;
}

void Walker::TypeofPtree(PTree::Node *, TypeInfo& t)
{
    t.Unknown();
}

// translation for each class of node

PTree::Node *Walker::TranslateTypedef(PTree::Node *def)
{
  PTree::Node *tspec, *tspec2;

    tspec = PTree::second(def);
    tspec2 = TranslateTypespecifier(tspec);
    env->RecordTypedefName(PTree::third(def));
    if(tspec == tspec2)
	return def;
    else
	return new PTree::Typedef(PTree::first(def),
				  PTree::list(tspec2,
						    PTree::tail(def, 2)));
}

PTree::Node *Walker::TranslateTemplateDecl(PTree::Node *def)
{
    PTree::Node *body = PTree::nth(def, 4);
    PTree::Node *class_spec = GetClassTemplateSpec(body);
    if(class_spec->IsA(Token::ntClassSpec))
	return TranslateTemplateClass(def, class_spec);
    else
	return TranslateTemplateFunction(def, body);
}

PTree::Node *Walker::TranslateExternTemplate(PTree::Node *def)
{
    return def;
}

PTree::Node *Walker::TranslateTemplateClass(PTree::Node *temp_def, PTree::Node *class_spec)
{
    PTree::Node *userkey;
    PTree::Node *class_def;

    if(class_spec->car()->is_atom()){
	userkey = 0;
	class_def = class_spec;
    }
    else{
	userkey = class_spec->car();
	class_def = class_spec->cdr();
    }

    Class* metaobject = 0;
    if(PTree::length(class_def) == 4)
	metaobject = MakeTemplateClassMetaobject(temp_def, userkey, class_def);

    env->RecordTemplateClass(class_spec, metaobject);
    PTree::Node *class_spec2 = TranslateClassSpec(class_spec, userkey, class_def,
					    metaobject);
    if(class_spec == class_spec2)
	return temp_def;
    else
	return new PTree::TemplateDecl(temp_def->car(),
				       PTree::subst(class_spec2, class_spec,
							  temp_def->cdr()));
}

Class* Walker::MakeTemplateClassMetaobject(PTree::Node *def, PTree::Node *userkey,
					   PTree::Node *class_def)
{
    Class* metaobject = LookupMetaclass(def, userkey, class_def, true);
    if(metaobject == 0)
	metaobject = new TemplateClass;
    else
	if(metaobject->AcceptTemplate())
	    return metaobject;
	else{

	    ErrorMessage("the specified metaclass is not for templates.",
			 0, def);
	    metaobject = new TemplateClass;
	}

    metaobject->InitializeInstance(def, 0);
    return metaobject;
}

PTree::Node *Walker::TranslateTemplateFunction(PTree::Node *temp_def, PTree::Node *fun)
{
    env->RecordTemplateFunction(temp_def, fun);
    return temp_def;
}

PTree::Node *Walker::TranslateTemplateInstantiation(PTree::Node *inst_spec)
{
    PTree::Node *userkey;
    PTree::Node *class_spec;
    PTree::Node *full_class_spec = PTree::first(inst_spec);

    if(full_class_spec->car()->is_atom()){
	userkey = 0;
	class_spec = full_class_spec;
    }
    else{
	userkey = full_class_spec->car();
	class_spec = full_class_spec->cdr();
    }

    Class* metaobject = 0;
    metaobject = MakeTemplateInstantiationMetaobject(full_class_spec,
						     userkey, class_spec);
    return TranslateTemplateInstantiation(inst_spec, userkey, 
					  class_spec, metaobject);
}

Class* Walker::MakeTemplateInstantiationMetaobject(
    PTree::Node *full_class_spec, PTree::Node *userkey, PTree::Node *class_spec)
{
    // [class [foo [< ... >]]] -> [class foo]
    PTree::Node *class_name = PTree::first(PTree::second(class_spec));
    Bind* binding = 0;
    if (!env->Lookup(class_name,binding))
	return 0;

    Class* metaobject = 0;
    if (binding->What() != Bind::isTemplateClass) {
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
	else{
	    ErrorMessage("the specified metaclass is not for templates.",
			 0, full_class_spec);
	    metaobject = new TemplateClass;
	}

    return metaobject;
}

PTree::Node *Walker::TranslateTemplateInstantiation(PTree::Node *inst_spec,
		PTree::Node *userkey, PTree::Node *class_spec, Class* metaobject)
{
    if (metaobject == 0)
	return inst_spec;
    else {
      PTree::Node *class_spec2 = TranslateClassSpec(class_spec);
	if (class_spec == class_spec2)
	    return inst_spec;
	else
	    return class_spec2;
    }
}

PTree::Node *Walker::TranslateMetaclassDecl(PTree::Node *decl)
{
    env->RecordMetaclassName(decl);
    return decl;
}

PTree::Node *Walker::TranslateLinkageSpec(PTree::Node *def)
{
    PTree::Node *body = PTree::third(def);
    PTree::Node *body2 = Translate(body);
    if(body == body2)
	return def;
    else
	return new PTree::LinkageSpec(PTree::first(def),
				      PTree::list(PTree::second(def),
							body2));
}

PTree::Node *Walker::TranslateNamespaceSpec(PTree::Node *def)
{
    PTree::Node *body = PTree::third(def);
    PTree::Node *body2 = Translate(body);
    env->RecordNamespace(PTree::second(def));
    if(body == body2)
	return def;
    else
	return new PTree::NamespaceSpec(PTree::first(def),
					PTree::list(PTree::second(def),
							  body2));
}

PTree::Node *Walker::TranslateNamespaceAlias(PTree::Node *def)
{
  return def;
}

PTree::Node *Walker::TranslateUsing(PTree::Node *def)
{
    return def;
}

PTree::Node *Walker::TranslateDeclaration(PTree::Node *def)
{
    PTree::Node *decls = PTree::third(def);
    if(decls->IsA(Token::ntDeclarator))	// if it is a function
	return TranslateFunctionImplementation(def);
    else{
	// if it is a function prototype or a variable declaration.
	PTree::Node *decls2;
	PTree::Node *sspec = PTree::first(def);
	PTree::Node *sspec2 = TranslateStorageSpecifiers(sspec);
	PTree::Node *tspec = PTree::second(def);
	PTree::Node *tspec2 = TranslateTypespecifier(tspec);
	if(decls->is_atom())	// if it is ";"
	    decls2 = decls;
	else
	    decls2 = TranslateDeclarators(decls);

	if(sspec == sspec2 && tspec == tspec2 && decls == decls2)
	    return def;
	else if(decls2 == 0)
	    return new PTree::Declaration(0, PTree::list(0, Class::semicolon_t));
	else
	    return new PTree::Declaration(sspec2,
					  PTree::shallow_subst(tspec2, tspec,
							       decls2, decls,
							       def->cdr()));
    }
}

// TranslateStorageSpecifiers() also deals with inline, virtual, etc.

PTree::Node *Walker::TranslateStorageSpecifiers(PTree::Node *spec)
{
    return spec;
}

PTree::Node *Walker::TranslateDeclarators(PTree::Node *decls)
{
    return TranslateDeclarators(decls, true);
}

PTree::Node *Walker::TranslateDeclarators(PTree::Node *decls, bool record)
{
    PTree::Array array;
    bool changed = false;
    PTree::Node *rest = decls;
    while(rest != 0){
      PTree::Node *p, *q;
	int len;
	p = q = rest->car();
	if(p->IsA(Token::ntDeclarator)){
	  PTree::Node *exp, *exp2;

	    if(record)
		env->RecordDeclarator(p);

	    len = PTree::length(p);
	    exp = exp2 = 0;
	    if(len >= 2 && *PTree::nth(p, len - 2) == '='){
	      exp = PTree::tail(p, len - 2);
		exp2 = TranslateAssignInitializer((PTree::Declarator*)p, exp);
	    }
	    else{
		PTree::Node *last = PTree::last(p)->car();
		if(last != 0 && !last->is_atom() && *last->car() == '('){
		    exp = last;
		    exp2 = TranslateInitializeArgs((PTree::Declarator*)p, last);
		}
	    }

	    q = TranslateDeclarator(false, (PTree::Declarator*)p);
	    if(exp != exp2){
		// exp2 should be a list, but...
		if(exp2 != 0 && exp2->is_atom())
		    exp2 = PTree::list(exp2);

		if(p == q){
		    q = PTree::subst_sublist(exp2, exp, p->cdr());
		    q = new PTree::Declarator((PTree::Declarator*)p, p->car(), q);
		}
		else if(q != 0 && !q->is_atom())
		    q = new PTree::Declarator((PTree::Declarator*)p, q->car(),
					    PTree::subst(exp2, exp, q->cdr()));
	    }
	}

	if(q == 0){
	    changed = true;
	    rest = rest->cdr();
	    if(rest != 0)
		rest = rest->cdr();
	}
	else{
	    array.append(q);
	    if(p != q)
		changed = true;

	    rest = rest->cdr();
	    if(rest != 0){
		array.append(rest->car());
		rest = rest->cdr();
	    }
	}
    }

    if(changed)
	return array.all();
    else
	return decls;
}

PTree::Node *Walker::TranslateDeclarator(bool record, PTree::Declarator* decl)
{
    // if record is true, the formal arguments are recorded in the
    // current environment.

    PTree::Node *args;
    if(GetArgDeclList(decl, args)){
	PTree::Node *args2 = TranslateArgDeclList(record, decl, args);
	if(args == args2)
	    return decl;
	else
	    return new PTree::Declarator(decl, decl->car(),
					 PTree::subst(args2, args,
							    decl->cdr()));
    }
    else
	return decl;
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

PTree::Node *Walker::TranslateArgDeclList(bool record, PTree::Node *, PTree::Node *args)
{
    return TranslateArgDeclList2(record, env, false, false, 0, args);
}

// If translate is true, this function eliminates a user-defined keyword.

PTree::Node *Walker::TranslateArgDeclList2(bool record, Environment* e,
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
	    rest2 = TranslateArgDeclList2(record, e, translate, fill_args,
					  arg_name + 1, rest);
	    if(rest == rest2)
		rest = rest2 = args->cdr();
	    else
		rest2 = PTree::cons(PTree::cadr(args), rest2);
	}

	bool is_ellipsis = a->is_atom();		// a may be "..."
	if(is_ellipsis)
	    /* do nothing */;
	else if(a->car()->IsA(Token::ntUserdefKeyword)){
	    if(record)
		e->RecordDeclarator(PTree::third(a));

	    if(translate){
		a2 = a->cdr();
		if(fill_args)
		    a2 = FillArgumentName(a2, PTree::second(a2), arg_name);
	    }
	}
	else if(a->car()->IsA(Token::REGISTER)){
	    if(record)
		e->RecordDeclarator(PTree::third(a));

	    if(translate && fill_args){
		a2 = FillArgumentName(a, PTree::third(a), arg_name);
		if(a != a2)
		    a2 = PTree::cons(PTree::first(a), a2);
	    }
	}
	else{
	    if(record)
		e->RecordDeclarator(PTree::second(a));

	    if(translate && fill_args)
		a2 = FillArgumentName(a, PTree::second(a), arg_name);
	}

	if(a != a2 || rest != rest2)
	    return PTree::cons(a2, rest2);
	else
	    return args;
    }
}

PTree::Node *Walker::FillArgumentName(PTree::Node *arg, PTree::Node *d, int arg_name)
{
  PTree::Declarator* decl = (PTree::Declarator*)d;
  if(decl->Name()) return arg;
  else
  {
    const unsigned char* type = (const unsigned char*)decl->encoded_type();
    return Encoding::MakePtree(type, PTree::make(argument_name, arg_name));
  }
}

PTree::Node *Walker::TranslateAssignInitializer(PTree::Declarator*, PTree::Node *init)
{
  PTree::Node *exp = PTree::second(init);
  PTree::Node *exp2 = Translate(exp);
  if(exp == exp2) return init;
  else return PTree::list(init->car(), exp2);
}

PTree::Node *Walker::TranslateInitializeArgs(PTree::Declarator*, PTree::Node *init)
{
  return TranslateArguments(init);
}

PTree::Node *Walker::TranslateFunctionImplementation(PTree::Node *impl)
{
    PTree::Node *sspec = PTree::first(impl);
    PTree::Node *sspec2 = TranslateStorageSpecifiers(sspec);
    PTree::Node *tspec = PTree::second(impl);
    PTree::Node *decl = PTree::third(impl);
    PTree::Node *body = PTree::nth(impl, 3);
    PTree::Node *decl2;
    PTree::Node *body2;

    PTree::Node *tspec2 = TranslateTypespecifier(tspec);
    Environment* fenv = env->RecordDeclarator(decl);
    if(fenv == 0){
	// reach here if resolving the qualified name fails. error?
	NewScope();
	decl2 = TranslateDeclarator(true, (PTree::Declarator*)decl);
	body2 = Translate(body);
	ExitScope();
    }
    else{
	NameScope old_env = ChangeScope(fenv);
	NewScope();
	decl2 = TranslateDeclarator(true, (PTree::Declarator*)decl);
	body2 = Translate(body);
	ExitScope();
	RestoreScope(old_env);
    }

    if(sspec == sspec2 && tspec == tspec2 && decl == decl2 && body == body2)
	return impl;
    else
	return new PTree::Declaration(sspec2,
				      PTree::list(tspec2, decl2, body2));
}

PTree::Node *Walker::RecordArgsAndTranslateFbody(Class*, PTree::Node *args, PTree::Node *body)
{
    NewScope();
    TranslateArgDeclList2(true, env, false, false, 0, args);
    PTree::Node *body2 = TranslateFunctionBody(body);
    ExitScope();
    return body2;
}

PTree::Node *Walker::TranslateFunctionBody(PTree::Node *body)
{
    return Translate(body);
}

PTree::Node *Walker::TranslateBrace(PTree::Node *block)
{
    PTree::Array array;
    bool changed = false;
    PTree::Node *body = PTree::second(block);
    PTree::Node *rest = body;
    while(rest != 0){
	PTree::Node *p = rest->car();
	PTree::Node *q = Translate(p);
	array.append(q);
	if(p != q)
	    changed = true;

	rest = rest->cdr();
    }

    if(changed)
	return new PTree::Brace(PTree::first(block), array.all(),
				PTree::third(block));
    else
	return block;
}

PTree::Node *Walker::TranslateBlock(PTree::Node *block)
{
    PTree::Node *block2;

    NewScope();

    PTree::Array array;
    bool changed = false;
    PTree::Node *body = PTree::second(block);
    PTree::Node *rest = body;
    while(rest != 0){
	PTree::Node *p = rest->car();
	PTree::Node *q = Translate(p);
	array.append(q);
	if(p != q)
	    changed = true;

	rest = rest->cdr();
    }

    if(changed)
	block2 = new PTree::Block(PTree::first(block), array.all(),
				  PTree::third(block));
    else
	block2 = block;

    ExitScope();
    return block2;
}

PTree::Node *Walker::TranslateClassBody(PTree::Node *block, PTree::Node *bases,
				  Class* metaobject)
{
    PTree::Node *block2;

    NewScope(metaobject);
    RecordBaseclassEnv(bases);

    PTree::Array array;
    bool changed = false;
    PTree::Node *body = PTree::second(block);
    PTree::Node *rest = body;
    while(rest != 0){
	PTree::Node *p = rest->car();
	PTree::Node *q = Translate(p);
	array.append(q);
	if(p != q)
	    changed = true;

	rest = rest->cdr();
    }

    if(changed)
	block2 = new PTree::ClassBody(PTree::first(block), array.all(),
				      PTree::third(block));
    else
	block2 = block;

    ExitScope();
    return block2;
}

PTree::Node *Walker::TranslateClassSpec(PTree::Node *spec)
{
    PTree::Node *userkey;
    PTree::Node *class_def;

    if(spec->car()->is_atom()){
	userkey = 0;
	class_def = spec;
    }
    else{
	userkey = spec->car();
	class_def = spec->cdr();
    }

    Class* metaobject = 0;
    if(PTree::length(class_def) == 4)
	metaobject = MakeClassMetaobject(spec, userkey, class_def);

    env->RecordClassName(spec->encoded_name(), metaobject);
    return TranslateClassSpec(spec, userkey, class_def, metaobject);
}

Class* Walker::MakeClassMetaobject(PTree::Node *def, PTree::Node *userkey,
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

PTree::Node *Walker::TranslateClassSpec(PTree::Node *spec, PTree::Node *,
				  PTree::Node *class_def, Class* metaobject)
{
    if(metaobject == 0)
	return spec;
    else{
	// a class body is specified.
      PTree::Node *body = PTree::nth(class_def, 3);
	PTree::Node *body2 = TranslateClassBody(body, PTree::third(class_def),
					  metaobject);
	if(body == body2)
	    return spec;
	else
	    return new PTree::ClassSpec(spec->car(),
					PTree::shallow_subst(body2, body,
							     spec->cdr()),
					0, spec->encoded_name());
    }
}


PTree::Node *Walker::TranslateEnumSpec(PTree::Node *spec)
{
    env->RecordEnumName(spec);
    return spec;
}

PTree::Node *Walker::TranslateAccessSpec(PTree::Node *p)
{
    return p;
}

PTree::Node *Walker::TranslateAccessDecl(PTree::Node *p)
{
    return p;
}

PTree::Node *Walker::TranslateUserAccessSpec(PTree::Node *p)
{
    return p;
}

PTree::Node *Walker::TranslateIf(PTree::Node *s)
{
    PTree::Node *cond = PTree::third(s);
    PTree::Node *cond2 = Translate(cond);
    PTree::Node *then_part = PTree::nth(s, 4);
    PTree::Node *then_part2 = Translate(then_part);
    PTree::Node *else_part = PTree::nth(s, 6);
    PTree::Node *else_part2 = Translate(else_part);

    if(cond == cond2 && then_part == then_part2 && else_part == else_part2)
	return s;
    else{
	PTree::Node *rest = PTree::shallow_subst(cond2, cond, then_part2, then_part,
						 else_part2, else_part, s->cdr());
	return new PTree::IfStatement(s->car(), rest);
    }
}

PTree::Node *Walker::TranslateSwitch(PTree::Node *s)
{
  PTree::Node *cond = PTree::third(s);
    PTree::Node *cond2 = Translate(cond);
    PTree::Node *body = PTree::nth(s, 4);
    PTree::Node *body2 = Translate(body);
    if(cond == cond2 && body == body2)
	return s;
    else{
	PTree::Node *rest = PTree::shallow_subst(cond2, cond, body2, body, s->cdr());
	return new PTree::SwitchStatement(s->car(), rest);
    }
}

PTree::Node *Walker::TranslateWhile(PTree::Node *s)
{
    PTree::Node *cond = PTree::third(s);
    PTree::Node *cond2 = Translate(cond);
    PTree::Node *body = PTree::nth(s, 4);
    PTree::Node *body2 = Translate(body);
    if(cond == cond2 && body == body2)
	return s;
    else{
	PTree::Node *rest = PTree::shallow_subst(cond2, cond, body2, body, s->cdr());
	return new PTree::WhileStatement(s->car(), rest);
    }
}

PTree::Node *Walker::TranslateDo(PTree::Node *s)
{
    PTree::Node *body = PTree::second(s);
    PTree::Node *body2 = Translate(body);
    PTree::Node *cond = PTree::nth(s, 4);
    PTree::Node *cond2 = Translate(cond);
    if(cond == cond2 && body == body2)
	return s;
    else{
	PTree::Node *rest = PTree::shallow_subst(body2, body, cond2, cond, s->cdr());
	return new PTree::DoStatement(s->car(), rest);
    }
}

PTree::Node *Walker::TranslateFor(PTree::Node *s)
{
    NewScope();
    PTree::Node *exp1 = PTree::third(s);
    PTree::Node *exp1t = Translate(exp1);
    PTree::Node *exp2 = PTree::nth(s, 3);
    PTree::Node *exp2t = Translate(exp2);
    PTree::Node *exp3 = PTree::nth(s, 5);
    PTree::Node *exp3t = Translate(exp3);
    PTree::Node *body = PTree::nth(s, 7);
    PTree::Node *body2 = Translate(body);
    ExitScope();

    if(exp1 == exp1t && exp2 == exp2t && exp3 == exp3t && body == body2)
	return s;
    else{
	PTree::Node *rest = PTree::shallow_subst(exp1t, exp1, exp2t, exp2,
					  exp3t, exp3, body2, body, s->cdr());
	return new PTree::ForStatement(s->car(), rest);
    }
}

PTree::Node *Walker::TranslateTry(PTree::Node *s)
{
    PTree::Node *try_block = PTree::second(s);
    PTree::Node *try_block2 = Translate(try_block);

    PTree::Array array;
    PTree::Node *handlers = PTree::cddr(s);
    bool changed = false;

    while(handlers != 0){
	PTree::Node *handle = handlers->car();
	PTree::Node *body = PTree::nth(handle, 4);
	PTree::Node *body2 = Translate(body);
	if(body == body2)
	    array.append(handle);
	else{
	    array.append(PTree::shallow_subst(body2, body, handle));
	    changed = true;
	}

	handlers = handlers->cdr();
    }

    if(try_block == try_block2 && !changed)
	return s;
    else
	return new PTree::TryStatement(s->car(),
				       PTree::cons(try_block2, array.all()));
}

PTree::Node *Walker::TranslateBreak(PTree::Node *s)
{
    return s;
}

PTree::Node *Walker::TranslateContinue(PTree::Node *s)
{
    return s;
}

PTree::Node *Walker::TranslateReturn(PTree::Node *s)
{
    if(PTree::length(s) == 2)
	return s;
    else{
	PTree::Node *exp = PTree::second(s);
	PTree::Node *exp2 = Translate(exp);
	if(exp == exp2)
	    return s;
	else
	    return new PTree::ReturnStatement(s->car(),
					      PTree::shallow_subst(exp2, exp,
								   s->cdr()));
    }
}

PTree::Node *Walker::TranslateGoto(PTree::Node *s)
{
    return s;
}

PTree::Node *Walker::TranslateCase(PTree::Node *s)
{
  PTree::Node *st = PTree::nth(s, 3);
    PTree::Node *st2 = Translate(st);
    if(st == st2)
	return s;
    else
	return new PTree::CaseStatement(s->car(),
					PTree::shallow_subst(st2, st, s->cdr()));
}

PTree::Node *Walker::TranslateDefault(PTree::Node *s)
{
    PTree::Node *st = PTree::third(s);
    PTree::Node *st2 = Translate(st);
    if(st == st2)
	return s;
    else
	return new PTree::DefaultStatement(s->car(),
					   PTree::list(PTree::cadr(s), st2));
}

PTree::Node *Walker::TranslateLabel(PTree::Node *s)
{
    PTree::Node *st = PTree::third(s);
    PTree::Node *st2 = Translate(st);
    if(st == st2)
	return s;
    else
	return new PTree::LabelStatement(s->car(),
					 PTree::list(PTree::cadr(s), st2));
}

PTree::Node *Walker::TranslateExprStatement(PTree::Node *s)
{
    PTree::Node *exp = PTree::first(s);
    PTree::Node *exp2 = Translate(exp);
    if(exp == exp2)
	return s;
    else
      return new PTree::ExprStatement(exp2, s->cdr());
}

PTree::Node *Walker::TranslateTypespecifier(PTree::Node *tspec)
{
  PTree::Node *class_spec, *class_spec2;

    class_spec = GetClassOrEnumSpec(tspec);
    if(class_spec == 0)
	class_spec2 = 0;
    else
	class_spec2 = Translate(class_spec);

    if(class_spec == class_spec2)
	return tspec;
    else
	return PTree::shallow_subst(class_spec2, class_spec, tspec);
}

PTree::Node *Walker::GetClassOrEnumSpec(PTree::Node *typespec)
{
    PTree::Node *spec = StripCvFromIntegralType(typespec);
    if(spec->IsA(Token::ntClassSpec, Token::ntEnumSpec))
	return spec;

    return 0;
}

PTree::Node *Walker::GetClassTemplateSpec(PTree::Node *body)
{
    if(*PTree::third(body) == ';'){
	PTree::Node *spec = StripCvFromIntegralType(PTree::second(body));
	if(spec->IsA(Token::ntClassSpec))
	    return spec;
    }

    return 0;
}

PTree::Node *Walker::StripCvFromIntegralType(PTree::Node *integral)
{
    if(integral == 0)
	return 0;

    if(!integral->is_atom())
	if(integral->car()->IsA(Token::CONST, Token::VOLATILE))
	    return PTree::second(integral);
	else if(PTree::second(integral)->IsA(Token::CONST, Token::VOLATILE))
	    return integral->car();

    return integral;
}

PTree::Node *Walker::TranslateComma(PTree::Node *exp)
{
    PTree::Node *left = PTree::first(exp);
    PTree::Node *left2 = Translate(left);
    PTree::Node *right = PTree::third(exp);
    PTree::Node *right2 = Translate(right);
    if(left == left2 && right == right2)
	return exp;
    else
	return new PTree::CommaExpr(left2, PTree::list(PTree::second(exp), right2));
}

void Walker::TypeofComma(PTree::Node *exp, TypeInfo& t)
{
    Typeof(PTree::third(exp), t);
}

PTree::Node *Walker::TranslateAssign(PTree::Node *exp)
{
    PTree::Node *left = PTree::first(exp);
    PTree::Node *left2 = Translate(left);
    PTree::Node *right = PTree::third(exp);
    PTree::Node *right2 = Translate(right);
    if(left == left2 && right == right2)
	return exp;
    else
	return new PTree::AssignExpr(left2, PTree::list(PTree::second(exp), right2));
}

void Walker::TypeofAssign(PTree::Node *exp, TypeInfo& t)
{
    Typeof(PTree::first(exp), t);
}

PTree::Node *Walker::TranslateCond(PTree::Node *exp)
{
    PTree::Node *c = PTree::first(exp);
    PTree::Node *c2 = Translate(c);
    PTree::Node *t = PTree::third(exp);
    PTree::Node *t2 = Translate(t);
    PTree::Node *e = PTree::nth(exp, 4);
    PTree::Node *e2 = Translate(e);
    if(c == c2 && t == t2 && e == e2)
	return exp;
    else
	return new PTree::CondExpr(c2, PTree::list(PTree::second(exp), t2,
							 PTree::nth(exp, 3), e2));
}

void Walker::TypeofCond(PTree::Node *exp, TypeInfo& t)
{
    Typeof(PTree::third(exp), t);
}

PTree::Node *Walker::TranslateInfix(PTree::Node *exp)
{
    PTree::Node *left = PTree::first(exp);
    PTree::Node *left2 = Translate(left);
    PTree::Node *right = PTree::third(exp);
    PTree::Node *right2 = Translate(right);
    if(left == left2 && right == right2)
	return exp;
    else
	return new PTree::InfixExpr(left2, PTree::list(PTree::second(exp), right2));
}

void Walker::TypeofInfix(PTree::Node *exp, TypeInfo& t)
{
    Typeof(PTree::first(exp), t);
}

PTree::Node *Walker::TranslatePm(PTree::Node *exp)
{
    PTree::Node *left = PTree::first(exp);
    PTree::Node *left2 = Translate(left);
    PTree::Node *right = PTree::third(exp);
    PTree::Node *right2 = Translate(right);
    if(left == left2 && right == right2)
	return exp;
    else
	return new PTree::PmExpr(left2, PTree::list(PTree::second(exp), right2));
}

void Walker::TypeofPm(PTree::Node *exp, TypeInfo& t)
{
    Typeof(PTree::third(exp), t);
    t.Dereference();
}

PTree::Node *Walker::TranslateCast(PTree::Node *exp)
{
  PTree::Node *e = PTree::nth(exp, 3);
    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::CastExpr(PTree::first(exp),
				   PTree::shallow_subst(e2, e, exp->cdr()));
}

void Walker::TypeofCast(PTree::Node *exp, TypeInfo& t)
{
    t.Set(PTree::second(PTree::second(exp))->encoded_type(), env);
}

PTree::Node *Walker::TranslateUnary(PTree::Node *exp)
{
    PTree::Node *oprnd = PTree::second(exp);
    PTree::Node *oprnd2 = Translate(oprnd);
    if(oprnd == oprnd2)
	return exp;
    else
	return new PTree::UnaryExpr(PTree::first(exp), PTree::list(oprnd2));
}

void Walker::TypeofUnary(PTree::Node *exp, TypeInfo& t)
{
    Typeof(PTree::second(exp), t);

    PTree::Node *op = PTree::first(exp);
    if(*op == '*')
	t.Dereference();
    else if(*op == '&')
	t.Reference();
}

PTree::Node *Walker::TranslateThrow(PTree::Node *exp)
{
    PTree::Node *oprnd = PTree::second(exp);
    PTree::Node *oprnd2 = Translate(oprnd);
    if(oprnd == oprnd2)
	return exp;
    else
	return new PTree::ThrowExpr(PTree::first(exp), PTree::list(oprnd2));
}

void Walker::TypeofThrow(PTree::Node *, TypeInfo& t)
{
    t.SetVoid();
}

PTree::Node *Walker::TranslateSizeof(PTree::Node *exp)
{
    PTree::Node *e = PTree::second(exp);
    if(*e == '(')
	e = PTree::third(exp);

    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::SizeofExpr(PTree::first(exp),
				     PTree::shallow_subst(e2, e, exp->cdr()));
}

void Walker::TypeofSizeof(PTree::Node *, TypeInfo& t)
{
    t.SetInt();
}

PTree::Node *Walker::TranslateTypeid(PTree::Node *exp)
{
    PTree::Node *e = PTree::second(exp);
    if(*e == '(')
	e = PTree::third(exp);

    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::TypeidExpr(PTree::first(exp),
				     PTree::shallow_subst(e2, e, exp->cdr()));
}

void Walker::TypeofTypeid(PTree::Node *exp, TypeInfo& t)
{
    t.SetInt(); // FIXME: Should be the type_info type (exp->Third()->Second()->GetEncodedType(), env);
}


PTree::Node *Walker::TranslateTypeof(PTree::Node *exp)
{
    PTree::Node *e = PTree::second(exp);
    if(*e == '(')
	e = PTree::third(exp);

    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::TypeofExpr(PTree::first(exp),
				     PTree::shallow_subst(e2, e, exp->cdr()));
}

void Walker::TypeofTypeof(PTree::Node *exp, TypeInfo& t)
{
    t.SetInt(); // FIXME: Should be the type_info type (exp->Third()->Second()->GetEncodedType(), env);
}


PTree::Node *Walker::TranslateNew(PTree::Node *exp)
{
  PTree::Node *p;
  PTree::Node *userkey, *scope, *op, *placement, *type, *init;

    p = exp;
    userkey = p->car();
    if(userkey == 0 || !userkey->is_atom())
	p = exp->cdr();		// user keyword
    else
	userkey = 0;

    if(*p->car() == "::"){
	scope = p->car();
	p = p->cdr();
    }
    else
	scope = 0;

    op = p->car();
    placement = PTree::cadr(p);
    type = PTree::third(p);
    init = PTree::nth(p, 3);
    return TranslateNew2(exp, userkey, scope, op, placement, type, init);
}

PTree::Node *Walker::TranslateNew2(PTree::Node *exp, PTree::Node *, PTree::Node *,
			     PTree::Node *, PTree::Node *placement,
			     PTree::Node *type, PTree::Node *init)
{
    PTree::Node *placement2 = TranslateArguments(placement);
    PTree::Node *type2 = TranslateNew3(type);
    PTree::Node *init2 = TranslateArguments(init);
    if(placement == placement2 && init == init2)
	return exp;
    else
	return new PTree::NewExpr(exp->car(),
				  PTree::shallow_subst(placement2, placement,
						       type2, type,
						       init2, init,
						       exp->cdr()));
}

PTree::Node *Walker::TranslateNew3(PTree::Node *type)
{
    PTree::Node *p = type;
    if(*p->car() == '(')
	p = PTree::second(p);

    PTree::Node *decl = PTree::second(p);
    PTree::Node *decl2 = TranslateNewDeclarator(decl);
    if(decl == decl2)
	return type;
    else
	return PTree::subst(decl2, decl, type);
}

void Walker::TypeofNew(PTree::Node *exp, TypeInfo& t)
{
  PTree::Node *p, *userkey, *type;

    p = exp;
    userkey = p->car();
    if(userkey == 0 || !userkey->is_atom())
	p = exp->cdr();		// user keyword

    if(*p->car() == "::")
	p = p->cdr();

    type = PTree::third(p);

    if(*type->car() == '(')
	t.Set(PTree::second(PTree::second(type))->encoded_type(), env);
    else
	t.Set(PTree::second(type)->encoded_type(), env);

    t.Reference();
}

PTree::Node *Walker::TranslateDelete(PTree::Node *exp)
{
    PTree::Node *obj = PTree::last(exp)->car();
    PTree::Node *obj2 = Translate(obj);
    if(obj == obj2)
	return exp;
    else
	return new PTree::DeleteExpr(exp->car(),
				     PTree::shallow_subst(obj2, obj,
							  exp->cdr()));
}

void Walker::TypeofDelete(PTree::Node *, TypeInfo& t)
{
    t.SetVoid();
}

PTree::Node *Walker::TranslateThis(PTree::Node *exp)
{
    return exp;
}

void Walker::TypeofThis(PTree::Node *, TypeInfo& t)
{
    t.Set(env->LookupThis());
}

PTree::Node *Walker::TranslateVariable(PTree::Node *exp)
{
    return exp;
}

/*
  This may be a class name if the expression is a function-style cast.
*/
void Walker::TypeofVariable(PTree::Node *exp, TypeInfo& t)
{
    bool is_type_name;

    if(env->Lookup(exp, is_type_name, t))
	if(is_type_name)	// if exp is a class name
	    t.Reference();	// see TypeofFuncall
}

/*
  TranslateFstyleCast() deals with function-style casts
  to an integral type
*/
PTree::Node *Walker::TranslateFstyleCast(PTree::Node *exp)
{
    PTree::Node *args = exp->cdr();
    PTree::Node *args2 = TranslateArguments(args);
    if(args == args2)
	return exp;
    else
	return new PTree::FstyleCastExpr(exp->encoded_type(), exp->car(),
					 args2);
}

void Walker::TypeofFstyleCast(PTree::Node *exp, TypeInfo& t)
{
    t.Set(exp->encoded_type(), env);
}

PTree::Node *Walker::TranslateArray(PTree::Node *exp)
{
    PTree::Node *array = exp->car();
    PTree::Node *array2 = Translate(array);
    PTree::Node *index = PTree::third(exp);
    PTree::Node *index2 = Translate(index);
    if(array == array2 && index == index2)
	return exp;
    else
	return new PTree::ArrayExpr(array2, PTree::subst(index2, index,
							       exp->cdr()));
}

void Walker::TypeofArray(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->car(), t);
    t.Dereference();
}

/*
  TranslateFuncall() also deals with function-style casts to a class.
*/
PTree::Node *Walker::TranslateFuncall(PTree::Node *exp)
{
    PTree::Node *fun = exp->car();
    PTree::Node *fun2 = Translate(fun);
    PTree::Node *args = exp->cdr();
    PTree::Node *args2 = TranslateArguments(args);
    if(fun == fun2 && args == args2)
	return exp;
    else
	return new PTree::FuncallExpr(fun2, args2);
}

void Walker::TypeofFuncall(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->car(), t);
    if(!t.IsFunction())
	t.Dereference();	// maybe a pointer to a function

    t.Dereference();
}

PTree::Node *Walker::TranslatePostfix(PTree::Node *exp)
{
    PTree::Node *left = exp->car();
    PTree::Node *left2 = Translate(left);
    if(left == left2)
	return exp;
    else
	return new PTree::PostfixExpr(left2, exp->cdr());
}

void Walker::TypeofPostfix(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->car(), t);
}

PTree::Node *Walker::TranslateUserStatement(PTree::Node *exp)
{
    return exp;
}

void Walker::TypeofUserStatement(PTree::Node *, TypeInfo& t)
{
    t.Unknown();
}

PTree::Node *Walker::TranslateDotMember(PTree::Node *exp)
{
    PTree::Node *left = exp->car();
    PTree::Node *left2 = Translate(left);
    if(left == left2)
	return exp;
    else
	return new PTree::DotMemberExpr(left2, exp->cdr());
}

void Walker::TypeofDotMember(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->car(), t);
    t.SetMember(PTree::third(exp));
}

PTree::Node *Walker::TranslateArrowMember(PTree::Node *exp)
{
    PTree::Node *left = exp->car();
    PTree::Node *left2 = Translate(left);
    if(left == left2)
	return exp;
    else
	return new PTree::ArrowMemberExpr(left2, exp->cdr());
}

void Walker::TypeofArrowMember(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->car(), t);
    t.Dereference();
    t.SetMember(PTree::third(exp));
}

PTree::Node *Walker::TranslateParen(PTree::Node *exp)
{
    PTree::Node *e = PTree::second(exp);
    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::ParenExpr(exp->car(), PTree::list(e2, PTree::third(exp)));
}

void Walker::TypeofParen(PTree::Node *exp, TypeInfo& t)
{
    Typeof(PTree::second(exp), t);
}

PTree::Node *Walker::TranslateStaticUserStatement(PTree::Node *exp)
{
    return exp;
}

void Walker::TypeofStaticUserStatement(PTree::Node *, TypeInfo& t)
{
    t.Unknown();
}

PTree::Node *Walker::TranslateNewDeclarator(PTree::Node *decl)
{
    PTree::Node *decl2 = decl;
    PTree::Node *p = decl;
    while(p != 0){
	PTree::Node *head = p->car();
	if(head == 0)
	    return decl;
	else if(*head == '['){
	    PTree::Node *p2 = TranslateNewDeclarator2(p);
	    if(p == p2)
		return decl;
	    else{
		decl2 = PTree::shallow_subst(p2, p, decl);
		break;
	    }
	}
	else if(!head->is_atom() && *head->car() == '('){
	    PTree::Node *d = PTree::cadr(head);
	    PTree::Node *d2 = TranslateNewDeclarator(d);
	    decl2 = PTree::shallow_subst(d2, d, decl);
	    break;
	}

	p = p->cdr();
    }

    if(p == 0)
	return decl;
    else if(decl->IsA(Token::ntDeclarator))
	return new PTree::Declarator((PTree::Declarator*)decl,
				     decl2->car(), decl2->cdr());
    else
	return decl2;
}

PTree::Node *Walker::TranslateNewDeclarator2(PTree::Node *decl)
{
    for(PTree::Node *p = decl; p != 0; p = p->cdr()){
	PTree::Node *head = p->car();
	if(*head == '['){
	    PTree::Node *size = PTree::cadr(p);
	    PTree::Node *size2 = Translate(size);
	    if(size != size2){
		PTree::Node *q = TranslateNewDeclarator2(PTree::tail(p, 3));
		return PTree::nconc(PTree::list(p->car(), size2, PTree::third(p)),
				    q);
	    }
	}
	else if(*head == '(')
	    break;
    }

    return decl;
}

PTree::Node *Walker::TranslateArguments(PTree::Node *arglist)
{
    if(arglist == 0)
	return arglist;

    PTree::Array array;
    bool changed = false;
    PTree::Node *body = PTree::second(arglist);
    PTree::Node *args = body;
    while(args != 0){
	PTree::Node *p = args->car();
	PTree::Node *q = Translate(p);
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
    if (!leaf) { std::cerr << "Warning: Failed to find leaf when trying to add comments." << std::endl;
	parent->print(std::cout);
	return; }

    if (!(cleaf = dynamic_cast<PTree::CommentedAtom *>(leaf))) {
	// Must change first child of parent to be a commented leaf
        Token tk(leaf->position(), leaf->length(), Token::Comment);
	cleaf = new (GC) PTree::CommentedAtom(tk, comments);
	parent->set_car(cleaf);
    } else {
	// Already is a commented leaf, so add the comments to it
	comments = PTree::snoc(cleaf->GetComments(), comments);
	cleaf->SetComments(comments);
    }
}

void Walker::SetDeclaratorComments(PTree::Node *def, PTree::Node *comments)
{
    if (def == 0 || !def->IsA(Token::ntDeclaration))
	return;

    PTree::Node *decl;
    int n = 0;
    for (;;) {
	int i = n++;
	decl = NthDeclarator(def, i);
	if (decl == 0)
	    break;
	else if (decl->IsA(Token::ntDeclarator))
	    ((PTree::Declarator*)decl)->SetComments(comments);
    }
}

PTree::Node *Walker::NthDeclarator(PTree::Node *def, int& nth)
{
    PTree::Node *decls = PTree::third(def);
    if(decls == 0 || decls->is_atom())
	return 0;

    if(decls->IsA(Token::ntDeclarator)){	// if it is a function
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

PTree::Node *Walker::FindDeclarator(PTree::Node *def, const char* name, int len,
			      const char* signature, int& nth, Environment* e)
{
    PTree::Node *decls = PTree::third(def);
    if(decls == 0 || decls->is_atom())
	return 0;

    if(decls->IsA(Token::ntDeclarator)){	// if it is a function
	if(MatchedDeclarator(decls, name, len, signature, e))
	    return decls;

	++nth;
    }
    else
	while(decls != 0){
	    PTree::Node *d = decls->car();
	    if(MatchedDeclarator(d, name, len, signature, e))
		return d;

	    ++nth;
	    if((decls = decls->cdr()) != 0)
		decls = decls->cdr();		// skip ,
	}

    return 0;
}

bool Walker::MatchedDeclarator(PTree::Node *decl, const char* name, int len,
			       const char* signature, Environment* e)
{
    const char* str;
    int strlen;
    const char* sig;

    str = decl->encoded_name();
    sig = decl->encoded_type();
    if(str == 0 || sig == 0)
	return false;

    str = Encoding::GetBaseName(str, strlen, e);
    return bool(len == strlen && memcmp(name, str, len) == 0
		&& strcmp(signature, sig) == 0);
}

bool Walker::WhichDeclarator(PTree::Node *def, PTree::Node *name, int& nth,
			     Environment* env)
{
    const char* str;
    int len;
    Environment* e;
    PTree::Node *decls = PTree::third(def);
    if(decls == 0 || decls->is_atom())
	return false;

    if(decls->IsA(Token::ntDeclarator)){	// if it is a function
	str = decls->encoded_name();
	e = env;
	str = Encoding::GetBaseName(str, len, e);
	if(equal(*name, str, len))
	    return true;

	++nth;
    }
    else
	while(decls != 0){
	    str = decls->car()->encoded_name();
	    e = env;
	    str = Encoding::GetBaseName(str, len, e);
	    if(equal(*name, str, len))
		return true;

	    ++nth;
	    if((decls = decls->cdr()) != 0)
		decls = decls->cdr();
	}

    return false;
}

void Walker::ErrorMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
    parser->ErrorMessage(msg, name, where);
}

void Walker::WarningMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
    parser->WarningMessage(msg, name, where);
}

// InaccurateErrorMessage() may report a wrong line number.

void Walker::InaccurateErrorMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
    if(default_parser == 0)
	MopErrorMessage("Walker::InaccurateErrorMessage()",
			"no default parser");
    else
	default_parser->ErrorMessage(msg, name, where);
}

void Walker::InaccurateWarningMessage(const char* msg, PTree::Node *name, PTree::Node *where)
{
    if(default_parser == 0)
	MopErrorMessage("Walker::InaccurateWarningMessage()",
			"no default parser");
    else
	default_parser->WarningMessage(msg, name, where);
}
