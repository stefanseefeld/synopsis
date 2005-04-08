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
#include <ostream>
#include <iterator>

namespace Synopsis
{
namespace TypeAnalysis
{

class Type
{
public:
  virtual ~Type() {}
  virtual void accept(Visitor *visitor) = 0;
};

class BuiltinType : public Type
{
public:
  BuiltinType(std::string const &name) : my_name(name) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

private:
  std::string my_name;
};

class Enum : public Type
{
public:
  Enum(std::string const &name) : my_name(name) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

private:
  std::string my_name;
};

class Compound : public Type
{
public:
  Compound(std::string const &name) : my_name(name) {}

protected:
  std::string my_name;
};

class Class : public Compound
{
public:
  enum Kind { STRUCT, CLASS};

  Class(Kind kind, std::string const &name) : Compound(name), my_kind(kind) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

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

class CVType : public Type
{
public:
  enum CVQualifier { NONE=0x0, CONST=0x1, VOLATILE=0x2};

  CVType(Type const *type, CVQualifier q) : my_type(type), my_qual(q) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

private:
  Type const *my_type;
  CVQualifier my_qual;
};

class Pointer : public Type
{
public:
  Pointer(Type const *type) : my_type(type) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

private:
  Type const *my_type;
};

class Reference : public Type
{
public:
  Reference(Type const *type) : my_type(type) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

private:
  Type const *my_type;
};

class Array : public Type
{
public:
  Array(Type const *type) : my_type(type) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

private:
  Type const *my_type;
};

class Function : public Type
{
public:
  Function() {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

private:
  Type const *my_type;
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
