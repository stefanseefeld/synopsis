//
// Copyright (C) 1997 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef Synopsis_SymbolLookup_Type_hh_
#define Synopsis_SymbolLookup_Type_hh_

#include <Synopsis/PTree.hh>
#include <Synopsis/SymbolLookup/Scope.hh>

namespace Synopsis
{
namespace SymbolLookup
{

//. Type interprets an encoded type name. For details of the encoded
//. type name, see class PTree::Encoding.
class Type
{
public:
  enum Kind 
  {
    UNDEF,
    BUILTIN, CLASS, ENUM, TEMPLATE,
    POINTER, REFERENCE, POINTER_TO_MEMBER,
    ARRAY, FUNCTION
  };

  enum BuiltinType
  {
    CHAR = 1, INT = 2, SHORT = 4, LONG = 8,
    LONG_LONG = 16, SIGNED = 32, UNSIGNED = 64, FLOAT = 128,
    DOUBLE = 256, LONG_DOUBLE = 512, VOID = 1024,
    BOOLEAN = 2048,
    WIDE_CHAR = 4096
  };

  Type();
  Type(PTree::Encoding const &, Scope const * = 0);
  void unknown();
  void set(PTree::Encoding const &, Scope const * = 0);
  void set_void();
  void set_int();
  void set_member(PTree::Node *);

  Kind id();

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
  bool is_class();
  bool is_template_class();
  bool is_enum();
  bool is_enum(PTree::Node *&spec);
  
  void dereference() { --my_refcount;}
  void reference() { ++my_refcount;}

  int num_of_arguments();
  bool nth_argument(int, Type&);
  bool nth_template_argument(int, Type&);
  
  PTree::Node *full_type_name();
  PTree::Node *make_ptree(PTree::Node * = 0);

  friend std::ostream &operator << (std::ostream &, Type const &);
private:
  static PTree::Node *get_qualified_name(Scope const *, PTree::Node *);
  void normalize();
  bool resolve_typedef(Scope const *&, PTree::Encoding &, bool);
  
  static PTree::Encoding skip_cv(PTree::Encoding const &, Scope const *&);
  static PTree::Encoding skip_name(PTree::Encoding const &, Scope const *);
  static PTree::Encoding get_return_type(PTree::Encoding const &, Scope const *);
  static PTree::Encoding skip_type(PTree::Encoding const &, Scope const*);
  
  short           my_refcount;
  PTree::Encoding my_encoding;
  Scope const *   my_scope;
};

}
}

#endif
