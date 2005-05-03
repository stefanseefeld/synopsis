//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_SymbolLookup_Visitor_hh_
#define Synopsis_SymbolLookup_Visitor_hh_

namespace Synopsis
{
namespace SymbolLookup
{

class Symbol;
class VariableName;
class ConstName;
class TypeName;
class TypedefName;
class ClassName;
class EnumName;
class ClassTemplateName;
class FunctionName;
class FunctionTemplateName;
class NamespaceName;

class Visitor
{
public:
  virtual ~Visitor() {}

  virtual void visit(Symbol const *) = 0;
  virtual void visit(VariableName const *) = 0;
  virtual void visit(ConstName const *) = 0;
  virtual void visit(TypeName const *) = 0;
  virtual void visit(TypedefName const *) = 0;
  virtual void visit(ClassName const *) = 0;
  virtual void visit(EnumName const *) = 0;
  virtual void visit(ClassTemplateName const *) = 0;
  virtual void visit(FunctionName const *) = 0;
  virtual void visit(FunctionTemplateName const *) = 0;
  virtual void visit(NamespaceName const *) = 0;
};

}
}

#endif
