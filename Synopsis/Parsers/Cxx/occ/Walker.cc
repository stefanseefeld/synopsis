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
char* Walker::argument_name = "_arg_%d_";
char* Walker::default_metaclass = 0;

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
	bases = bases->Cdr();		// skip : or ,
	PTree::Node *base_class = bases->Car()->Last()->Car();
	Class* metaobject = env->LookupClassMetaobject(base_class);
	if(metaobject != 0){
	    Environment* e = metaobject->GetEnvironment();
	    if(e != 0)
		env->AddBaseclassEnv(e);
	}

	bases = bases->Cdr();
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

    tspec = PTree::Node::Second(def);
    tspec2 = TranslateTypespecifier(tspec);
    env->RecordTypedefName(PTree::Node::Third(def));
    if(tspec == tspec2)
	return def;
    else
	return new PTree::Typedef(PTree::Node::First(def),
				  PTree::Node::List(tspec2,
						    PTree::Node::ListTail(def, 2)));
}

PTree::Node *Walker::TranslateTemplateDecl(PTree::Node *def)
{
    PTree::Node *body = PTree::Node::Nth(def, 4);
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

    if(class_spec->Car()->IsLeaf()){
	userkey = 0;
	class_def = class_spec;
    }
    else{
	userkey = class_spec->Car();
	class_def = class_spec->Cdr();
    }

    Class* metaobject = 0;
    if(PTree::Node::Length(class_def) == 4)
	metaobject = MakeTemplateClassMetaobject(temp_def, userkey, class_def);

    env->RecordTemplateClass(class_spec, metaobject);
    PTree::Node *class_spec2 = TranslateClassSpec(class_spec, userkey, class_def,
					    metaobject);
    if(class_spec == class_spec2)
	return temp_def;
    else
	return new PTree::TemplateDecl(temp_def->Car(),
				       PTree::Node::Subst(class_spec2, class_spec,
							  temp_def->Cdr()));
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
    PTree::Node *full_class_spec = PTree::Node::First(inst_spec);

    if(full_class_spec->Car()->IsLeaf()){
	userkey = 0;
	class_spec = full_class_spec;
    }
    else{
	userkey = full_class_spec->Car();
	class_spec = full_class_spec->Cdr();
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
    PTree::Node *class_name = PTree::Node::First(PTree::Node::Second(class_spec));
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
    PTree::Node *body = PTree::Node::Third(def);
    PTree::Node *body2 = Translate(body);
    if(body == body2)
	return def;
    else
	return new PTree::LinkageSpec(PTree::Node::First(def),
				      PTree::Node::List(PTree::Node::Second(def),
							body2));
}

PTree::Node *Walker::TranslateNamespaceSpec(PTree::Node *def)
{
    PTree::Node *body = PTree::Node::Third(def);
    PTree::Node *body2 = Translate(body);
    env->RecordNamespace(PTree::Node::Second(def));
    if(body == body2)
	return def;
    else
	return new PTree::NamespaceSpec(PTree::Node::First(def),
					PTree::Node::List(PTree::Node::Second(def),
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
    PTree::Node *decls = PTree::Node::Third(def);
    if(decls->IsA(Token::ntDeclarator))	// if it is a function
	return TranslateFunctionImplementation(def);
    else{
	// if it is a function prototype or a variable declaration.
	PTree::Node *decls2;
	PTree::Node *sspec = PTree::Node::First(def);
	PTree::Node *sspec2 = TranslateStorageSpecifiers(sspec);
	PTree::Node *tspec = PTree::Node::Second(def);
	PTree::Node *tspec2 = TranslateTypespecifier(tspec);
	if(decls->IsLeaf())	// if it is ";"
	    decls2 = decls;
	else
	    decls2 = TranslateDeclarators(decls);

	if(sspec == sspec2 && tspec == tspec2 && decls == decls2)
	    return def;
	else if(decls2 == 0)
	    return new PTree::Declaration(0, PTree::Node::List(0, Class::semicolon_t));
	else
	    return new PTree::Declaration(sspec2,
					  PTree::Node::ShallowSubst(tspec2, tspec,
								    decls2, decls,
								    def->Cdr()));
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
	p = q = rest->Car();
	if(p->IsA(Token::ntDeclarator)){
	  PTree::Node *exp, *exp2;

	    if(record)
		env->RecordDeclarator(p);

	    len = p->Length();
	    exp = exp2 = 0;
	    if(len >= 2 && p->Nth(len - 2)->Eq('=')){
		exp = p->ListTail(len - 2);
		exp2 = TranslateAssignInitializer((PTree::Declarator*)p, exp);
	    }
	    else{
		PTree::Node *last = p->Last()->Car();
		if(last != 0 && !last->IsLeaf() && last->Car()->Eq('(')){
		    exp = last;
		    exp2 = TranslateInitializeArgs((PTree::Declarator*)p, last);
		}
	    }

	    q = TranslateDeclarator(false, (PTree::Declarator*)p);
	    if(exp != exp2){
		// exp2 should be a list, but...
		if(exp2 != 0 && exp2->IsLeaf())
		    exp2 = PTree::Node::List(exp2);

		if(p == q){
		    q = PTree::Node::SubstSublist(exp2, exp, p->Cdr());
		    q = new PTree::Declarator((PTree::Declarator*)p, p->Car(), q);
		}
		else if(q != 0 && !q->IsLeaf())
		    q = new PTree::Declarator((PTree::Declarator*)p, q->Car(),
					    PTree::Node::Subst(exp2, exp, q->Cdr()));
	    }
	}

	if(q == 0){
	    changed = true;
	    rest = rest->Cdr();
	    if(rest != 0)
		rest = rest->Cdr();
	}
	else{
	    array.Append(q);
	    if(p != q)
		changed = true;

	    rest = rest->Cdr();
	    if(rest != 0){
		array.Append(rest->Car());
		rest = rest->Cdr();
	    }
	}
    }

    if(changed)
	return array.All();
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
	    return new PTree::Declarator(decl, decl->Car(),
					 PTree::Node::Subst(args2, args,
							    decl->Cdr()));
    }
    else
	return decl;
}

bool Walker::GetArgDeclList(PTree::Declarator* decl, PTree::Node *& args)
{
    PTree::Node *p = decl;
    while(p != 0){
	PTree::Node *q = p->Car();
	if(q != 0)
	    if(q->IsLeaf()){
		if(q->Eq('(')){
		    args = p->Cadr();
		    return true;
		}
	    }
	    else if(q->Car()->Eq('('))	// e.g. int (*p)[];
		p = q->Cadr();

	p = p->Cdr();
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
	a = a2 = args->Car();
	if(args->Cdr() == 0)
	    rest = rest2 = 0;
	else{
	    rest = args->Cddr();	// skip ","
	    rest2 = TranslateArgDeclList2(record, e, translate, fill_args,
					  arg_name + 1, rest);
	    if(rest == rest2)
		rest = rest2 = args->Cdr();
	    else
		rest2 = PTree::Node::Cons(args->Cadr(), rest2);
	}

	bool is_ellipsis = a->IsLeaf();		// a may be "..."
	if(is_ellipsis)
	    /* do nothing */;
	else if(a->Car()->IsA(Token::ntUserdefKeyword)){
	    if(record)
		e->RecordDeclarator(a->Third());

	    if(translate){
		a2 = a->Cdr();
		if(fill_args)
		    a2 = FillArgumentName(a2, a2->Second(), arg_name);
	    }
	}
	else if(a->Car()->IsA(Token::REGISTER)){
	    if(record)
		e->RecordDeclarator(a->Third());

	    if(translate && fill_args){
		a2 = FillArgumentName(a, a->Third(), arg_name);
		if(a != a2)
		    a2 = PTree::Node::Cons(a->First(), a2);
	    }
	}
	else{
	    if(record)
		e->RecordDeclarator(a->Second());

	    if(translate && fill_args)
		a2 = FillArgumentName(a, a->Second(), arg_name);
	}

	if(a != a2 || rest != rest2)
	    return PTree::Node::Cons(a2, rest2);
	else
	    return args;
    }
}

PTree::Node *Walker::FillArgumentName(PTree::Node *arg, PTree::Node *d, int arg_name)
{
    PTree::Declarator* decl = (PTree::Declarator*)d;
    if(decl->Name() != 0)
	return arg;
    else{
	unsigned char* type = (unsigned char*)decl->GetEncodedType();
	return Encoding::MakePtree(type,
				   PTree::Node::Make(argument_name, arg_name));
    }
}

PTree::Node *Walker::TranslateAssignInitializer(PTree::Declarator*, PTree::Node *init)
{
    PTree::Node *exp = init->Second();
    PTree::Node *exp2 = Translate(exp);
    if(exp == exp2)
	return init;
    else
	return PTree::Node::List(init->Car(), exp2);
}

PTree::Node *Walker::TranslateInitializeArgs(PTree::Declarator*, PTree::Node *init)
{
    return TranslateArguments(init);
}

PTree::Node *Walker::TranslateFunctionImplementation(PTree::Node *impl)
{
    PTree::Node *sspec = PTree::Node::First(impl);
    PTree::Node *sspec2 = TranslateStorageSpecifiers(sspec);
    PTree::Node *tspec = PTree::Node::Second(impl);
    PTree::Node *decl = PTree::Node::Third(impl);
    PTree::Node *body = PTree::Node::Nth(impl, 3);
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
				      PTree::Node::List(tspec2, decl2, body2));
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
    PTree::Node *body = PTree::Node::Second(block);
    PTree::Node *rest = body;
    while(rest != 0){
	PTree::Node *p = rest->Car();
	PTree::Node *q = Translate(p);
	array.Append(q);
	if(p != q)
	    changed = true;

	rest = rest->Cdr();
    }

    if(changed)
	return new PTree::Brace(PTree::Node::First(block), array.All(),
				PTree::Node::Third(block));
    else
	return block;
}

PTree::Node *Walker::TranslateBlock(PTree::Node *block)
{
    PTree::Node *block2;

    NewScope();

    PTree::Array array;
    bool changed = false;
    PTree::Node *body = PTree::Node::Second(block);
    PTree::Node *rest = body;
    while(rest != 0){
	PTree::Node *p = rest->Car();
	PTree::Node *q = Translate(p);
	array.Append(q);
	if(p != q)
	    changed = true;

	rest = rest->Cdr();
    }

    if(changed)
	block2 = new PTree::Block(PTree::Node::First(block), array.All(),
				  PTree::Node::Third(block));
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
    PTree::Node *body = PTree::Node::Second(block);
    PTree::Node *rest = body;
    while(rest != 0){
	PTree::Node *p = rest->Car();
	PTree::Node *q = Translate(p);
	array.Append(q);
	if(p != q)
	    changed = true;

	rest = rest->Cdr();
    }

    if(changed)
	block2 = new PTree::ClassBody(PTree::Node::First(block), array.All(),
				      PTree::Node::Third(block));
    else
	block2 = block;

    ExitScope();
    return block2;
}

PTree::Node *Walker::TranslateClassSpec(PTree::Node *spec)
{
    PTree::Node *userkey;
    PTree::Node *class_def;

    if(spec->Car()->IsLeaf()){
	userkey = 0;
	class_def = spec;
    }
    else{
	userkey = spec->Car();
	class_def = spec->Cdr();
    }

    Class* metaobject = 0;
    if(PTree::Node::Length(class_def) == 4)
	metaobject = MakeClassMetaobject(spec, userkey, class_def);

    env->RecordClassName(spec->GetEncodedName(), metaobject);
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

void Walker::ChangeDefaultMetaclass(char* name)
{
    default_metaclass = name;
}

// LookupMetaclass() returns 0 if no metaclass is found.

Class* Walker::LookupMetaclass(PTree::Node *def, PTree::Node *userkey, PTree::Node *class_def,
			       bool is_template)
{
  PTree::Node *mclass, *margs;
    Class* metaobject;

    PTree::Node *class_name = class_def->Second();

    // for bootstrapping
    if(Metaclass::IsBuiltinMetaclass(class_name)){
	metaobject = new Metaclass;
	metaobject->InitializeInstance(def, 0);
	return metaobject;
    }

    PTree::Node *mdecl = env->LookupMetaclass(class_name);
    if(mdecl != 0){
	mclass = mdecl->Second();
	margs = mdecl->Nth(4);
	metaobject = opcxx_ListOfMetaclass::New(mclass, def, margs);
	if(metaobject == 0)
	    ErrorMessage("the metaclass is not loaded: ", mclass, class_def);
	else if(userkey != 0)
	    ErrorMessage("the metaclass declaration conflicts"
			 " with the keyword: ", mclass, class_def);

	return metaobject;
    }

    if(userkey != 0){
	mclass = env->LookupClasskeyword(userkey->Car());
	if(mclass == 0)
	    ErrorMessage("invalid keyword: ", userkey, class_def);
	else{
	    metaobject = opcxx_ListOfMetaclass::New(mclass, class_def,
						    userkey->Third());
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
    PTree::Node *bases = class_def->Third();
    while(bases != 0){
	bases = bases->Cdr();
	PTree::Node *base = bases->Car()->Last()->Car();
	bases = bases->Cdr();
	Class* m = env->LookupClassMetaobject(base);
	if(m != 0){
	    if(metaobject == 0)
		metaobject = m;
	    else if(m == 0 || strcmp(metaobject->MetaclassName(),
				       m->MetaclassName()) != 0){
		ErrorMessage("inherited metaclasses conflict: ",
			     class_def->Second(), class_def);
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
	PTree::Node *body = class_def->Nth(3);
	PTree::Node *body2 = TranslateClassBody(body, class_def->Third(),
					  metaobject);
	if(body == body2)
	    return spec;
	else
	    return new PTree::ClassSpec(spec->Car(),
					PTree::Node::ShallowSubst(body2, body,
								  spec->Cdr()),
					0, spec->GetEncodedName());
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
    PTree::Node *cond = s->Third();
    PTree::Node *cond2 = Translate(cond);
    PTree::Node *then_part = s->Nth(4);
    PTree::Node *then_part2 = Translate(then_part);
    PTree::Node *else_part = s->Nth(6);
    PTree::Node *else_part2 = Translate(else_part);

    if(cond == cond2 && then_part == then_part2 && else_part == else_part2)
	return s;
    else{
	PTree::Node *rest = PTree::Node::ShallowSubst(cond2, cond, then_part2, then_part,
					  else_part2, else_part, s->Cdr());
	return new PTree::IfStatement(s->Car(), rest);
    }
}

PTree::Node *Walker::TranslateSwitch(PTree::Node *s)
{
    PTree::Node *cond = s->Third();
    PTree::Node *cond2 = Translate(cond);
    PTree::Node *body = s->Nth(4);
    PTree::Node *body2 = Translate(body);
    if(cond == cond2 && body == body2)
	return s;
    else{
	PTree::Node *rest = PTree::Node::ShallowSubst(cond2, cond, body2, body, s->Cdr());
	return new PTree::SwitchStatement(s->Car(), rest);
    }
}

PTree::Node *Walker::TranslateWhile(PTree::Node *s)
{
    PTree::Node *cond = s->Third();
    PTree::Node *cond2 = Translate(cond);
    PTree::Node *body = s->Nth(4);
    PTree::Node *body2 = Translate(body);
    if(cond == cond2 && body == body2)
	return s;
    else{
	PTree::Node *rest = PTree::Node::ShallowSubst(cond2, cond, body2, body, s->Cdr());
	return new PTree::WhileStatement(s->Car(), rest);
    }
}

PTree::Node *Walker::TranslateDo(PTree::Node *s)
{
    PTree::Node *body = s->Second();
    PTree::Node *body2 = Translate(body);
    PTree::Node *cond = s->Nth(4);
    PTree::Node *cond2 = Translate(cond);
    if(cond == cond2 && body == body2)
	return s;
    else{
	PTree::Node *rest = PTree::Node::ShallowSubst(body2, body, cond2, cond, s->Cdr());
	return new PTree::DoStatement(s->Car(), rest);
    }
}

PTree::Node *Walker::TranslateFor(PTree::Node *s)
{
    NewScope();
    PTree::Node *exp1 = s->Third();
    PTree::Node *exp1t = Translate(exp1);
    PTree::Node *exp2 = s->Nth(3);
    PTree::Node *exp2t = Translate(exp2);
    PTree::Node *exp3 = s->Nth(5);
    PTree::Node *exp3t = Translate(exp3);
    PTree::Node *body = s->Nth(7);
    PTree::Node *body2 = Translate(body);
    ExitScope();

    if(exp1 == exp1t && exp2 == exp2t && exp3 == exp3t && body == body2)
	return s;
    else{
	PTree::Node *rest = PTree::Node::ShallowSubst(exp1t, exp1, exp2t, exp2,
					  exp3t, exp3, body2, body, s->Cdr());
	return new PTree::ForStatement(s->Car(), rest);
    }
}

PTree::Node *Walker::TranslateTry(PTree::Node *s)
{
    PTree::Node *try_block = s->Second();
    PTree::Node *try_block2 = Translate(try_block);

    PTree::Array array;
    PTree::Node *handlers = s->Cddr();
    bool changed = false;

    while(handlers != 0){
	PTree::Node *handle = handlers->Car();
	PTree::Node *body = handle->Nth(4);
	PTree::Node *body2 = Translate(body);
	if(body == body2)
	    array.Append(handle);
	else{
	    array.Append(PTree::Node::ShallowSubst(body2, body, handle));
	    changed = true;
	}

	handlers = handlers->Cdr();
    }

    if(try_block == try_block2 && !changed)
	return s;
    else
	return new PTree::TryStatement(s->Car(),
				       PTree::Node::Cons(try_block2, array.All()));
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
    if(s->Length() == 2)
	return s;
    else{
	PTree::Node *exp = s->Second();
	PTree::Node *exp2 = Translate(exp);
	if(exp == exp2)
	    return s;
	else
	    return new PTree::ReturnStatement(s->Car(),
					      PTree::Node::ShallowSubst(exp2, exp,
									s->Cdr()));
    }
}

PTree::Node *Walker::TranslateGoto(PTree::Node *s)
{
    return s;
}

PTree::Node *Walker::TranslateCase(PTree::Node *s)
{
    PTree::Node *st = s->Nth(3);
    PTree::Node *st2 = Translate(st);
    if(st == st2)
	return s;
    else
	return new PTree::CaseStatement(s->Car(),
					PTree::Node::ShallowSubst(st2, st, s->Cdr()));
}

PTree::Node *Walker::TranslateDefault(PTree::Node *s)
{
    PTree::Node *st = s->Third();
    PTree::Node *st2 = Translate(st);
    if(st == st2)
	return s;
    else
	return new PTree::DefaultStatement(s->Car(),
					   PTree::Node::List(s->Cadr(), st2));
}

PTree::Node *Walker::TranslateLabel(PTree::Node *s)
{
    PTree::Node *st = s->Third();
    PTree::Node *st2 = Translate(st);
    if(st == st2)
	return s;
    else
	return new PTree::LabelStatement(s->Car(),
					 PTree::Node::List(s->Cadr(), st2));
}

PTree::Node *Walker::TranslateExprStatement(PTree::Node *s)
{
    PTree::Node *exp = s->First();
    PTree::Node *exp2 = Translate(exp);
    if(exp == exp2)
	return s;
    else
      return new PTree::ExprStatement(exp2, s->Cdr());
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
	return PTree::Node::ShallowSubst(class_spec2, class_spec, tspec);
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
    if(PTree::Node::Eq(PTree::Node::Third(body), ';')){
	PTree::Node *spec = StripCvFromIntegralType(PTree::Node::Second(body));
	if(spec->IsA(Token::ntClassSpec))
	    return spec;
    }

    return 0;
}

PTree::Node *Walker::StripCvFromIntegralType(PTree::Node *integral)
{
    if(integral == 0)
	return 0;

    if(!integral->IsLeaf())
	if(integral->Car()->IsA(Token::CONST, Token::VOLATILE))
	    return PTree::Node::Second(integral);
	else if(PTree::Node::Second(integral)->IsA(Token::CONST, Token::VOLATILE))
	    return integral->Car();

    return integral;
}

PTree::Node *Walker::TranslateComma(PTree::Node *exp)
{
    PTree::Node *left = exp->First();
    PTree::Node *left2 = Translate(left);
    PTree::Node *right = exp->Third();
    PTree::Node *right2 = Translate(right);
    if(left == left2 && right == right2)
	return exp;
    else
	return new PTree::CommaExpr(left2, PTree::Node::List(exp->Second(), right2));
}

void Walker::TypeofComma(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Third(), t);
}

PTree::Node *Walker::TranslateAssign(PTree::Node *exp)
{
    PTree::Node *left = exp->First();
    PTree::Node *left2 = Translate(left);
    PTree::Node *right = exp->Third();
    PTree::Node *right2 = Translate(right);
    if(left == left2 && right == right2)
	return exp;
    else
	return new PTree::AssignExpr(left2, PTree::Node::List(exp->Second(), right2));
}

void Walker::TypeofAssign(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->First(), t);
}

PTree::Node *Walker::TranslateCond(PTree::Node *exp)
{
    PTree::Node *c = exp->First();
    PTree::Node *c2 = Translate(c);
    PTree::Node *t = exp->Third();
    PTree::Node *t2 = Translate(t);
    PTree::Node *e = exp->Nth(4);
    PTree::Node *e2 = Translate(e);
    if(c == c2 && t == t2 && e == e2)
	return exp;
    else
	return new PTree::CondExpr(c2, PTree::Node::List(exp->Second(), t2,
						 exp->Nth(3), e2));
}

void Walker::TypeofCond(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Third(), t);
}

PTree::Node *Walker::TranslateInfix(PTree::Node *exp)
{
    PTree::Node *left = exp->First();
    PTree::Node *left2 = Translate(left);
    PTree::Node *right = exp->Third();
    PTree::Node *right2 = Translate(right);
    if(left == left2 && right == right2)
	return exp;
    else
	return new PTree::InfixExpr(left2, PTree::Node::List(exp->Second(), right2));
}

void Walker::TypeofInfix(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->First(), t);
}

PTree::Node *Walker::TranslatePm(PTree::Node *exp)
{
    PTree::Node *left = exp->First();
    PTree::Node *left2 = Translate(left);
    PTree::Node *right = exp->Third();
    PTree::Node *right2 = Translate(right);
    if(left == left2 && right == right2)
	return exp;
    else
	return new PTree::PmExpr(left2, PTree::Node::List(exp->Second(), right2));
}

void Walker::TypeofPm(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Third(), t);
    t.Dereference();
}

PTree::Node *Walker::TranslateCast(PTree::Node *exp)
{
    PTree::Node *e = exp->Nth(3);
    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::CastExpr(exp->First(),
				   PTree::Node::ShallowSubst(e2, e, exp->Cdr()));
}

void Walker::TypeofCast(PTree::Node *exp, TypeInfo& t)
{
    t.Set(exp->Second()->Second()->GetEncodedType(), env);
}

PTree::Node *Walker::TranslateUnary(PTree::Node *exp)
{
    PTree::Node *oprnd = exp->Second();
    PTree::Node *oprnd2 = Translate(oprnd);
    if(oprnd == oprnd2)
	return exp;
    else
	return new PTree::UnaryExpr(exp->First(), PTree::Node::List(oprnd2));
}

void Walker::TypeofUnary(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Second(), t);

    PTree::Node *op = exp->First();
    if(op->Eq('*'))
	t.Dereference();
    else if(op->Eq('&'))
	t.Reference();
}

PTree::Node *Walker::TranslateThrow(PTree::Node *exp)
{
    PTree::Node *oprnd = exp->Second();
    PTree::Node *oprnd2 = Translate(oprnd);
    if(oprnd == oprnd2)
	return exp;
    else
	return new PTree::ThrowExpr(exp->First(), PTree::Node::List(oprnd2));
}

void Walker::TypeofThrow(PTree::Node *, TypeInfo& t)
{
    t.SetVoid();
}

PTree::Node *Walker::TranslateSizeof(PTree::Node *exp)
{
    PTree::Node *e = exp->Second();
    if(e->Eq('('))
	e = exp->Third();

    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::SizeofExpr(exp->First(),
				     PTree::Node::ShallowSubst(e2, e, exp->Cdr()));
}

void Walker::TypeofSizeof(PTree::Node *, TypeInfo& t)
{
    t.SetInt();
}

PTree::Node *Walker::TranslateTypeid(PTree::Node *exp)
{
    PTree::Node *e = exp->Second();
    if(e->Eq('('))
	e = exp->Third();

    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::TypeidExpr(exp->First(),
				     PTree::Node::ShallowSubst(e2, e, exp->Cdr()));
}

void Walker::TypeofTypeid(PTree::Node *exp, TypeInfo& t)
{
    t.SetInt(); // FIXME: Should be the type_info type (exp->Third()->Second()->GetEncodedType(), env);
}


PTree::Node *Walker::TranslateTypeof(PTree::Node *exp)
{
    PTree::Node *e = exp->Second();
    if(e->Eq('('))
	e = exp->Third();

    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::TypeofExpr(exp->First(),
				     PTree::Node::ShallowSubst(e2, e, exp->Cdr()));
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
    userkey = p->Car();
    if(userkey == 0 || !userkey->IsLeaf())
	p = exp->Cdr();		// user keyword
    else
	userkey = 0;

    if(p->Car()->Eq("::")){
	scope = p->Car();
	p = p->Cdr();
    }
    else
	scope = 0;

    op = p->Car();
    placement = p->Cadr();
    type = p->Third();
    init = p->Nth(3);
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
	return new PTree::NewExpr(exp->Car(),
				  PTree::Node::ShallowSubst(placement2, placement,
							    type2, type,
							    init2, init,
							    exp->Cdr()));
}

PTree::Node *Walker::TranslateNew3(PTree::Node *type)
{
    PTree::Node *p = type;
    if(p->Car()->Eq('('))
	p = p->Second();

    PTree::Node *decl = p->Second();
    PTree::Node *decl2 = TranslateNewDeclarator(decl);
    if(decl == decl2)
	return type;
    else
	return PTree::Node::Subst(decl2, decl, type);
}

void Walker::TypeofNew(PTree::Node *exp, TypeInfo& t)
{
  PTree::Node *p, *userkey, *type;

    p = exp;
    userkey = p->Car();
    if(userkey == 0 || !userkey->IsLeaf())
	p = exp->Cdr();		// user keyword

    if(p->Car()->Eq("::"))
	p = p->Cdr();

    type = p->Third();

    if(type->Car()->Eq('('))
	t.Set(type->Second()->Second()->GetEncodedType(), env);
    else
	t.Set(type->Second()->GetEncodedType(), env);

    t.Reference();
}

PTree::Node *Walker::TranslateDelete(PTree::Node *exp)
{
    PTree::Node *obj = PTree::Node::Last(exp)->Car();
    PTree::Node *obj2 = Translate(obj);
    if(obj == obj2)
	return exp;
    else
	return new PTree::DeleteExpr(exp->Car(),
				     PTree::Node::ShallowSubst(obj2, obj,
							       exp->Cdr()));
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
    PTree::Node *args = exp->Cdr();
    PTree::Node *args2 = TranslateArguments(args);
    if(args == args2)
	return exp;
    else
	return new PTree::FstyleCastExpr(exp->GetEncodedType(), exp->Car(),
					 args2);
}

void Walker::TypeofFstyleCast(PTree::Node *exp, TypeInfo& t)
{
    t.Set(exp->GetEncodedType(), env);
}

PTree::Node *Walker::TranslateArray(PTree::Node *exp)
{
    PTree::Node *array = exp->Car();
    PTree::Node *array2 = Translate(array);
    PTree::Node *index = exp->Third();
    PTree::Node *index2 = Translate(index);
    if(array == array2 && index == index2)
	return exp;
    else
	return new PTree::ArrayExpr(array2, PTree::Node::Subst(index2, index,
							       exp->Cdr()));
}

void Walker::TypeofArray(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Car(), t);
    t.Dereference();
}

