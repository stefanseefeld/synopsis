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

#ifndef _ClassBodyWalker_hh
#define _ClassBodyWalker_hh

#include "ClassWalker.hh"

//. ClassBodyWalker is only used by ClassWalker::TranslateClassSpec.
class ClassBodyWalker : public ClassWalker 
{
public:
  ClassBodyWalker(Walker *, PTree::Array *);
  void append_new_members(Class*, PTree::Array&, bool&);
  void visit(PTree::Typedef *);
  void visit(PTree::MetaclassDecl *);
  PTree::ClassBody *translate_class_body(PTree::ClassBody *block, PTree::Node *bases, Class*);
  PTree::Node *translate_type_specifier(PTree::Node *);  
  PTree::Node *translate_declarators(PTree::Node *);
  PTree::Node *translate_assign_initializer(PTree::Declarator*, PTree::Node *);
  PTree::Node *translate_initialize_args(PTree::Declarator*, PTree::Node *);
  PTree::Node *translate_declarator(bool, PTree::Declarator*);
  PTree::Node *translate_declarator(bool, PTree::Declarator*, bool);
  PTree::Node *translate_function_implementation(PTree::Node *);
private:
  PTree::Array* tspec_list;
};

#endif
