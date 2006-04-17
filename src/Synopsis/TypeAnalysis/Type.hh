//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_TypeAnalysis_Type_hh_
#define Synopsis_TypeAnalysis_Type_hh_

#include <Synopsis/TypeAnalysis/Visitor.hh>
#include <string>
#include <vector>

namespace Synopsis
{
namespace TypeAnalysis
{

//. Base class for synopsis' type system.
class Type
{
public:
  Type() : my_refcounter(1) {}
  virtual ~Type() {}
  virtual void accept(Visitor *visitor) = 0;
  virtual void ref() const { ++my_refcounter;}
  virtual void deref() const { if (--my_refcounter) delete this;}
  // TODO: add 'new' / 'delete' operators for optimization.

private:
  mutable size_t my_refcounter;
};

//. Atomic types are declared types that can not be decomposed
//. in terms of qualifiers, pointers, references, etc. If each
//. type is represented by a graph of type nodes, these are leave
//. nodes.
class AtomicType : public Type
{
public:
  AtomicType(std::string const &name) : my_name(name) {}
  std::string const &name() const { return my_name;}

private:
  const std::string my_name;
};

class BuiltinType : public AtomicType
{
public:
  BuiltinType(std::string const &name) : AtomicType(name) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  // BuiltinType is preallocated and thus is destructed at program termination.
  virtual void ref() const {}
  virtual void deref() const {}
};

extern BuiltinType BOOL;
extern BuiltinType CHAR;
extern BuiltinType WCHAR;
extern BuiltinType SHORT;
extern BuiltinType INT;
extern BuiltinType LONG;
extern BuiltinType LONGLONG;
extern BuiltinType FLOAT;
extern BuiltinType DOUBLE;
extern BuiltinType UCHAR;
extern BuiltinType USHORT;
extern BuiltinType UINT;
extern BuiltinType ULONG;
extern BuiltinType SCHAR;
extern BuiltinType SSHORT;
extern BuiltinType SINT;
extern BuiltinType SLONG;
extern BuiltinType VOID;

class Enum : public AtomicType
{
public:
  Enum(std::string const &name) : AtomicType(name) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class Compound : public AtomicType
{
public:
  Compound(std::string const &name) : AtomicType(name) {}
};

class Class : public Compound
{
public:
  enum Kind { STRUCT, CLASS};

  Class(Kind kind, std::string const &name) : Compound(name), my_kind(kind) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Kind kind() const { return my_kind;}

private:
  Kind my_kind;
};

class Union : public Compound
{
public:
  Union(std::string const &name) : Compound(name) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

private:
};

//. A template type parameter.
class Parameter : public AtomicType
{
public:
  Parameter(std::string const &name) : AtomicType(name) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

//. A dependent type, such as in
//. 'typename T::value_type'
class Dependent : public AtomicType
{
public:
  Dependent(std::string const &name) : AtomicType(name) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

//. A cv qualifier
class CVType : public Type
{
public:
  enum Qualifier { NONE=0x0, CONST=0x1, VOLATILE=0x2, CV = 0x3};

  CVType(Type const *type, Qualifier q) : my_type(type), my_qual(q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Type const *unqualified() const { return my_type;}
  Qualifier qualifier() const { return my_qual;}

private:
  static std::string const names[4];

  Type const *my_type;
  Qualifier   my_qual;
};

class Pointer : public Type
{
public:
  Pointer(Type const *type) : my_type(type) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Type const *dereference() const { return my_type;}

private:
  Type const *my_type;
};

class Reference : public Type
{
public:
  Reference(Type const *type) : my_type(type) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Type const *dereference() const { return my_type;}

private:
  Type const *my_type;
};

class Array : public Type
{
public:
  Array(Type const *type, unsigned long dim) : my_type(type), my_dim(dim) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Type const *dereference() const { return my_type;}
  unsigned long dim() const { return my_dim;}

private:
  Type const *  my_type;
  unsigned long my_dim;
};

class Function : public Type
{
public:
  typedef std::vector<Type const *> ParameterList;

  Function(ParameterList const &p, Type const *r)
    : my_params(p), my_return_type(r) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Type const *return_type() const { return my_return_type;}
  ParameterList const &params() const { return my_params;}
private:
  ParameterList my_params;
  Type const *  my_return_type;
};

class PointerToMember : public Type
{
public:
  PointerToMember() {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

private:
  Type const *my_container;
  Type const *my_member;
};

}
}

#endif
