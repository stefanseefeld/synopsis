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
#include "PTree.hh"
#include "ClassWalker.hh"
#include "ClassBodyWalker.hh"
#include "Class.hh"
#include "Environment.hh"
#include "TypeInfo.hh"
#include "Member.hh"
#include "Encoding.hh"

Class* ClassWalker::GetClassMetaobject(TypeInfo& tinfo)
{
    Class* c;
    if(tinfo.IsClass(c))
	return c;
    else if(tinfo.IsReferenceType()){
	tinfo.Dereference();
	if(tinfo.IsClass(c))
	    return c;
    }

    return 0;
}

bool ClassWalker::IsClassWalker()
{
    return true;
}

void ClassWalker::InsertBeforeStatement(PTree::Node *p)
{
  before_statement.append(p);
}

void ClassWalker::AppendAfterStatement(PTree::Node *p)
{
  after_statement.append(p);
}

void ClassWalker::InsertBeforeToplevel(PTree::Node *p)
{
  before_toplevel.append(p);
}

void ClassWalker::AppendAfterToplevel(PTree::Node *p)
{
  after_toplevel.append(p);
}

bool ClassWalker::InsertDeclaration(PTree::Node *d, Class* metaobject, PTree::Node *key,
				    void* data)
{
    inserted_declarations.append(d);
    if(metaobject == 0 || key == 0)
	return true;
    else if(LookupClientData(metaobject, key))
	return false;
    else{
	ClientDataLink* entry = new ClientDataLink;
	entry->next = client_data;
	entry->metaobject = metaobject;
	entry->key = key;
	entry->data = data;
	client_data = entry;
	return true;
    }
}

void* ClassWalker::LookupClientData(Class* metaobject, PTree::Node *key)
{
    for(ClientDataLink* c = client_data; c != 0; c = c->next)
	if(c->metaobject == metaobject && PTree::equal(key, c->key))
	    return c->data;

    return 0;
}

PTree::Node *ClassWalker::GetInsertedPtree()
{
    PTree::Node *result = 0;
    if(before_toplevel.number() > 0)
	result = PTree::nconc(result, before_toplevel.all());

    if(inserted_declarations.number() > 0)
	result = PTree::nconc(result, inserted_declarations.all());

    if(before_statement.number() > 0)
	result = PTree::nconc(result, before_statement.all());

    before_statement.clear();
    inserted_declarations.clear();
    client_data = 0;
    before_toplevel.clear();
    return result;
}

PTree::Node *ClassWalker::GetAppendedPtree()
{
  PTree::Node *result = 0;
  if(after_statement.number() > 0)
    result = PTree::nconc(result, after_statement.all());

  if(after_toplevel.number() > 0)
    result = PTree::nconc(result, after_toplevel.all());

  after_statement.clear();
  after_toplevel.clear();
  return result;
}

PTree::Node *ClassWalker::TranslateMetaclassDecl(PTree::Node *decl)
{
  env->RecordMetaclassName(decl);
  return 0;
}

PTree::Node *ClassWalker::TranslateClassSpec(PTree::Node *spec, PTree::Node *userkey,
					     PTree::Node *class_def, Class* metaobject)
{
  if(metaobject)
  {
    // the class body is given.
    PTree::Node *bases = PTree::third(class_def);
    PTree::Array* tspec_list = RecordMembers(class_def, bases, metaobject);
    metaobject->TranslateClass(env);
    metaobject->TranslateClassHasFinished();
    if(metaobject->removed) return 0;

    ClassBodyWalker w(this, tspec_list);
    PTree::Node *body = PTree::nth(class_def, 3);
    PTree::Node *body2 = w.TranslateClassBody(body, PTree::third(class_def),
					      metaobject);
    PTree::Node *bases2 = metaobject->GetBaseClasses();
    PTree::Node *cspec = metaobject->GetClassSpecifier();
    PTree::Node *name2 = metaobject->GetNewName();
    if(bases != bases2 || body != body2 || cspec != 0 || name2 != 0)
    {
      if(!name2) name2 = PTree::second(class_def);
      PTree::Node *rest = PTree::list(name2, bases2, body2);
      if(cspec) rest = PTree::cons(cspec, rest);
      return new PTree::ClassSpec(class_def->car(), rest, 0,
				  spec->encoded_name());
    }
  }
  if(!userkey) return spec;
  else return new PTree::ClassSpec(class_def->car(), class_def->cdr(),
				   0, spec->encoded_name());
}

