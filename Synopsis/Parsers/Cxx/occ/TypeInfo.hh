//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _TypeInfo_hh
#define _TypeInfo_hh

#include <Synopsis/PTree.hh>

using namespace Synopsis;

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
class TypeInfo : public PTree::LightObject 
{
public:
  TypeInfo();
  void unknown();
  void set(const PTree::Encoding &, Environment*);
  void set(Class*);
  void set_void();
  void set_int();
  void set_member(PTree::Node *);

  TypeInfoId id();

  bool is_no_return_type();

  bool is_const();
  bool is_volatile();
  
  size_t is_builtin_type();
  bool is_function();
  bool is_ellipsis();
  bool is_pointer_type();
  bool is_reference_type();
  bool is_array();
  bool is_pointer_to_member();
  bool is_template_class();
  Class *class_metaobject();
  bool is_class(Class*&);
  bool is_enum();
  bool is_enum(PTree::Node *&spec);
  
  void dereference() { --my_refcount;}
  void dereference(TypeInfo&);
  void reference() { ++my_refcount;}
  void reference(TypeInfo&);
  bool nth_argument(int, TypeInfo&);
  int num_of_arguments();
  bool nth_template_argument(int, TypeInfo&);
  
  PTree::Node *full_type_name();
  PTree::Node *make_ptree(PTree::Node * = 0);

private:
  static PTree::Node *get_qualified_name(Environment*, PTree::Node *);
  static PTree::Node *get_qualified_name2(Class*);
  void normalize();
  bool resolve_typedef(Environment *&, PTree::Encoding &, bool);
  
  static PTree::Encoding skip_cv(const PTree::Encoding &, Environment*&);
  static PTree::Encoding skip_name(const PTree::Encoding &, Environment*);
  static PTree::Encoding get_return_type(const PTree::Encoding &, Environment*);
  static PTree::Encoding skip_type(const PTree::Encoding &, Environment*);
  
  size_t          my_refcount;
  PTree::Encoding my_encoding;
  Class          *my_metaobject;
  Environment    *my_env;
};

#endif
