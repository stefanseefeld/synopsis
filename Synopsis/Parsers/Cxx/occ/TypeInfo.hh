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

#ifndef _TypeInfo_hh
#define _TypeInfo_hh

#include <PTree.hh>

class Class;
class Environment;
class Bind;

enum TypeInfoId {
    UndefType,
    BuiltInType, ClassType, EnumType, TemplateType,
    PointerType, ReferenceType, PointerToMemberType,
    ArrayType, FunctionType
};

// These are used by TypeInfo::WhatBuiltinType()
//
enum {
    CharType = 1, IntType = 2, ShortType = 4, LongType = 8,
    LongLongType = 16, SignedType = 32, UnsignedType = 64, FloatType = 128,
    DoubleType = 256, LongDoubleType = 512, VoidType = 1024,
    BooleanType = 2048,
    WideCharType = 4096
};

/*
  TypeInfo interprets an encoded type name.  For details of the encoded
  type name, see class PTree::Encoding.
*/
class TypeInfo : public PTree::LightObject {
public:
  TypeInfo();
  void Unknown();
  void Set(const PTree::Encoding &, Environment*);
  void Set(Class*);
  void SetVoid();
  void SetInt();
  void SetMember(PTree::Node *);

  TypeInfoId WhatIs();

  bool IsNoReturnType();

  bool IsConst();
  bool IsVolatile();
  
  uint IsBuiltInType();
  bool IsFunction();
  bool IsEllipsis();
  bool IsPointerType();
  bool IsReferenceType();
  bool IsArray();
  bool IsPointerToMember();
  bool IsTemplateClass();
  Class* ClassMetaobject();
  bool IsClass(Class*&);
  bool IsEnum();
  bool IsEnum(PTree::Node *&spec);
  
  void Dereference() { --refcount; }
  void Dereference(TypeInfo&);
  void Reference() { ++refcount; }
  void Reference(TypeInfo&);
  bool NthArgument(int, TypeInfo&);
  int NumOfArguments();
  bool NthTemplateArgument(int, TypeInfo&);
  
  PTree::Node *FullTypeName();
  PTree::Node *MakePtree(PTree::Node * = 0);

private:
  static PTree::Node *GetQualifiedName(Environment*, PTree::Node *);
  static PTree::Node *GetQualifiedName2(Class*);
  void Normalize();
  bool resolve_typedef(Environment *&, PTree::Encoding &, bool);
  
  static PTree::Encoding skip_cv(const PTree::Encoding &, Environment*&);
  static PTree::Encoding skip_name(const PTree::Encoding &, Environment*);
  static PTree::Encoding get_return_type(const PTree::Encoding &, Environment*);
  static PTree::Encoding skip_type(const PTree::Encoding &, Environment*);
  
  int refcount;
  PTree::Encoding encode;
  Class* metaobject;
  Environment* env;
};

#endif
