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

// ClassBodyWalker is only used by ClassWalker::TranslateClassSpec.

class ClassBodyWalker : public ClassWalker 
{
public:
  ClassBodyWalker(Walker* w, PTree::Array* tlist) : ClassWalker(w) 
  {
    tspec_list = tlist;
  }
  PTree::Node *TranslateClassBody(PTree::Node *block, PTree::Node *bases, Class*);
  void AppendNewMembers(Class*, PTree::Array&, bool&);
  PTree::Node *TranslateTypespecifier(PTree::Node *tspec);
  PTree::Node *TranslateTypedef(PTree::Node *def);
  PTree::Node *TranslateMetaclassDecl(PTree::Node *decl);
  PTree::Node *TranslateDeclarators(PTree::Node *decls);
  PTree::Node *TranslateAssignInitializer(PTree::Declarator* decl, PTree::Node *init);
  PTree::Node *TranslateInitializeArgs(PTree::Declarator* decl, PTree::Node *init);
  PTree::Node *TranslateDeclarator(bool record, PTree::Declarator* decl);
  PTree::Node *TranslateDeclarator(bool record, PTree::Declarator* decl,
				   bool append_body);
  PTree::Node *TranslateFunctionImplementation(PTree::Node *impl);

private:
  PTree::Array* tspec_list;
};

#endif