PTree::Node *ClassWalker::TranslateTemplateInstantiation(PTree::Node *inst_spec,
			PTree::Node *userkey, PTree::Node *class_spec, Class* metaobject)
{
    PTree::Node *class_spec2;
    if (metaobject != 0 && metaobject->AcceptTemplate()) {
	TemplateClass* tmetaobj = (TemplateClass*)metaobject;
	class_spec2 = tmetaobj->TranslateInstantiation(env, class_spec);
	if (class_spec != class_spec2)
	    return class_spec2;
    }
    else
	class_spec2 = class_spec;

    if(userkey == 0)
	return inst_spec;
    else if (class_spec == class_spec2)
	return inst_spec;
    else
	return new PTree::TemplateInstantiation(class_spec);
}

PTree::Node *ClassWalker::ConstructClass(Class* metaobject)
{
    PTree::Node *def = metaobject->Definition();
    PTree::Node *def2;

    metaobject->TranslateClassHasFinished();
    ClassBodyWalker w(this, 0);
    PTree::Node *body = PTree::nth(def, 3);
    PTree::Node *body2 = w.TranslateClassBody(body, 0, metaobject);
    PTree::Node *bases2 = metaobject->GetBaseClasses();
    PTree::Node *cspec2 = metaobject->GetClassSpecifier();
    PTree::Node *name2 = metaobject->GetNewName();
    if(body == body2 && bases2 == 0 && cspec2 == 0 && name2 == 0)
	def2 = def;
    else{
	if(name2 == 0)
	  name2 = PTree::second(def);

	PTree::Node *rest = PTree::list(name2, bases2, body2);
	if(cspec2 != 0)
	    rest = PTree::cons(cspec2, rest);

	def2 = new PTree::ClassSpec(def->car(), rest,
				    0, def->encoded_name());
    }

    return new PTree::Declaration(0, PTree::list(def2, Class::semicolon_t));
}

PTree::Array* ClassWalker::RecordMembers(PTree::Node *class_def, PTree::Node *bases,
					 Class* metaobject)
{
  PTree::Node *tspec, *tspec2;

    NewScope(metaobject);
    RecordBaseclassEnv(bases);

    PTree::Array* tspec_list = new PTree::Array();

    PTree::Node *rest = PTree::second(PTree::nth(class_def, 3));
    while(rest != 0){
	PTree::Node *mem = rest->car();
	switch(mem->What()){
	case Token::ntTypedef :
	    tspec = PTree::second(mem);
	    tspec2 = TranslateTypespecifier(tspec);
	    env->RecordTypedefName(PTree::third(mem));
	    if(tspec != tspec2){
		tspec_list->append(tspec);
		tspec_list->append(tspec2);
	    }

	    break;
	case Token::ntMetaclassDecl :
	    env->RecordMetaclassName(mem);
	    break;
	case Token::ntDeclaration :
	    RecordMemberDeclaration(mem, tspec_list);
	    break;
	case Token::ntTemplateDecl :
	case Token::ntTemplateInstantiation :
	case Token::ntUsing :
	default :
	    break;
	}

	rest = rest->cdr();
    }

    if(tspec_list->number() == 0){
	delete tspec_list;
	tspec_list = 0;
    }
	
    ExitScope();
    return tspec_list;
}

//  RecordMemberDeclaration() is derived from TranslateDeclaration().

void ClassWalker::RecordMemberDeclaration(PTree::Node *mem,
					  PTree::Array* tspec_list)
{
  PTree::Node *tspec, *tspec2, *decls;

  tspec = PTree::second(mem);
    tspec2 = TranslateTypespecifier(tspec);
    decls = PTree::third(mem);
    if(decls->IsA(Token::ntDeclarator))	// if it is a function
	env->RecordDeclarator(decls);
    else if(!decls->is_atom())		// not a null declaration.
	while(decls != 0){
	    PTree::Node *d = decls->car();
	    if(d->IsA(Token::ntDeclarator))
		env->RecordDeclarator(d);

	    decls = decls->cdr();
	    if(decls != 0)
		decls = decls->cdr();
	}

    if(tspec != tspec2){
	tspec_list->append(tspec);
	tspec_list->append(tspec2);
    }
}

