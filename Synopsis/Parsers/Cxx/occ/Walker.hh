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

#ifndef _Walker_hh
#define _Walker_hh

#include "types.h"

class Environment;
class TypeInfo;
class Class;
class Parser;
namespace PTree 
{
  class Node;
  class Declarator;
}

class Walker : public LightObject 
{
public:
  Walker(Parser*);
  Walker(Parser*, Environment*);
  Walker(Environment*);
  Walker(Walker*);

  PTree::Node *Translate(PTree::Node *);
  void Typeof(PTree::Node *, TypeInfo&);

  virtual bool IsClassWalker();

    // default translation
  virtual PTree::Node *TranslatePtree(PTree::Node *);
  virtual void TypeofPtree(PTree::Node *, TypeInfo&);

  virtual PTree::Node *TranslateTypedef(PTree::Node *);
  virtual PTree::Node *TranslateTemplateDecl(PTree::Node *);
  virtual PTree::Node *TranslateTemplateInstantiation(PTree::Node *);
  virtual PTree::Node *TranslateTemplateInstantiation(PTree::Node *, PTree::Node *,
						PTree::Node *, Class*);
  virtual Class* MakeTemplateInstantiationMetaobject(PTree::Node *full_class_spec, PTree::Node *userkey, PTree::Node *class_spec);
  virtual PTree::Node *TranslateExternTemplate(PTree::Node *);
  virtual PTree::Node *TranslateTemplateClass(PTree::Node *, PTree::Node *);
  virtual Class* MakeTemplateClassMetaobject(PTree::Node *, PTree::Node *, PTree::Node *);
  virtual PTree::Node *TranslateTemplateFunction(PTree::Node *, PTree::Node *);
  virtual PTree::Node *TranslateMetaclassDecl(PTree::Node *);
  virtual PTree::Node *TranslateLinkageSpec(PTree::Node *);
  virtual PTree::Node *TranslateNamespaceSpec(PTree::Node *);
  virtual PTree::Node *TranslateNamespaceAlias(PTree::Node *);
  virtual PTree::Node *TranslateUsing(PTree::Node *);
  virtual PTree::Node *TranslateDeclaration(PTree::Node *);
  virtual PTree::Node *TranslateStorageSpecifiers(PTree::Node *);
  virtual PTree::Node *TranslateDeclarators(PTree::Node *);
  virtual PTree::Node *TranslateDeclarator(bool, PTree::Declarator*);
  static bool GetArgDeclList(PTree::Declarator*, PTree::Node *&);
  virtual PTree::Node *TranslateArgDeclList(bool, PTree::Node *, PTree::Node *);
  static PTree::Node *TranslateArgDeclList2(bool, Environment*, bool, bool, int,
				      PTree::Node *);
  static PTree::Node *FillArgumentName(PTree::Node *, PTree::Node *, int arg_name);
  virtual PTree::Node *TranslateInitializeArgs(PTree::Declarator*, PTree::Node *);
  virtual PTree::Node *TranslateAssignInitializer(PTree::Declarator*, PTree::Node *);
  virtual PTree::Node *TranslateFunctionImplementation(PTree::Node *);
  virtual PTree::Node *RecordArgsAndTranslateFbody(Class*, PTree::Node *args,
					     PTree::Node *body);
  virtual PTree::Node *TranslateFunctionBody(PTree::Node *);
  virtual PTree::Node *TranslateBrace(PTree::Node *);
  virtual PTree::Node *TranslateBlock(PTree::Node *);
  virtual PTree::Node *TranslateClassBody(PTree::Node *, PTree::Node *, Class*);
  
  virtual PTree::Node *TranslateClassSpec(PTree::Node *);
  virtual Class* MakeClassMetaobject(PTree::Node *, PTree::Node *, PTree::Node *);
  virtual PTree::Node *TranslateClassSpec(PTree::Node *, PTree::Node *, PTree::Node *, Class*);
  virtual PTree::Node *TranslateEnumSpec(PTree::Node *);
  
  virtual PTree::Node *TranslateAccessSpec(PTree::Node *);
  virtual PTree::Node *TranslateAccessDecl(PTree::Node *);
  virtual PTree::Node *TranslateUserAccessSpec(PTree::Node *);
  
  virtual PTree::Node *TranslateIf(PTree::Node *);
  virtual PTree::Node *TranslateSwitch(PTree::Node *);
  virtual PTree::Node *TranslateWhile(PTree::Node *);
  virtual PTree::Node *TranslateDo(PTree::Node *);
  virtual PTree::Node *TranslateFor(PTree::Node *);
  virtual PTree::Node *TranslateTry(PTree::Node *);
  virtual PTree::Node *TranslateBreak(PTree::Node *);
  virtual PTree::Node *TranslateContinue(PTree::Node *);
  virtual PTree::Node *TranslateReturn(PTree::Node *);
  virtual PTree::Node *TranslateGoto(PTree::Node *);
  virtual PTree::Node *TranslateCase(PTree::Node *);
  virtual PTree::Node *TranslateDefault(PTree::Node *);
  virtual PTree::Node *TranslateLabel(PTree::Node *);
  virtual PTree::Node *TranslateExprStatement(PTree::Node *);
  
  virtual PTree::Node *TranslateTypespecifier(PTree::Node *);
  
