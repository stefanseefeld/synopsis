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

#include <Synopsis/PTree.hh>
#include <TypeInfoVisitor.hh>
#include "ClassWalker.hh"
#include "ClassBodyWalker.hh"
#include "Class.hh"
#include "Environment.hh"
#include "TypeInfo.hh"
#include "Member.hh"
#include <iostream>

using namespace Synopsis;

Class *ClassWalker::get_class_metaobject(TypeInfo& tinfo)
{
  Class* c;
  if(tinfo.is_class(c))
    return c;
  else if(tinfo.is_reference_type()){
    tinfo.dereference();
    if(tinfo.is_class(c))
      return c;
  }
  return 0;
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

PTree::Node *ClassWalker::translate_metaclass_decl(PTree::Node *decl)
{
  my_environment->RecordMetaclassName(decl);
  return 0;
}

PTree::Node *ClassWalker::translate_class_spec(PTree::Node *spec, PTree::Node *userkey,
					       PTree::Node *class_def, Class* metaobject)
{
  if(metaobject)
  {
    // the class body is given.
    PTree::Node *bases = PTree::third(class_def);
    PTree::Array* tspec_list = RecordMembers(class_def, bases, metaobject);
    metaobject->TranslateClass(my_environment);
    metaobject->TranslateClassHasFinished();
    if(metaobject->removed) return 0;

    ClassBodyWalker w(this, tspec_list);
    PTree::ClassBody *body = static_cast<PTree::ClassBody *>(PTree::nth(class_def, 3));
    PTree::ClassBody *body2 = w.translate_class_body(body, PTree::third(class_def),
						     metaobject);
    PTree::Node *bases2 = metaobject->GetBaseClasses();
    PTree::Node *cspec = metaobject->GetClassSpecifier();
    PTree::Node *name2 = metaobject->GetNewName();
    if(bases != bases2 || body != body2 || cspec != 0 || name2 != 0)
    {
      if(!name2) name2 = PTree::second(class_def);
      PTree::Node *rest = PTree::list(name2, bases2, body2);
      if(cspec) rest = PTree::cons(cspec, rest);
      return new PTree::ClassSpec(spec->encoded_name(), class_def->car(), rest, 0);
    }
  }
  if(!userkey) return spec;
  else return new PTree::ClassSpec(spec->encoded_name(),
				   class_def->car(), class_def->cdr(), 0);
}

PTree::Node *ClassWalker::translate_template_instantiation(PTree::TemplateInstantiation *inst_spec,
							   PTree::Node *userkey,
							   PTree::ClassSpec *class_spec,
							   Class *metaobject)
{
  PTree::Node *class_spec2;
  if (metaobject && metaobject->AcceptTemplate())
  {
    TemplateClass* tmetaobj = (TemplateClass*)metaobject;
    class_spec2 = tmetaobj->TranslateInstantiation(my_environment, class_spec);
    if (class_spec != class_spec2) return class_spec2;
  }
  else class_spec2 = class_spec;

  if(userkey == 0) return inst_spec;
  else if (class_spec == class_spec2) return inst_spec;
  else return new PTree::TemplateInstantiation(class_spec);
}

PTree::Node *ClassWalker::ConstructClass(Class* metaobject)
{
    PTree::Node *def = metaobject->Definition();
    PTree::Node *def2;

    metaobject->TranslateClassHasFinished();
    ClassBodyWalker w(this, 0);
    PTree::ClassBody *body = static_cast<PTree::ClassBody *>(PTree::nth(def, 3));
    PTree::ClassBody *body2 = w.translate_class_body(body, 0, metaobject);
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

	def2 = new PTree::ClassSpec(def->encoded_name(), def->car(), rest, 0);
    }

    return new PTree::Declaration(0, PTree::list(def2, Class::semicolon_t));
}