PTree::Node *ClassWalker::ConstructAccessSpecifier(int access)
{
    PTree::Node *lf;
    switch(access){
    case Class::Protected :
        lf = Class::protected_t;
	break;
    case Class::Private :
	lf = Class::private_t;
	break;
    case Class::Public :
    default :
	lf = Class::public_t;
	break;
     }

     return new PTree::AccessSpec(lf, PTree::list(Class::colon_t));
}

PTree::Node *ClassWalker::ConstructMember(void* ptr)
{
    ChangedMemberList::Cmem* m = (ChangedMemberList::Cmem*)ptr;
    PTree::Node *def = m->def;
    PTree::Node *def2;

    if(PTree::third(def)->IsA(Token::ntDeclarator)){
	// function implementation
	if(m->body == 0){
	    NameScope old_env;
	    Environment* fenv = env->DontRecordDeclarator(m->declarator);
	    if(fenv != 0)
		old_env = ChangeScope(fenv);

	    NewScope();
	    def2 = MakeMemberDeclarator(true, m,
					(PTree::Declarator*)m->declarator);
	    def2 = PTree::list(def2,
				     TranslateFunctionBody(PTree::nth(def, 3)));
	    ExitScope();
	    if(fenv != 0)
		RestoreScope(old_env);
	}
	else{
	    def2 = MakeMemberDeclarator(false, m,
					(PTree::Declarator*)m->declarator);
	    def2 = PTree::list(def2, m->body);
	}
    }
    else{
	// declaration
	def2 = MakeMemberDeclarator(false, m,
				    (PTree::Declarator*)m->declarator);
	if(m->body == 0)
	    def2 = PTree::list(PTree::list(def2), Class::semicolon_t);
	else
	    def2 = PTree::list(def2, m->body);
    }

    def2 = new PTree::Declaration(TranslateStorageSpecifiers(PTree::first(def)),
				  PTree::cons(TranslateTypespecifier(PTree::second(def)),
						    def2));
    return def2;
}

PTree::Node *ClassWalker::TranslateStorageSpecifiers(PTree::Node *spec)
{
    return TranslateStorageSpecifiers2(spec);
}

PTree::Node *ClassWalker::TranslateStorageSpecifiers2(PTree::Node *rest)
{
    if(rest == 0)
	return 0;
    else{
	PTree::Node *h = rest->car();
	PTree::Node *t = rest->cdr();
	PTree::Node *t2 = TranslateStorageSpecifiers2(t);
	if(h->IsA(Token::ntUserdefKeyword))
	    return t2;
	else if(t == t2)
	    return rest;
	else
	    return PTree::cons(h, t2);
    }
}

PTree::Node *ClassWalker::TranslateTemplateFunction(PTree::Node *temp_def, PTree::Node *impl)
{
    Environment* fenv = env->RecordTemplateFunction(temp_def, impl);
    if (fenv != 0) {
	Class* metaobject = fenv->IsClassEnvironment();
	if(metaobject != 0){
	    NameScope old_env = ChangeScope(fenv);
	    NewScope();

	    ChangedMemberList::Cmem m;
	    PTree::Node *decl = PTree::third(impl);
	    MemberFunction mem(metaobject, impl, decl);
	    metaobject->TranslateMemberFunction(env, mem);
	    ChangedMemberList::Copy(&mem, &m, Class::Undefined);
	    PTree::Node *decl2
		= MakeMemberDeclarator(true, &m, (PTree::Declarator*)decl);

	    ExitScope();
	    RestoreScope(old_env);
	    if(decl != decl2) {
	      PTree::Node *pt = PTree::list(PTree::second(impl), decl2, PTree::nth(impl, 3));
	      pt = new PTree::Declaration(PTree::first(impl), pt);
	      pt = PTree::list(PTree::second(temp_def), PTree::third(temp_def),
				     PTree::nth(temp_def, 3), pt);
	      return new PTree::TemplateDecl(PTree::first(temp_def), pt);
	    }
	}
    }

    return temp_def;
}

