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

ClassBodyWalker::ClassBodyWalker(Walker* w, PTree::Array* tlist)
  : ClassWalker(w), 
    tspec_list(tlist)
{
}

PTree::ClassBody *
ClassBodyWalker::translate_class_body(PTree::ClassBody *block,
				      PTree::Node *,
				      Class *metaobject)
{
  PTree::ClassBody *block2;
  Environment *fenv = metaobject->GetEnvironment();
  if(!fenv) fenv = my_environment; // should not reach here.
  NameScope old_env = change_scope(fenv);
  PTree::Array array;
  bool changed = false;
  PTree::Node *body = PTree::second(block);
  PTree::Node *rest = body;
  while(rest)
  {
    PTree::Node *p = rest->car();
    PTree::Node *q = translate(p);
    array.append(q);
    if(p != q) changed = true;
    rest = rest->cdr();
  }

  append_new_members(metaobject, array, changed);

  PTree::Node *appended = metaobject->GetAppendedCode();
  if(appended)
  {
    changed = true;
    while(appended)
    {
      array.append(appended->car());
      appended = appended->cdr();
    }
  }

  if(changed)
    block2 = new PTree::ClassBody(PTree::first(block), array.all(),
				  PTree::third(block));
  else
    block2 = block;

  restore_scope(old_env);
  return block2;
}

void ClassBodyWalker::append_new_members(Class *metaobject,
					 PTree::Array &array,
					 bool &changed)
{
  ChangedMemberList::Cmem *m;
  ChangedMemberList *appended_list = metaobject->GetAppendedMembers();
  if(!appended_list) return;
  int i = 0;
  while((m = appended_list->Get(i++)) != 0)
    if(m->def)
    {
      changed = true;
      ClassWalker w(this);
      array.append(w.ConstructAccessSpecifier(m->access));
      array.append(w.ConstructMember(m));
    }
}

PTree::Node *ClassBodyWalker::translate_type_specifier(PTree::Node *tspec)
{
  if(!tspec_list) return tspec;
  size_t n = tspec_list->number();
  for(size_t i =  0; i < n; i += 2)
    if(tspec_list->ref(i) == tspec)
      return tspec_list->ref(i + 1);
  return tspec;
}

void ClassBodyWalker::visit(PTree::Typedef *node)
{
  PTree::Node *tspec = PTree::second(node);
  PTree::Node *tspec2 = translate_type_specifier(tspec);
  if(tspec == tspec2) my_result = node;
  else my_result = new PTree::Typedef(PTree::first(node),
				      PTree::list(tspec2, PTree::tail(node, 2)));
}

void ClassBodyWalker::visit(PTree::MetaclassDecl *)
{
  my_result = 0;
}

PTree::Node *ClassBodyWalker::translate_declarators(PTree::Node *decls)
{
  return ClassWalker::translate_declarators(decls, false);
}

PTree::Node *ClassBodyWalker::translate_assign_initializer(PTree::Declarator *decl,
							   PTree::Node *init)
{
  ClassWalker w(this);
  return w.translate_assign_initializer(decl, init);
}

PTree::Node *ClassBodyWalker::translate_initialize_args(PTree::Declarator *decl,
							PTree::Node *init)
{
  ClassWalker w(this);
  return w.translate_initialize_args(decl, init);
}

PTree::Node *ClassBodyWalker::translate_declarator(bool record,
						   PTree::Declarator *decl)
{
  return translate_declarator(record, decl, true);
}

PTree::Node *ClassBodyWalker::translate_declarator(bool record,
						   PTree::Declarator *decl,
						   bool append_body)
{
  ClassWalker w(this);
  Class *metaobject = my_environment->LookupThis();
  if(metaobject)
  {
    ChangedMemberList::Cmem *m = metaobject->GetChangedMember(decl);
    if(m)
    {
      PTree::Node *decl2 = w.MakeMemberDeclarator(record, m, decl);
      if(m->removed || m->body == 0 || !append_body) return decl2;
      else return PTree::list(decl2, m->body);
    }
  }
  return w.translate_declarator(record, decl);
}

PTree::Node *ClassBodyWalker::translate_function_implementation(PTree::Node *impl)
{
  PTree::Node *sspec = PTree::first(impl);
  PTree::Node *sspec2 = translate_storage_specifiers(sspec);
  PTree::Node *tspec = PTree::second(impl);
  PTree::Node *decl = PTree::third(impl);
  PTree::Node *body = PTree::nth(impl, 3);
  PTree::Node *decl2;
  PTree::Node *body2;

  PTree::Node *tspec2 = translate_type_specifier(tspec);
  Environment *fenv = my_environment->DontRecordDeclarator(decl);

  if(fenv == 0)
  {
    // shouldn't reach here.
    new_scope();
    ClassWalker w(this);	// this should be after NewScope().
    decl2 = w.translate_declarator(true, (PTree::Declarator*)decl);
    body2 = w.translate_function_body(body);
    exit_scope();
  }
  else
  {
    bool is_nested_class = bool(my_environment != fenv);
    NameScope old_env = change_scope(fenv);
    new_scope();
    ClassWalker w(this);
    if(is_nested_class)
    {
      // if it's a member function of a nested class
      decl2 = w.translate_declarator(true, (PTree::Declarator*)decl);
      body2 = w.translate_function_body(body);
    }
    else
    {
      decl2 = translate_declarator(true, (PTree::Declarator*)decl, false);
      Class* metaobject = fenv->IsClassEnvironment();
      ChangedMemberList::Cmem* m = 0;
      if(metaobject) m = metaobject->GetChangedMember(decl);
      if(m && m->body) body2 = m->body;
      else body2 = w.translate_function_body(body);
    }
    exit_scope();
    restore_scope(old_env);
  }
  if(sspec == sspec2 && tspec == tspec2 && decl == decl2 && body == body2)
    return impl;
  else if(decl2 == 0)
    return new PTree::Declaration(0, PTree::list(0, Class::semicolon_t));
  else
    return new PTree::Declaration(sspec2, PTree::list(tspec2, decl2, body2));
}