PTree::Array* ClassWalker::RecordMembers(PTree::Node *class_def, PTree::Node *bases,
					 Class* metaobject)
{
  PTree::Node *tspec, *tspec2;

    new_scope(metaobject);
    RecordBaseclassEnv(bases);

    PTree::Array* tspec_list = new PTree::Array();

    PTree::Node *rest = PTree::second(PTree::nth(class_def, 3));
    while(rest != 0){
	PTree::Node *mem = rest->car();
	switch(PTree::type_of(mem)){
	case Token::ntTypedef :
	  tspec = PTree::second(mem);
	  tspec2 = translate_type_specifier(tspec);
	  my_environment->RecordTypedefName(PTree::third(mem));
	  if(tspec != tspec2)
	  {
	    tspec_list->append(tspec);
	    tspec_list->append(tspec2);
	  }
	  break;
	case Token::ntMetaclassDecl :
	  my_environment->RecordMetaclassName(mem);
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
	
    exit_scope();
    return tspec_list;
}

//  RecordMemberDeclaration() is derived from TranslateDeclaration().

void ClassWalker::RecordMemberDeclaration(PTree::Node *mem,
					  PTree::Array* tspec_list)
{
  PTree::Node *tspec, *tspec2, *decls;

  tspec = PTree::second(mem);
  tspec2 = translate_type_specifier(tspec);
  decls = PTree::third(mem);
  if(PTree::is_a(decls, Token::ntDeclarator))	// if it is a function
    my_environment->RecordDeclarator(decls);
  else if(!decls->is_atom())		// not a null declaration.
    while(decls != 0)
    {
      PTree::Node *d = decls->car();
      if(PTree::is_a(d, Token::ntDeclarator))
	my_environment->RecordDeclarator(d);

      decls = decls->cdr();
      if(decls) decls = decls->cdr();
    }

  if(tspec != tspec2)
  {
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

    return new PTree::AccessSpec(lf, PTree::list(Class::colon_t), 0);
}

PTree::Node *ClassWalker::ConstructMember(void* ptr)
{
    ChangedMemberList::Cmem* m = (ChangedMemberList::Cmem*)ptr;
    PTree::Node *def = m->def;
    PTree::Node *def2;

    if(PTree::is_a(PTree::third(def), Token::ntDeclarator)){
	// function implementation
	if(m->body == 0){
	    NameScope old_env;
	    Environment* fenv = my_environment->DontRecordDeclarator(m->declarator);
	    if(fenv != 0)
		old_env = change_scope(fenv);

	    new_scope();
	    def2 = MakeMemberDeclarator(true, m,
					(PTree::Declarator*)m->declarator);
	    def2 = PTree::list(def2,
			       translate_function_body(PTree::nth(def, 3)));
	    exit_scope();
	    if(fenv != 0)
		restore_scope(old_env);
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

    def2 = new PTree::Declaration(translate_storage_specifiers(PTree::first(def)),
				  PTree::cons(translate_type_specifier(PTree::second(def)),
					      def2));
    return def2;
}

PTree::Node *ClassWalker::translate_storage_specifiers(PTree::Node *spec)
{
    return translate_storage_specifiers2(spec);
}

PTree::Node *ClassWalker::translate_storage_specifiers2(PTree::Node *rest)
{
    if(rest == 0)
	return 0;
    else{
	PTree::Node *h = rest->car();
	PTree::Node *t = rest->cdr();
	PTree::Node *t2 = translate_storage_specifiers2(t);
	if(PTree::is_a(h, Token::ntUserdefKeyword))
	    return t2;
	else if(t == t2)
	    return rest;
	else
	    return PTree::cons(h, t2);
    }
}

PTree::Node *ClassWalker::translate_template_function(PTree::Node *temp_def, PTree::Node *impl)
{
    Environment* fenv = my_environment->RecordTemplateFunction(temp_def, impl);
    if (fenv != 0) {
	Class* metaobject = fenv->IsClassEnvironment();
	if(metaobject != 0){
	    NameScope old_env = change_scope(fenv);
	    new_scope();

	    ChangedMemberList::Cmem m;
	    PTree::Node *decl = PTree::third(impl);
	    MemberFunction mem(metaobject, impl, decl);
	    metaobject->TranslateMemberFunction(my_environment, mem);
	    ChangedMemberList::Copy(&mem, &m, Class::Undefined);
	    PTree::Node *decl2
		= MakeMemberDeclarator(true, &m, (PTree::Declarator*)decl);

	    exit_scope();
	    restore_scope(old_env);
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

PTree::Node *ClassWalker::translate_function_implementation(PTree::Node *impl)
{
    PTree::Node *sspec = PTree::first(impl);
    PTree::Node *sspec2 = translate_storage_specifiers(sspec);
    PTree::Node *tspec = PTree::second(impl);
    PTree::Node *decl = PTree::third(impl);
    PTree::Node *body = PTree::nth(impl, 3);
    PTree::Node *decl2;
    PTree::Node *body2;

    PTree::Node *tspec2 = translate_type_specifier(tspec);
    Environment* fenv = my_environment->RecordDeclarator(decl);

    if(fenv == 0){
	// reach here if resolving the qualified name fails. error?
	new_scope();
	decl2 = translate_declarator(true, (PTree::Declarator*)decl);
	body2 = translate_function_body(body);
	exit_scope();
    }
    else{
	Class* metaobject = fenv->IsClassEnvironment();
	NameScope old_env = change_scope(fenv);
	new_scope();

	if (metaobject == 0 && Class::metaclass_for_c_functions != 0)
	    metaobject = MakeMetaobjectForCfunctions();

	if(metaobject == 0){
	    decl2 = translate_declarator(true, (PTree::Declarator*)decl);
	    body2 = translate_function_body(body);
	}
	else{
	    ChangedMemberList::Cmem m;
	    MemberFunction mem(metaobject, impl, decl);
	    metaobject->TranslateMemberFunction(my_environment, mem);
	    ChangedMemberList::Copy(&mem, &m, Class::Undefined);
	    decl2 = MakeMemberDeclarator(true, &m, (PTree::Declarator*)decl);
	    if(m.body != 0)
		body2 = m.body;
	    else
		body2 = translate_function_body(body);
	}

	exit_scope();
	restore_scope(old_env);
    }

    if(sspec == sspec2 && tspec == tspec2 && decl == decl2 && body == body2)
	return impl;
    else
	return new PTree::Declaration(sspec2,
				      PTree::list(tspec2, decl2, body2));
}

Class* ClassWalker::MakeMetaobjectForCfunctions() {
    if (Class::for_c_functions == 0) {
	PTree::Encoding encode;
	PTree::Node *name = new PTree::Atom("<C>", 3);
	encode.simple_name(name);
	PTree::Node *class_def
	  = new PTree::ClassSpec(encode,
				 Class::class_t,
				 PTree::list(name, 0, Class::empty_block_t),
				 0);
	std::cerr << "encode: " << class_def->encoded_name();
	Class* metaobject = opcxx_ListOfMetaclass::New(
			Class::metaclass_for_c_functions,
			class_def, 0);
	if(metaobject == 0)
	    MopErrorMessage2(
		"the metaclass for C functions cannot be loaded: ",
		Class::metaclass_for_c_functions);

	metaobject->SetEnvironment(my_environment);
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
	    args2 = translate_arg_decl_list2(record, my_environment, true,
					     m->arg_name_filled, 0, args);
	else{
	    args2 = m->args;
	    // we may need to record the arguments.
	    translate_arg_decl_list2(record, my_environment, false, false, 0, args);
	}
    else
	args = args2 = 0;

    name = decl->name();
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

PTree::Node *ClassWalker::record_args_and_translate_fbody(Class* c, PTree::Node *args,
							  PTree::Node *body)
{
    NameScope old_env;
    Environment* fenv = c->GetEnvironment();

    if(fenv != 0)
	old_env = change_scope(fenv);

    new_scope();
    translate_arg_decl_list2(true, my_environment, false, false, 0, args);
    PTree::Node *body2 = translate_function_body(body);
    exit_scope();

    if(fenv != 0)
	restore_scope(old_env);

    return body2;
}

PTree::Node *ClassWalker::translate_function_body(PTree::Node *body)
{
    PTree::Node *body2;

    inserted_declarations.clear();
    client_data = 0;
    body = translate(body);
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

void ClassWalker::visit(PTree::Block *node)
{
  new_scope();

  PTree::Array array;
  bool changed = false;
  PTree::Node *body = PTree::second(node);
  PTree::Node *rest = body;
  while(rest)
  {
    size_t i, n;
    PTree::Node *p = rest->car();
    PTree::Node *q = translate(p);

    n = before_statement.number();
    if(n > 0)
    {
      changed = true;
      for(i = 0; i < n; ++i)
	array.append(before_statement[i]);
    }

    array.append(q);
    if(p != q) changed = true;

    n = after_statement.number();
    if(n > 0)
    {
      changed = true;
      for(i = 0; i < n; ++i)
	array.append(after_statement[i]);
    }

    before_statement.clear();
    after_statement.clear();
    rest = rest->cdr();
  }
  
  if(changed)
    my_result = new PTree::Block(PTree::first(node), array.all(),
				 PTree::third(node));
  else
    my_result = node;

  exit_scope();
}

PTree::Node *ClassWalker::translate_arg_decl_list(bool record,
						  PTree::Node *,
						  PTree::Node *args)
{
  return translate_arg_decl_list2(record, my_environment, true, false, 0, args);
}

PTree::Node *ClassWalker::translate_initialize_args(PTree::Declarator* decl,
						    PTree::Node *init)
{
  TypeInfo tinfo;
  my_environment->Lookup(decl, tinfo);
  Class* metaobject = tinfo.class_metaobject();
  if(metaobject)
    return metaobject->TranslateInitializer(my_environment, decl->name(), init);
  else
    return translate_arguments(init);
}

PTree::Node *ClassWalker::translate_assign_initializer(PTree::Declarator* decl,
						       PTree::Node *init)
{
  TypeInfo tinfo;
  my_environment->Lookup(decl, tinfo);
  Class* metaobject = tinfo.class_metaobject();
  if(metaobject)
    return metaobject->TranslateInitializer(my_environment, decl->name(), init);
  else
  {
    PTree::Node *exp = PTree::second(init);
    PTree::Node *exp2 = translate(exp);
    if(exp == exp2)
      return init;
    else
      return PTree::list(init->car(), exp2);
  }
}

void ClassWalker::visit(PTree::AssignExpr *node)
{
  PTree::Node *left, *left2, *right, *right2, *exp2;
  Environment* scope;
  Class* metaobject;
  TypeInfo type;

  left = PTree::first(node);
  right = PTree::third(node);
  if(PTree::is_a(left, Token::ntDotMemberExpr, Token::ntArrowMemberExpr))
  {
    PTree::Node *object = PTree::first(left);
    PTree::Node *op = PTree::second(left);
    PTree::Node *member = PTree::third(left);
    PTree::Node *assign_op = PTree::second(node);
    type_of(object, my_environment, type);
    if(*op != '.')
      type.dereference();

    metaobject = get_class_metaobject(type);
    if(metaobject)
    {
      exp2 = metaobject->TranslateMemberWrite(my_environment, object, op,
					      member, assign_op, right);
      my_result = CheckMemberEquiv(node, exp2);
    }
  }
  else if((scope = my_environment->IsMember(left)))
  {
    metaobject = scope->IsClassEnvironment();
    if(metaobject)
    {
      exp2 = metaobject->TranslateMemberWrite(my_environment, left, PTree::second(node),
					      right);
      my_result = CheckEquiv(node, exp2);
    }
  }
  else
  {
    type_of(left, my_environment, type);
    metaobject = get_class_metaobject(type);
    if(metaobject)
    {
      exp2 = metaobject->TranslateAssign(my_environment, left, PTree::second(node), right);
      my_result = CheckEquiv(node, exp2);
    }
  }

  left2 = translate(left);
  right2 = translate(right);
  if(left == left2 && right == right2)
    my_result = node;
  else
    my_result = new PTree::AssignExpr(left2, PTree::list(PTree::second(node), right2));
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

void ClassWalker::visit(PTree::InfixExpr *node)
{
  PTree::Node *left = PTree::first(node);
  PTree::Node *right = PTree::third(node);
  TypeInfo type;
  type_of(right, my_environment, type);
  Class* metaobject = get_class_metaobject(type);
  if(!metaobject)
  {
    type_of(left, my_environment, type);
    metaobject = get_class_metaobject(type);
  }

  if(metaobject)
  {
    PTree::Node *exp2 = metaobject->TranslateBinary(my_environment,
						    left, PTree::second(node),
						    right);
    my_result = CheckEquiv(node, exp2);
  }
  else
  {
    PTree::Node *left2 = translate(left);
    PTree::Node *right2 = translate(right);
    if(left == left2 && right == right2)
      my_result = node;
    else
      my_result = new PTree::InfixExpr(left2, PTree::list(PTree::second(node), right2));
    }
}

void ClassWalker::visit(PTree::UnaryExpr *node)
{
  TypeInfo type;
  Environment* scope;
  Class* metaobject;
  PTree::Node *exp2;

  PTree::Node *unaryop = node->car();
  PTree::Node *right = PTree::second(node);
  if(PTree::is_a(right, Token::ntDotMemberExpr, Token::ntArrowMemberExpr))
  {
    PTree::Node *object = PTree::first(right);
    PTree::Node *op = PTree::second(right);
    type_of(object, my_environment, type);
    if(*op != '.') type.dereference();

    metaobject = get_class_metaobject(type);
    if(metaobject)
    {
      exp2 = metaobject->TranslateUnaryOnMember(my_environment, unaryop, object,
						op, PTree::third(right));
      if(PTree::length(exp2) == 2 && exp2->car() == unaryop
	 && PTree::equiv(PTree::second(exp2), right))
	my_result = node;
      else
	my_result = exp2;
    }
  }    
  else if((scope = my_environment->IsMember(right)))
  {
    metaobject = scope->IsClassEnvironment();
    if(metaobject)
    {
      exp2 = metaobject->TranslateUnaryOnMember(my_environment, unaryop, right);
      my_result = CheckEquiv(node, exp2);
    }
  }

  type_of(right, my_environment, type);
  metaobject = get_class_metaobject(type);
  if(metaobject)
  {
    PTree::Node *exp2 = metaobject->TranslateUnary(my_environment, unaryop, right);
    my_result = CheckEquiv(node, exp2);
  }
  else
  {
    PTree::Node *right2 = translate(right);
    if(right == right2)
      my_result = node;
    else
      my_result = new PTree::UnaryExpr(unaryop, PTree::list(right2));
  }
}

void ClassWalker::visit(PTree::ArrayExpr *node)
{
  TypeInfo type;

  PTree::Node *array = node->car();
  type_of(array, my_environment, type);
  Class* metaobject = get_class_metaobject(type);
  if(metaobject)
  {
    PTree::Node *exp2 = metaobject->TranslateSubscript(my_environment,
						       array, node->cdr());
    my_result = CheckEquiv(node, exp2);
  }
  else
  {
    PTree::Node *index = PTree::third(node);
    PTree::Node *array2 = translate(array);
    PTree::Node *index2 = translate(index);
    if(array == array2 && index == index2)
      my_result = node;
    else
      my_result = new PTree::ArrayExpr(array2,
				       PTree::shallow_subst(index2,
							    index,
							    node->cdr()));
  }
}

void ClassWalker::visit(PTree::PostfixExpr *node)
{
  TypeInfo type;
  Environment* scope;
  Class* metaobject;
  PTree::Node *exp2;

  PTree::Node *left = node->car();
  PTree::Node *postop = PTree::second(node);
  if(PTree::is_a(left, Token::ntDotMemberExpr, Token::ntArrowMemberExpr))
  {
    PTree::Node *object = PTree::first(left);
    PTree::Node *op = PTree::second(left);
    type_of(object, my_environment, type);
    if(*op != '.') type.dereference();

    metaobject = get_class_metaobject(type);
    if(metaobject)
    {
      exp2 = metaobject->TranslatePostfixOnMember(my_environment, object, op,
						  PTree::third(left), postop);
      my_result = CheckMemberEquiv(node, exp2);
    }
  }    
  else if((scope = my_environment->IsMember(left)))
  {
    metaobject = scope->IsClassEnvironment();
    if(metaobject)
    {
      exp2 = metaobject->TranslatePostfixOnMember(my_environment, left, postop);
      my_result = CheckEquiv(node, exp2);
    }
  }
  type_of(left, my_environment, type);
  metaobject = get_class_metaobject(type);
  if(metaobject)
  {
    exp2 = metaobject->TranslatePostfix(my_environment, left, postop);
    my_result = CheckEquiv(node, exp2);
  }
  else
  {
    PTree::Node *left2 = translate(left);
    if(left == left2)
      my_result = node;
    else
      my_result = new PTree::PostfixExpr(left2, node->cdr());
  }
}

void ClassWalker::visit(PTree::FuncallExpr *node)
{
  TypeInfo type;
  Environment* scope;
  Class* metaobject;
  PTree::Node *fun, *arglist, *exp2;

  fun = node->car();
  arglist = node->cdr();
  if(PTree::is_a(fun, Token::ntDotMemberExpr, Token::ntArrowMemberExpr))
  {
    PTree::Node *object = PTree::first(fun);
    PTree::Node *op = PTree::second(fun);
    PTree::Node *member = PTree::third(fun);
    type_of(object, my_environment, type);
    if(*op != '.') type.dereference();

    metaobject = get_class_metaobject(type);
    if(metaobject)
    {
      exp2 = metaobject->TranslateMemberCall(my_environment, object, op,
					     member, arglist);
      my_result = CheckMemberEquiv(node, exp2);
    }
  }
  else if((scope = my_environment->IsMember(fun)))
  {
    metaobject = scope->IsClassEnvironment();
    if(metaobject)
    {
      exp2 = metaobject->TranslateMemberCall(my_environment, fun, arglist);
      my_result = CheckEquiv(node, exp2);
    }
  }
  else
  {
    type_of(fun, my_environment, type);
    metaobject = get_class_metaobject(type);
    if(metaobject)
    {
      exp2 = metaobject->TranslateFunctionCall(my_environment, fun, arglist);
      my_result = CheckEquiv(node, exp2);
    }
  }

  PTree::Node *fun2 = translate(fun);
  PTree::Node *arglist2 = translate_arguments(arglist);
  if(fun == fun2 && arglist == arglist2)
    my_result = node;
  else
    my_result = new PTree::FuncallExpr(fun2, arglist2);
}

void ClassWalker::visit(PTree::DotMemberExpr *node)
{
  TypeInfo type;

  PTree::Node *left = node->car();
  type_of(left, my_environment, type);
  Class* metaobject = get_class_metaobject(type);
  if(metaobject)
  {
    PTree::Node *exp2 = metaobject->TranslateMemberRead(my_environment,
							left, PTree::second(node),
							PTree::third(node));
    my_result = CheckEquiv(node, exp2);
  }
  else
  {
    PTree::Node *left2 = translate(left);
    if(left == left2)
      my_result = node;
    else
      my_result = new PTree::DotMemberExpr(left2, node->cdr());
  }
}

void ClassWalker::visit(PTree::ArrowMemberExpr *node)
{
  TypeInfo type;

  PTree::Node *left = node->car();
  type_of(left, my_environment, type);
  type.dereference();
  Class* metaobject = get_class_metaobject(type);
  if(metaobject)
  {
    PTree::Node *exp2 = metaobject->TranslateMemberRead(my_environment,
							left, PTree::second(node),
							PTree::third(node));
    my_result = CheckEquiv(node, exp2);
  }
  else
  {
    PTree::Node *left2 = translate(left);
    if(left == left2)
      my_result = node;
    else
      my_result = new PTree::ArrowMemberExpr(left2, node->cdr());
  }
}

void ClassWalker::visit(PTree::Kwd::This *node)
{
  TypeInfo type;
  type_of(node, my_environment, type);
  type.dereference();
  Class* metaobject = get_class_metaobject(type);
  if(metaobject)
    my_result = metaobject->TranslatePointer(my_environment, node);
  else
    my_result = node;
}

PTree::Node *ClassWalker::translate_variable(PTree::Node *exp)
{
    Environment* scope = my_environment->IsMember(exp);
    if(scope != 0){
	Class* metaobject = scope->IsClassEnvironment();
	if(metaobject != 0)
	    return metaobject->TranslateMemberRead(my_environment, exp);
    }

    TypeInfo type;
    type_of(exp, my_environment, type);
    if(type.is_pointer_type()){
	type.dereference();
	Class* metaobject = get_class_metaobject(type);
	if(metaobject != 0)
	    return metaobject->TranslatePointer(my_environment, exp);
    }

    return exp;
}

void ClassWalker::visit(PTree::UserStatementExpr *node)
{
  TypeInfo type;
  PTree::Node *object = PTree::first(node);
  PTree::Node *op = PTree::second(node);
  PTree::Node *keyword = PTree::third(node);
  PTree::Node *rest = PTree::tail(node, 3);

  type_of(object, my_environment, type);
  if(*op != '.') type.dereference();

  Class* metaobject = get_class_metaobject(type);
  if(metaobject)
  {
    new_scope();
    if(PTree::is_a(keyword, Token::UserKeyword2))		// closure style
      translate_arg_decl_list2(true, my_environment,
			       false, false, 0, PTree::second(rest));

    PTree::Node *exp2 = metaobject->TranslateUserStatement(my_environment, object, op,
							   keyword, rest);
    exit_scope();
    my_result = exp2;
  }
  else
  {
    Walker::error_message("no complete class specification for: ", object, node);
    my_result = 0;
  }
}

void ClassWalker::visit(PTree::StaticUserStatementExpr *node)
{
  bool is_type_name;
  TypeInfo type;
  PTree::Node *qualifier = PTree::first(node);
  PTree::Node *keyword = PTree::third(node);
  PTree::Node *rest = PTree::tail(node, 3);

  if(my_environment->Lookup(qualifier, is_type_name, type))
    if(is_type_name)
    {
      Class* metaobject = get_class_metaobject(type);
      if(metaobject)
      {
	new_scope();
	if(PTree::is_a(keyword, Token::UserKeyword2))		// closure style
	  translate_arg_decl_list2(true, my_environment, false, false, 0,
				   PTree::second(rest));
	PTree::Node *exp2 = metaobject->TranslateStaticUserStatement(my_environment,
								     keyword, rest);
	exit_scope();
	my_result = exp2;
	return;
      }
    }

  Walker::error_message("no complete class specification for: ", qualifier, node);
  my_result = 0;
}

PTree::Node *ClassWalker::translate_new2(PTree::Node *exp, PTree::Node *userkey, PTree::Node *scope,
					 PTree::Node *op, PTree::Node *placement,
					 PTree::Node *type, PTree::Node *init)
{
  TypeInfo t;

  if(type->car() && *type->car() == '(')
    t.set(PTree::second(PTree::second(type))->encoded_type(), my_environment);
  else
    t.set(PTree::second(type)->encoded_type(), my_environment);

  Class* metaobject = get_class_metaobject(t);
  if(metaobject != 0)
  {
    if(userkey == 0)
      userkey = scope;
    
    PTree::Node *exp2 = metaobject->TranslateNew(my_environment, userkey, op,
						 placement, type, init);
    return CheckEquiv(exp, exp2);
  }
  else
  {
    PTree::Node *placement2 = translate_arguments(placement);
    PTree::Node *type2 = translate_new3(type);
    PTree::Node *init2 = translate_arguments(init);
    if(userkey == 0 && placement == placement2
       && type == type2 && init == init2)
      return exp;
    else
    {
      if(userkey == 0)
	return new PTree::NewExpr(exp->car(),
				  PTree::shallow_subst(placement2, placement,
						       type2, type,
						       init2, init, exp->cdr()));
      else
      {
	Walker::error_message("no complete class specification for: ", type, exp);
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

void ClassWalker::visit(PTree::DeleteExpr *node)
{
  TypeInfo type;

  PTree::Node *obj = PTree::last(node)->car();
  if(PTree::length(node) == 2)
  {	// not ::delete or delete []
    type_of(obj, my_environment, type);
    type.dereference();
    Class* metaobject = get_class_metaobject(type);
    if(metaobject)
    {
      PTree::Node *exp2 = metaobject->TranslateDelete(my_environment, node->car(), obj);
      my_result = CheckEquiv(node, exp2);
    }
  }

  PTree::Node *obj2 = translate(obj);
  if(obj == obj2)
    my_result = node;
  else
    my_result = new PTree::DeleteExpr(node->car(),
				      PTree::shallow_subst(obj2, obj,
							   node->cdr()));
}