PTree::Node *ClassWalker::TranslateFunctionImplementation(PTree::Node *impl)
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
	body2 = TranslateFunctionBody(body);
	ExitScope();
    }
    else{
	Class* metaobject = fenv->IsClassEnvironment();
	NameScope old_env = ChangeScope(fenv);
	NewScope();

	if (metaobject == 0 && Class::metaclass_for_c_functions != 0)
	    metaobject = MakeMetaobjectForCfunctions();

	if(metaobject == 0){
	    decl2 = TranslateDeclarator(true, (PTree::Declarator*)decl);
	    body2 = TranslateFunctionBody(body);
	}
	else{
	    ChangedMemberList::Cmem m;
	    MemberFunction mem(metaobject, impl, decl);
	    metaobject->TranslateMemberFunction(env, mem);
	    ChangedMemberList::Copy(&mem, &m, Class::Undefined);
	    decl2 = MakeMemberDeclarator(true, &m, (PTree::Declarator*)decl);
	    if(m.body != 0)
		body2 = m.body;
	    else
		body2 = TranslateFunctionBody(body);
	}

	ExitScope();
	RestoreScope(old_env);
    }

    if(sspec == sspec2 && tspec == tspec2 && decl == decl2 && body == body2)
	return impl;
    else
	return new PTree::Declaration(sspec2,
				      PTree::list(tspec2, decl2, body2));
}

Class* ClassWalker::MakeMetaobjectForCfunctions() {
    if (Class::for_c_functions == 0) {
	Encoding encode;
	PTree::Node *name = new PTree::Atom("<C>", 3);
	encode.SimpleName(name);
	PTree::Node *class_def
	    = new PTree::ClassSpec(Class::class_t,
				   PTree::list(name, 0,
						     Class::empty_block_t),
				   0, encode.Get());
	std::cerr << "encode: " << class_def->encoded_name();
	Class* metaobject = opcxx_ListOfMetaclass::New(
			Class::metaclass_for_c_functions,
			class_def, 0);
	if(metaobject == 0)
	    MopErrorMessage2(
		"the metaclass for C functions cannot be loaded: ",
		Class::metaclass_for_c_functions);

	metaobject->SetEnvironment(env);
	Class::for_c_functions = metaobject;
    }

    return Class::for_c_functions;
}

PTree::Node *ClassWalker::MakeMemberDeclarator(bool record, void* ptr,
					       PTree::Declarator* decl)
{
  PTree::Node *args, *args2, *name, *name2, *init, *init2;

    //  Since g++ cannot parse the nested-class declaration:
    //     class ChangedMemberList::Cmem;
    //  MakeMemberDeclarator() takes a void* pointer and convert the type
    //  into ChangedMemberList::Cmem*.
    ChangedMemberList::Cmem* m = (ChangedMemberList::Cmem*)ptr;

    if(m->removed)
	return 0;

    if(GetArgDeclList(decl, args))
	if(m->args == 0)
	    args2 = TranslateArgDeclList2(record, env, true,
					  m->arg_name_filled, 0, args);
	else{
	    args2 = m->args;
	    // we may need to record the arguments.
	    TranslateArgDeclList2(record, env, false, false, 0, args);
	}
    else
	args = args2 = 0;

    name = decl->Name();
    if(m->name != 0)
	name2 = m->name;
    else
	name2 = name;

    if(m->init == 0)
	init = init2 = 0;
    else{
	init2 = m->init;
	init = PTree::last(decl)->car();
	if(init->is_atom() || *init->car() != ':')
	    init = 0;
    }

    if(args == args2 && name == name2 && init == init2)
	return decl;
    else{
	PTree::Node *rest;
	if(init == 0 && init2 != 0){
	    rest = PTree::subst(args2, args, name2, name, decl->cdr());
	    rest = PTree::append(rest, init2);
	}
	else
	    rest = PTree::subst(args2, args, name2, name,
				init2, init, decl->cdr());

	if(decl->car() == name)
	    return new PTree::Declarator(decl, name2, rest);
	else
	    return new PTree::Declarator(decl, decl->car(), rest);
    }
}

PTree::Node *ClassWalker::RecordArgsAndTranslateFbody(Class* c, PTree::Node *args,
						      PTree::Node *body)
{
    NameScope old_env;
    Environment* fenv = c->GetEnvironment();

    if(fenv != 0)
	old_env = ChangeScope(fenv);

    NewScope();
    TranslateArgDeclList2(true, env, false, false, 0, args);
    PTree::Node *body2 = TranslateFunctionBody(body);
    ExitScope();

    if(fenv != 0)
	RestoreScope(old_env);

    return body2;
}