/*
  TranslateFuncall() also deals with function-style casts to a class.
*/
PTree::Node *Walker::TranslateFuncall(PTree::Node *exp)
{
    PTree::Node *fun = exp->Car();
    PTree::Node *fun2 = Translate(fun);
    PTree::Node *args = exp->Cdr();
    PTree::Node *args2 = TranslateArguments(args);
    if(fun == fun2 && args == args2)
	return exp;
    else
	return new PTree::FuncallExpr(fun2, args2);
}

void Walker::TypeofFuncall(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Car(), t);
    if(!t.IsFunction())
	t.Dereference();	// maybe a pointer to a function

    t.Dereference();
}

PTree::Node *Walker::TranslatePostfix(PTree::Node *exp)
{
    PTree::Node *left = exp->Car();
    PTree::Node *left2 = Translate(left);
    if(left == left2)
	return exp;
    else
	return new PTree::PostfixExpr(left2, exp->Cdr());
}

void Walker::TypeofPostfix(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Car(), t);
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
    PTree::Node *left = exp->Car();
    PTree::Node *left2 = Translate(left);
    if(left == left2)
	return exp;
    else
	return new PTree::DotMemberExpr(left2, exp->Cdr());
}

void Walker::TypeofDotMember(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Car(), t);
    t.SetMember(exp->Third());
}

