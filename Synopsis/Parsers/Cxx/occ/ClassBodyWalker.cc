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

#include "PTree.hh"
#include "ClassBodyWalker.hh"
#include "Class.hh"
#include "Environment.hh"
#include "Member.hh"

PTree::Node *ClassBodyWalker::TranslateClassBody(PTree::Node *block, PTree::Node *,
					   Class* metaobject)
{
    PTree::Node *block2;

    Environment* fenv = metaobject->GetEnvironment();
    if(fenv == 0)
	fenv = env;	// should not reach here.

    NameScope old_env = ChangeScope(fenv);

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

    AppendNewMembers(metaobject, array, changed);

    PTree::Node *appended = metaobject->GetAppendedCode();
    if(appended != 0){
	changed = true;
	while(appended != 0){
	    array.append(appended->car());
	    appended = appended->cdr();
	}
    }

    if(changed)
	block2 = new PTree::ClassBody(PTree::first(block), array.all(),
				      PTree::third(block));
    else
	block2 = block;

    RestoreScope(old_env);
    return block2;
}

void ClassBodyWalker::AppendNewMembers(Class* metaobject, PTree::Array& array,
				       bool& changed)
{
    ChangedMemberList::Cmem* m;
    ChangedMemberList* appended_list = metaobject->GetAppendedMembers();
    if(appended_list == 0)
	return;

    int i = 0;
    while((m = appended_list->Get(i++)) != 0)
	if(m->def != 0){
	    changed = true;
	    ClassWalker w(this);
	    array.append(w.ConstructAccessSpecifier(m->access));
	    array.append(w.ConstructMember(m));
	}
}

PTree::Node *ClassBodyWalker::TranslateTypespecifier(PTree::Node *tspec)
{
  if(!tspec_list) return tspec;
  size_t n = tspec_list->number();
  for(size_t i =  0; i < n; i += 2)
    if(tspec_list->ref(i) == tspec)
      return tspec_list->ref(i + 1);
  return tspec;
}

PTree::Node *ClassBodyWalker::TranslateTypedef(PTree::Node *def)
{
  PTree::Node *tspec = PTree::second(def);
  PTree::Node *tspec2 = TranslateTypespecifier(tspec);
  if(tspec == tspec2) return def;
  else return new PTree::Typedef(PTree::first(def),
				 PTree::list(tspec2,
					     PTree::tail(def, 2)));
}

PTree::Node *ClassBodyWalker::TranslateMetaclassDecl(PTree::Node *)
{
  return 0;
}

PTree::Node *ClassBodyWalker::TranslateDeclarators(PTree::Node *decls)
{
  return ClassWalker::TranslateDeclarators(decls, false);
}

PTree::Node *ClassBodyWalker::TranslateAssignInitializer(PTree::Declarator* decl,
							 PTree::Node *init)
{
  ClassWalker w(this);
  return w.TranslateAssignInitializer(decl, init);
}

PTree::Node *ClassBodyWalker::TranslateInitializeArgs(PTree::Declarator* decl,
						      PTree::Node *init)
{
  ClassWalker w(this);
  return w.TranslateInitializeArgs(decl, init);
}

PTree::Node *ClassBodyWalker::TranslateDeclarator(bool record,
						  PTree::Declarator* decl)
{
  return TranslateDeclarator(record, decl, true);
}

PTree::Node *ClassBodyWalker::TranslateDeclarator(bool record,
					    PTree::Declarator* decl,
					    bool append_body)
{
  ClassWalker w(this);
  Class* metaobject = env->LookupThis();
  if(metaobject)
  {
    ChangedMemberList::Cmem* m = metaobject->GetChangedMember(decl);
    if(m != 0)
    {
      PTree::Node *decl2 = w.MakeMemberDeclarator(record, m, decl);
      if(m->removed || m->body == 0 || !append_body) return decl2;
      else return PTree::list(decl2, m->body);
    }
  }
  return w.TranslateDeclarator(record, decl);
}

PTree::Node *ClassBodyWalker::TranslateFunctionImplementation(PTree::Node *impl)
{
  PTree::Node *sspec = PTree::first(impl);
  PTree::Node *sspec2 = TranslateStorageSpecifiers(sspec);
  PTree::Node *tspec = PTree::second(impl);
  PTree::Node *decl = PTree::third(impl);
  PTree::Node *body = PTree::nth(impl, 3);
  PTree::Node *decl2;
  PTree::Node *body2;

  PTree::Node *tspec2 = TranslateTypespecifier(tspec);
  Environment* fenv = env->DontRecordDeclarator(decl);

  if(fenv == 0)
  {
    // shouldn't reach here.
    NewScope();
    ClassWalker w(this);	// this should be after NewScope().
    decl2 = w.TranslateDeclarator(true, (PTree::Declarator*)decl);
    body2 = w.TranslateFunctionBody(body);
    ExitScope();
  }
  else
  {
    bool is_nested_class = bool(env != fenv);
    NameScope old_env = ChangeScope(fenv);
    NewScope();
    ClassWalker w(this);
    if(is_nested_class)
    {
      // if it's a member function of a nested class
      decl2 = w.TranslateDeclarator(true, (PTree::Declarator*)decl);
      body2 = w.TranslateFunctionBody(body);
    }
    else
    {
      decl2 = TranslateDeclarator(true, (PTree::Declarator*)decl, false);
      Class* metaobject = fenv->IsClassEnvironment();
      ChangedMemberList::Cmem* m = 0;
      if(metaobject) m = metaobject->GetChangedMember(decl);
      if(m && m->body) body2 = m->body;
      else body2 = w.TranslateFunctionBody(body);
    }
    ExitScope();
    RestoreScope(old_env);
  }
  if(sspec == sspec2 && tspec == tspec2 && decl == decl2 && body == body2)
    return impl;
  else if(decl2 == 0)
    return new PTree::Declaration(0, PTree::list(0, Class::semicolon_t));
  else
    return new PTree::Declaration(sspec2, PTree::list(tspec2, decl2, body2));
}