  virtual PTree::Node *TranslateComma(PTree::Node *);
  virtual PTree::Node *TranslateAssign(PTree::Node *);
  virtual PTree::Node *TranslateCond(PTree::Node *);
  virtual PTree::Node *TranslateInfix(PTree::Node *);
  virtual PTree::Node *TranslatePm(PTree::Node *);
  virtual PTree::Node *TranslateCast(PTree::Node *);
  virtual PTree::Node *TranslateUnary(PTree::Node *);
  virtual PTree::Node *TranslateThrow(PTree::Node *);
  virtual PTree::Node *TranslateSizeof(PTree::Node *);
  virtual PTree::Node *TranslateTypeid(PTree::Node *);
  virtual PTree::Node *TranslateTypeof(PTree::Node *);
  virtual PTree::Node *TranslateNew(PTree::Node *);
  virtual PTree::Node *TranslateNew2(PTree::Node *, PTree::Node *, PTree::Node *, PTree::Node *,
			       PTree::Node *, PTree::Node *, PTree::Node *);
  virtual PTree::Node *TranslateNew3(PTree::Node *type);
  virtual PTree::Node *TranslateDelete(PTree::Node *);
  virtual PTree::Node *TranslateThis(PTree::Node *);
  virtual PTree::Node *TranslateVariable(PTree::Node *);
  virtual PTree::Node *TranslateFstyleCast(PTree::Node *);
  virtual PTree::Node *TranslateArray(PTree::Node *);
  virtual PTree::Node *TranslateFuncall(PTree::Node *);	// and fstyle cast
  virtual PTree::Node *TranslatePostfix(PTree::Node *);
  virtual PTree::Node *TranslateUserStatement(PTree::Node *);
  virtual PTree::Node *TranslateDotMember(PTree::Node *);
  virtual PTree::Node *TranslateArrowMember(PTree::Node *);
  virtual PTree::Node *TranslateParen(PTree::Node *);
  virtual PTree::Node *TranslateStaticUserStatement(PTree::Node *);
  
  virtual void TypeofComma(PTree::Node *, TypeInfo&);
  virtual void TypeofAssign(PTree::Node *, TypeInfo&);
  virtual void TypeofCond(PTree::Node *, TypeInfo&);
  virtual void TypeofInfix(PTree::Node *, TypeInfo&);
  virtual void TypeofPm(PTree::Node *, TypeInfo&);
  virtual void TypeofCast(PTree::Node *, TypeInfo&);
  virtual void TypeofUnary(PTree::Node *, TypeInfo&);
  virtual void TypeofThrow(PTree::Node *, TypeInfo&);
  virtual void TypeofSizeof(PTree::Node *, TypeInfo&);
  virtual void TypeofTypeid(PTree::Node *, TypeInfo&);
  virtual void TypeofTypeof(PTree::Node *, TypeInfo&);
  virtual void TypeofNew(PTree::Node *, TypeInfo&);
  virtual void TypeofDelete(PTree::Node *, TypeInfo&);
  virtual void TypeofThis(PTree::Node *, TypeInfo&);
  virtual void TypeofVariable(PTree::Node *, TypeInfo&);
  virtual void TypeofFstyleCast(PTree::Node *, TypeInfo&);
  virtual void TypeofArray(PTree::Node *, TypeInfo&);
  virtual void TypeofFuncall(PTree::Node *, TypeInfo&);	// and fstyle cast
  virtual void TypeofPostfix(PTree::Node *, TypeInfo&);
  virtual void TypeofUserStatement(PTree::Node *, TypeInfo&);
  virtual void TypeofDotMember(PTree::Node *, TypeInfo&);
  virtual void TypeofArrowMember(PTree::Node *, TypeInfo&);
  virtual void TypeofParen(PTree::Node *, TypeInfo&);
  virtual void TypeofStaticUserStatement(PTree::Node *, TypeInfo&);
  
public:
  struct NameScope 
  {
    Environment* env;
    Walker* walker;
  };
  
  void NewScope();
  void NewScope(Class*);
  Environment* ExitScope();
  void RecordBaseclassEnv(PTree::Node *);
  NameScope ChangeScope(Environment*);
  void RestoreScope(NameScope&);
  
protected:
  PTree::Node *TranslateDeclarators(PTree::Node *, bool);
  Class* LookupMetaclass(PTree::Node *, PTree::Node *, PTree::Node *, bool);
  
private:
  Class* LookupBaseMetaclass(PTree::Node *, PTree::Node *, bool);
  
public:
  PTree::Node *TranslateNewDeclarator(PTree::Node *decl);
  PTree::Node *TranslateNewDeclarator2(PTree::Node *decl);
  PTree::Node *TranslateArguments(PTree::Node *);
  static PTree::Node *GetClassOrEnumSpec(PTree::Node *);
  static PTree::Node *GetClassTemplateSpec(PTree::Node *);
  static PTree::Node *StripCvFromIntegralType(PTree::Node *);
  static void SetDeclaratorComments(PTree::Node *, PTree::Node *);
  static PTree::Node *FindLeftLeaf(PTree::Node *node, PTree::Node *& parent);
  static void SetLeafComments(PTree::Node *, PTree::Node *);
  static PTree::Node *NthDeclarator(PTree::Node *, int&);
  static PTree::Node *FindDeclarator(PTree::Node *, const char*, int, const char*, int&,
			       Environment*);
  static bool MatchedDeclarator(PTree::Node *, const char*, int, const char*, Environment*);
  static bool WhichDeclarator(PTree::Node *, PTree::Node *, int&, Environment*);
  
  void ErrorMessage(const char*, PTree::Node *, PTree::Node *);
  void WarningMessage(const char*, PTree::Node *, PTree::Node *);
  
  static void InaccurateErrorMessage(const char*, PTree::Node *, PTree::Node *);
  static void InaccurateWarningMessage(const char*, PTree::Node *, PTree::Node *);
  
  static void ChangeDefaultMetaclass(const char*);
  
public:
  Parser* GetParser() { return parser; }
  
protected:
  Environment* env;
  Parser* parser;
  
public:
  static const char* argument_name;
  
private:
  static Parser* default_parser;
  static const char* default_metaclass;
};

#endif