PTree::Node *Walker::TranslateArrowMember(PTree::Node *exp)
{
    PTree::Node *left = exp->Car();
    PTree::Node *left2 = Translate(left);
    if(left == left2)
	return exp;
    else
	return new PTree::ArrowMemberExpr(left2, exp->Cdr());
}

void Walker::TypeofArrowMember(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Car(), t);
    t.Dereference();
    t.SetMember(exp->Third());
}

PTree::Node *Walker::TranslateParen(PTree::Node *exp)
{
    PTree::Node *e = exp->Second();
    PTree::Node *e2 = Translate(e);
    if(e == e2)
	return exp;
    else
	return new PTree::ParenExpr(exp->Car(), PTree::Node::List(e2, exp->Third()));
}

void Walker::TypeofParen(PTree::Node *exp, TypeInfo& t)
{
    Typeof(exp->Second(), t);
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
	PTree::Node *head = p->Car();
	if(head == 0)
	    return decl;
	else if(head->Eq('[')){
	    PTree::Node *p2 = TranslateNewDeclarator2(p);
	    if(p == p2)
		return decl;
	    else{
		decl2 = PTree::Node::ShallowSubst(p2, p, decl);
		break;
	    }
	}
	else if(!head->IsLeaf() && head->Car()->Eq('(')){
	    PTree::Node *d = head->Cadr();
	    PTree::Node *d2 = TranslateNewDeclarator(d);
	    decl2 = PTree::Node::ShallowSubst(d2, d, decl);
	    break;
	}

	p = p->Cdr();
    }

    if(p == 0)
	return decl;
    else if(decl->IsA(Token::ntDeclarator))
	return new PTree::Declarator((PTree::Declarator*)decl,
				     decl2->Car(), decl2->Cdr());
    else
	return decl2;
}