PTree::Node *ClassWalker::TranslateFunctionBody(PTree::Node *body)
{
    PTree::Node *body2;

    inserted_declarations.clear();
    client_data = 0;
    body = Translate(body);
    if(body == 0 || body->is_atom() || inserted_declarations.number() <= 0)
	body2 = body;
    else{
	PTree::Node *decls = inserted_declarations.all();
	body2 = new PTree::Block(PTree::first(body),
				 PTree::nconc(decls, PTree::second(body)),
				 PTree::third(body));
    }

    inserted_declarations.clear();
    client_data = 0;
    return body2;
}

PTree::Node *ClassWalker::TranslateBlock(PTree::Node *block)
{
    PTree::Node *block2;

    NewScope();

    PTree::Array array;
    bool changed = false;
    PTree::Node *body = PTree::second(block);
    PTree::Node *rest = body;
    while(rest != 0){
	uint i, n;
	PTree::Node *p = rest->car();
	PTree::Node *q = Translate(p);

	n = before_statement.number();
	if(n > 0){
	    changed = true;
	    for(i = 0; i < n; ++i)
		array.append(before_statement[i]);
	}

	array.append(q);
	if(p != q)
	    changed = true;

	n = after_statement.number();
	if(n > 0){
	    changed = true;
	    for(i = 0; i < n; ++i)
		array.append(after_statement[i]);
	}

	before_statement.clear();
	after_statement.clear();
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

PTree::Node *ClassWalker::TranslateArgDeclList(bool record, PTree::Node *, PTree::Node *args)
{
    return TranslateArgDeclList2(record, env, true, false, 0, args);
}

PTree::Node *ClassWalker::TranslateInitializeArgs(PTree::Declarator* decl,
						  PTree::Node *init)
{
    TypeInfo tinfo;
    env->Lookup(decl, tinfo);
    Class* metaobject = tinfo.ClassMetaobject();
    if(metaobject != 0)
	return metaobject->TranslateInitializer(env, decl->Name(), init);
    else
	return TranslateArguments(init);
}

PTree::Node *ClassWalker::TranslateAssignInitializer(PTree::Declarator* decl,
						     PTree::Node *init)
{
    TypeInfo tinfo;
    env->Lookup(decl, tinfo);
    Class* metaobject = tinfo.ClassMetaobject();
    if(metaobject != 0)
	return metaobject->TranslateInitializer(env, decl->Name(), init);
    else{
	PTree::Node *exp = PTree::second(init);
	PTree::Node *exp2 = Translate(exp);
	if(exp == exp2)
	    return init;
	else
	    return PTree::list(init->car(), exp2);
    }
}

PTree::Node *ClassWalker::TranslateUserAccessSpec(PTree::Node *)
{
    return 0;
}

PTree::Node *ClassWalker::TranslateAssign(PTree::Node *exp)
{
  PTree::Node *left, *left2, *right, *right2, *exp2;
    Environment* scope;
    Class* metaobject;
    TypeInfo type;

    left = PTree::first(exp);
    right = PTree::third(exp);
    if(left->IsA(Token::ntDotMemberExpr, Token::ntArrowMemberExpr)){
	PTree::Node *object = PTree::first(left);
	PTree::Node *op = PTree::second(left);
	PTree::Node *member = PTree::third(left);
	PTree::Node *assign_op = PTree::second(exp);
	Typeof(object, type);
	if(*op != '.')
	    type.Dereference();

	metaobject = GetClassMetaobject(type);
	if(metaobject != 0){
	    exp2 = metaobject->TranslateMemberWrite(env, object, op,
						    member, assign_op, right);
	    return CheckMemberEquiv(exp, exp2);
	}
    }
    else if((scope = env->IsMember(left)) != 0){
	metaobject = scope->IsClassEnvironment();
	if(metaobject != 0){
	    exp2 = metaobject->TranslateMemberWrite(env, left, PTree::second(exp),
						    right);
	    return CheckEquiv(exp, exp2);
	}
    }
    else{
	Typeof(left, type);
	metaobject = GetClassMetaobject(type);
	if(metaobject != 0){
	    exp2 = metaobject->TranslateAssign(env, left, PTree::second(exp),
					       right);
	    return CheckEquiv(exp, exp2);	
	}
    }

    left2 = Translate(left);
    right2 = Translate(right);
    if(left == left2 && right == right2)
	return exp;
    else
	return new PTree::AssignExpr(left2, PTree::list(PTree::second(exp), right2));
}

PTree::Node *ClassWalker::CheckMemberEquiv(PTree::Node *exp, PTree::Node *exp2)
{
    if(!exp2->is_atom()
       && PTree::equiv(exp->car(), exp2->car())
       && PTree::equiv(exp->cdr(), exp2->cdr()))
	return exp;
    else
	return exp2;
}

PTree::Node *ClassWalker::TranslateInfix(PTree::Node *exp)
{
    TypeInfo type;

    PTree::Node *left = PTree::first(exp);
    PTree::Node *right = PTree::third(exp);
    Typeof(right, type);
    Class* metaobject = GetClassMetaobject(type);
    if(metaobject == 0){
	Typeof(left, type);
	metaobject = GetClassMetaobject(type);
    }

    if(metaobject != 0){
	PTree::Node *exp2 = metaobject->TranslateBinary(env, left, PTree::second(exp),
						  right);
	return CheckEquiv(exp, exp2);
    }
    else{
	PTree::Node *left2 = Translate(left);
	PTree::Node *right2 = Translate(right);
	if(left == left2 && right == right2)
	    return exp;
	else
	    return new PTree::InfixExpr(left2, PTree::list(PTree::second(exp),
								 right2));
    }
}

PTree::Node *ClassWalker::TranslateUnary(PTree::Node *exp)
{
    TypeInfo type;
    Environment* scope;
    Class* metaobject;
    PTree::Node *exp2;

    PTree::Node *unaryop = exp->car();
    PTree::Node *right = PTree::second(exp);
    if(right->IsA(Token::ntDotMemberExpr, Token::ntArrowMemberExpr)){
	PTree::Node *object = PTree::first(right);
	PTree::Node *op = PTree::second(right);
	Typeof(object, type);
	if(*op != '.')
	    type.Dereference();

	metaobject = GetClassMetaobject(type);
	if(metaobject != 0){
	    exp2 = metaobject->TranslateUnaryOnMember(env, unaryop, object,
						      op, PTree::third(right));
	    if(PTree::length(exp2) == 2 && exp2->car() == unaryop
	       && PTree::equiv(PTree::second(exp2), right))
		return exp;
	    else
		return exp2;
	}
    }    
    else if((scope = env->IsMember(right)) != 0){
	metaobject = scope->IsClassEnvironment();
	if(metaobject != 0){
	    exp2 = metaobject->TranslateUnaryOnMember(env, unaryop, right);
	    return CheckEquiv(exp, exp2);
	}
    }

    Typeof(right, type);
    metaobject = GetClassMetaobject(type);
    if(metaobject != 0){
	PTree::Node *exp2 = metaobject->TranslateUnary(env, unaryop, right);
	return CheckEquiv(exp, exp2);
    }
    else{
	PTree::Node *right2 = Translate(right);
	if(right == right2)
	    return exp;
	else
	    return new PTree::UnaryExpr(unaryop, PTree::list(right2));
    }
}

PTree::Node *ClassWalker::TranslateArray(PTree::Node *exp)
{
    TypeInfo type;

    PTree::Node *array = exp->car();
    Typeof(array, type);
    Class* metaobject = GetClassMetaobject(type);
    if(metaobject != 0){
	PTree::Node *exp2 = metaobject->TranslateSubscript(env, array, exp->cdr());
	return CheckEquiv(exp, exp2);
    }
    else{
	PTree::Node *index = PTree::third(exp);
	PTree::Node *array2 = Translate(array);
	PTree::Node *index2 = Translate(index);
	if(array == array2 && index == index2)
	    return exp;
	else
	    return new PTree::ArrayExpr(array2,
					PTree::shallow_subst(index2, index, exp->cdr()));
    }
}

PTree::Node *ClassWalker::TranslatePostfix(PTree::Node *exp)
{
    TypeInfo type;
    Environment* scope;
    Class* metaobject;
    PTree::Node *exp2;

    PTree::Node *left = exp->car();
    PTree::Node *postop = PTree::second(exp);
    if(left->IsA(Token::ntDotMemberExpr, Token::ntArrowMemberExpr)){
	PTree::Node *object = PTree::first(left);
	PTree::Node *op = PTree::second(left);
	Typeof(object, type);
	if(*op != '.')
	    type.Dereference();

	metaobject = GetClassMetaobject(type);
	if(metaobject != 0){
	    exp2 = metaobject->TranslatePostfixOnMember(env, object, op,
							PTree::third(left), postop);
	    return CheckMemberEquiv(exp, exp2);
	}
    }    
    else if((scope = env->IsMember(left)) != 0){
	metaobject = scope->IsClassEnvironment();
	if(metaobject != 0){
	    exp2 = metaobject->TranslatePostfixOnMember(env, left, postop);
	    return CheckEquiv(exp, exp2);
	}
    }

    Typeof(left, type);
    metaobject = GetClassMetaobject(type);
    if(metaobject != 0){
	exp2 = metaobject->TranslatePostfix(env, left, postop);
	return CheckEquiv(exp, exp2);
    }
    else{
	PTree::Node *left2 = Translate(left);
	if(left == left2)
	    return exp;
	else
	    return new PTree::PostfixExpr(left2, exp->cdr());
    }
}

PTree::Node *ClassWalker::TranslateFuncall(PTree::Node *exp)
{
    TypeInfo type;
    Environment* scope;
    Class* metaobject;
    PTree::Node *fun, *arglist, *exp2;

    fun = exp->car();
    arglist = exp->cdr();
    if(fun->IsA(Token::ntDotMemberExpr, Token::ntArrowMemberExpr)){
	PTree::Node *object = PTree::first(fun);
	PTree::Node *op = PTree::second(fun);
	PTree::Node *member = PTree::third(fun);
	Typeof(object, type);
	if(*op != '.')
	    type.Dereference();

	metaobject = GetClassMetaobject(type);
	if(metaobject != 0){
	    exp2 = metaobject->TranslateMemberCall(env, object, op,
						   member, arglist);
	    return CheckMemberEquiv(exp, exp2);
	}
    }
    else if((scope = env->IsMember(fun)) != 0){
	metaobject = scope->IsClassEnvironment();
	if(metaobject != 0){
	    exp2 = metaobject->TranslateMemberCall(env, fun, arglist);
	    return CheckEquiv(exp, exp2);
	}
    }
    else{
	Typeof(fun, type);
	metaobject = GetClassMetaobject(type);
	if(metaobject != 0){
	    exp2 = metaobject->TranslateFunctionCall(env, fun, arglist);
	    return CheckEquiv(exp, exp2);
	}
    }

    PTree::Node *fun2 = Translate(fun);
    PTree::Node *arglist2 = TranslateArguments(arglist);
    if(fun == fun2 && arglist == arglist2)
	return exp;
    else
	return new PTree::FuncallExpr(fun2, arglist2);
}

PTree::Node *ClassWalker::TranslateDotMember(PTree::Node *exp)
{
    TypeInfo type;

    PTree::Node *left = exp->car();
    Typeof(left, type);
    Class* metaobject = GetClassMetaobject(type);
    if(metaobject != 0){
	PTree::Node *exp2 = metaobject->TranslateMemberRead(env, left, PTree::second(exp),
							    PTree::third(exp));
	return CheckEquiv(exp, exp2);
    }
    else{
	PTree::Node *left2 = Translate(left);
	if(left == left2)
	    return exp;
	else
	    return new PTree::DotMemberExpr(left2, exp->cdr());
    }
}

PTree::Node *ClassWalker::TranslateArrowMember(PTree::Node *exp)
{
    TypeInfo type;

    PTree::Node *left = exp->car();
    Typeof(left, type);
    type.Dereference();
    Class* metaobject = GetClassMetaobject(type);
    if(metaobject != 0){
	PTree::Node *exp2 = metaobject->TranslateMemberRead(env, left, PTree::second(exp),
							    PTree::third(exp));
	return CheckEquiv(exp, exp2);
    }
    else{
	PTree::Node *left2 = Translate(left);
	if(left == left2)
	    return exp;
	else
	    return new PTree::ArrowMemberExpr(left2, exp->cdr());
    }
}

PTree::Node *ClassWalker::TranslateThis(PTree::Node *exp)
{
    TypeInfo type;
    Typeof(exp, type);
    type.Dereference();
    Class* metaobject = GetClassMetaobject(type);
    if(metaobject != 0)
	return metaobject->TranslatePointer(env, exp);
    else
	return exp;
}

PTree::Node *ClassWalker::TranslateVariable(PTree::Node *exp)
{
    Environment* scope = env->IsMember(exp);
    if(scope != 0){
	Class* metaobject = scope->IsClassEnvironment();
	if(metaobject != 0)
	    return metaobject->TranslateMemberRead(env, exp);
    }

    TypeInfo type;
    Typeof(exp, type);
    if(type.IsPointerType()){
	type.Dereference();
	Class* metaobject = GetClassMetaobject(type);
	if(metaobject != 0)
	    return metaobject->TranslatePointer(env, exp);
    }

    return exp;
}

PTree::Node *ClassWalker::TranslateUserStatement(PTree::Node *exp)
{
    TypeInfo type;
    PTree::Node *object = PTree::first(exp);
    PTree::Node *op = PTree::second(exp);
    PTree::Node *keyword = PTree::third(exp);
    PTree::Node *rest = PTree::tail(exp, 3);

    Typeof(object, type);
    if(*op != '.')
	type.Dereference();

    Class* metaobject = GetClassMetaobject(type);
    if(metaobject != 0){
	NewScope();
	if(keyword->IsA(Token::UserKeyword2))		// closure style
	    TranslateArgDeclList2(true, env, false, false, 0, PTree::second(rest));

	PTree::Node *exp2 = metaobject->TranslateUserStatement(env, object, op,
							 keyword, rest);
	ExitScope();
	return exp2;
    }

    ErrorMessage("no complete class specification for: ", object, exp);
    return 0;
}

PTree::Node *ClassWalker::TranslateStaticUserStatement(PTree::Node *exp)
{
    bool is_type_name;
    TypeInfo type;
    PTree::Node *qualifier = PTree::first(exp);
    PTree::Node *keyword = PTree::third(exp);
    PTree::Node *rest = PTree::tail(exp, 3);

    if(env->Lookup(qualifier, is_type_name, type))
	if(is_type_name){
	    Class* metaobject = GetClassMetaobject(type);
	    if(metaobject != 0){
		NewScope();
		if(keyword->IsA(Token::UserKeyword2))		// closure style
		    TranslateArgDeclList2(true, env, false, false, 0,
					  PTree::second(rest));
		PTree::Node *exp2 = metaobject->TranslateStaticUserStatement(env,
							keyword, rest);
		ExitScope();
		return exp2;
	    }
	}

    ErrorMessage("no complete class specification for: ", qualifier, exp);
    return 0;
}

PTree::Node *ClassWalker::TranslateNew2(PTree::Node *exp, PTree::Node *userkey, PTree::Node *scope,
				  PTree::Node *op, PTree::Node *placement,
				  PTree::Node *type, PTree::Node *init)
{
    TypeInfo t;

    if(type->car() && *type->car() == '(')
	t.Set(PTree::second(PTree::second(type))->encoded_type(), env);
    else
	t.Set(PTree::second(type)->encoded_type(), env);

    Class* metaobject = GetClassMetaobject(t);
    if(metaobject != 0){
	if(userkey == 0)
	    userkey = scope;

	PTree::Node *exp2 = metaobject->TranslateNew(env, userkey, op,
					       placement, type, init);
	return CheckEquiv(exp, exp2);
    }
    else{
	PTree::Node *placement2 = TranslateArguments(placement);
	PTree::Node *type2 = TranslateNew3(type);
	PTree::Node *init2 = TranslateArguments(init);
	if(userkey == 0 && placement == placement2
	   && type == type2 && init == init2)
	    return exp;
	else{
	    if(userkey == 0)
		return new PTree::NewExpr(exp->car(),
				PTree::shallow_subst(placement2, placement,
						     type2, type,
						     init2, init, exp->cdr()));
	    else{
		ErrorMessage("no complete class specification for: ",
			     type, exp);
		exp = exp->cdr();
		if(placement == placement2 && type == type2 && init == init2)
		    return exp;
		else
		    return new PTree::NewExpr(exp->car(),
				PTree::shallow_subst(placement2, placement,
						     type2, type,
						     init2, init, exp->cdr()));
	    }

	}
    }
}

PTree::Node *ClassWalker::TranslateDelete(PTree::Node *exp)
{
    TypeInfo type;

    PTree::Node *obj = PTree::last(exp)->car();
    if(PTree::length(exp) == 2){	// not ::delete or delete []
	Typeof(obj, type);
	type.Dereference();
	Class* metaobject = GetClassMetaobject(type);
	if(metaobject != 0){
	    PTree::Node *exp2 = metaobject->TranslateDelete(env, exp->car(), obj);
	    return CheckEquiv(exp, exp2);
	}
    }

    PTree::Node *obj2 = Translate(obj);
    if(obj == obj2)
	return exp;
    else
	return new PTree::DeleteExpr(exp->car(),
				   PTree::shallow_subst(obj2, obj,
							exp->cdr()));
}