PTree::Node *Walker::TranslateNewDeclarator2(PTree::Node *decl)
{
    for(PTree::Node *p = decl; p != 0; p = p->Cdr()){
	PTree::Node *head = p->Car();
	if(head->Eq('[')){
	    PTree::Node *size = p->Cadr();
	    PTree::Node *size2 = Translate(size);
	    if(size != size2){
		PTree::Node *q = TranslateNewDeclarator2(PTree::Node::ListTail(p, 3));
		return PTree::Node::Nconc(PTree::Node::List(p->Car(), size2, p->Third()),
				    q);
	    }
	}
	else if(head->Eq('('))
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
    PTree::Node *body = PTree::Node::Second(arglist);
    PTree::Node *args = body;
    while(args != 0){
	PTree::Node *p = args->Car();
	PTree::Node *q = Translate(p);
	array.Append(q);
	if(p != q)
	    changed = true;

	args = args->Cdr();
	if(args != 0){
	    array.Append(args->Car());
	    args = args->Cdr();
	}
    }

    if(changed)
	return PTree::Node::ShallowSubst(array.All(), body, arglist);
    else
	return arglist;
}

//. Helper function to recursively find the first left-most leaf node
PTree::Node *Walker::FindLeftLeaf(PTree::Node *node, PTree::Node *& parent)
{
    if (!node || node->IsLeaf()) return node;
    // Non-leaf node. So find first leafy child
    PTree::Node *leaf;
    while (node) {
	if (node->Car()) {
	    // There is a child here..
	    if (node->Car()->IsLeaf()) {
		// And this child is a leaf! return it and set parent
		parent = node;
		return node->Car();
	    }
	    if ((leaf = FindLeftLeaf(node->Car(), parent)))
		// Not a leaf so try recursing on it
		return leaf;
	}
	// No leaves from Car of this node, so try next Cdr
	node = node->Cdr();
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
        Token tk(leaf->GetPosition(), leaf->GetLength(), Token::Comment);
	cleaf = new (GC) PTree::CommentedAtom(tk, comments);
	parent->SetCar(cleaf);
    } else {
	// Already is a commented leaf, so add the comments to it
	comments = PTree::Node::Snoc(cleaf->GetComments(), comments);
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
    PTree::Node *decls = def->Third();
    if(decls == 0 || decls->IsLeaf())
	return 0;

    if(decls->IsA(Token::ntDeclarator)){	// if it is a function
	if(nth-- == 0)
	    return decls;
    }
    else
	while(decls != 0 && !decls->IsLeaf()){
	    if(nth-- == 0)
		return decls->Car();

	    if((decls = decls->Cdr()) != 0)
		decls = decls->Cdr();		// skip ,
	}

    return 0;
}

PTree::Node *Walker::FindDeclarator(PTree::Node *def, char* name, int len,
			      char* signature, int& nth, Environment* e)
{
    PTree::Node *decls = def->Third();
    if(decls == 0 || decls->IsLeaf())
	return 0;

    if(decls->IsA(Token::ntDeclarator)){	// if it is a function
	if(MatchedDeclarator(decls, name, len, signature, e))
	    return decls;

	++nth;
    }
    else
	while(decls != 0){
	    PTree::Node *d = decls->Car();
	    if(MatchedDeclarator(d, name, len, signature, e))
		return d;

	    ++nth;
	    if((decls = decls->Cdr()) != 0)
		decls = decls->Cdr();		// skip ,
	}

    return 0;
}

bool Walker::MatchedDeclarator(PTree::Node *decl, char* name, int len,
			       char* signature, Environment* e)
{
    char* str;
    int strlen;
    char* sig;

    str = decl->GetEncodedName();
    sig = decl->GetEncodedType();
    if(str == 0 || sig == 0)
	return false;

    str = Encoding::GetBaseName(str, strlen, e);
    return bool(len == strlen && memcmp(name, str, len) == 0
		&& strcmp(signature, sig) == 0);
}

bool Walker::WhichDeclarator(PTree::Node *def, PTree::Node *name, int& nth,
			     Environment* env)
{
    char* str;
    int len;
    Environment* e;
    PTree::Node *decls = def->Third();
    if(decls == 0 || decls->IsLeaf())
	return false;

    if(decls->IsA(Token::ntDeclarator)){	// if it is a function
	str = decls->GetEncodedName();
	e = env;
	str = Encoding::GetBaseName(str, len, e);
	if(name->Eq(str, len))
	    return true;

	++nth;
    }
    else
	while(decls != 0){
	    str = decls->Car()->GetEncodedName();
	    e = env;
	    str = Encoding::GetBaseName(str, len, e);
	    if(name->Eq(str, len))
		return true;

	    ++nth;
	    if((decls = decls->Cdr()) != 0)
		decls = decls->Cdr();
	}

    return false;
}

void Walker::ErrorMessage(char* msg, PTree::Node *name, PTree::Node *where)
{
    parser->ErrorMessage(msg, name, where);
}

void Walker::WarningMessage(char* msg, PTree::Node *name, PTree::Node *where)
{
    parser->WarningMessage(msg, name, where);
}

// InaccurateErrorMessage() may report a wrong line number.

void Walker::InaccurateErrorMessage(char* msg, PTree::Node *name, PTree::Node *where)
{
    if(default_parser == 0)
	MopErrorMessage("Walker::InaccurateErrorMessage()",
			"no default parser");
    else
	default_parser->ErrorMessage(msg, name, where);
}

void Walker::InaccurateWarningMessage(char* msg, PTree::Node *name, PTree::Node *where)
{
    if(default_parser == 0)
	MopErrorMessage("Walker::InaccurateWarningMessage()",
			"no default parser");
    else
	default_parser->WarningMessage(msg, name, where);
}
