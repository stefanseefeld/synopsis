//
// Copyright (C) 1998 Shaun Flisakowski
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Declaration.hh>
#include <Expression.hh>
#include <Statement.hh>
#include <Token.hh>
#include "Grammar.hh"
#include <File.hh>

#include <cassert>
#include <cstring>
#include <sstream>

std::string TypeQual::to_string() const
{
  std::string retn;
  if (value & Const) retn += "const ";
  if (value & Volatile) retn += "volatile ";
  if (*retn.rbegin() == ' ') retn.erase(retn.end() - 1);
  return retn;
}

std::string Storage::to_string() const
{
  switch (value)
  {
    case Typedef: return "typedef";
    case Auto: return "auto";
    case Register: return "register";
    case Static: return "static";
    case Extern: return "extern";
    default: return "";
  }
}

const BaseType::Spec BaseType::NoType       = 0x00000000;  // no type provided
const BaseType::Spec BaseType::Void         = 0x00000001;  // explicitly no type
const BaseType::Spec BaseType::Bool         = 0x00000002;
const BaseType::Spec BaseType::Char         = 0x00000004;
const BaseType::Spec BaseType::Short        = 0x00000008;
const BaseType::Spec BaseType::Int          = 0x00000010;
const BaseType::Spec BaseType::Int64        = 0x00000020;
const BaseType::Spec BaseType::Int32        = 0x00000040;
const BaseType::Spec BaseType::Int16        = 0x00000080;
const BaseType::Spec BaseType::Int8         = 0x00000100;
const BaseType::Spec BaseType::Long         = 0x00000200;
const BaseType::Spec BaseType::LongLong     = 0x00000400;  // a likely C9X addition
const BaseType::Spec BaseType::Float        = 0x00000800;
const BaseType::Spec BaseType::Double       = 0x00001000;
const BaseType::Spec BaseType::Ellipsis     = 0x00002000;

const BaseType::Spec BaseType::Struct       = 0x00008000;
const BaseType::Spec BaseType::Union        = 0x00010000;
const BaseType::Spec BaseType::Enum         = 0x00020000;
const BaseType::Spec BaseType::UserType     = 0x00040000;
const BaseType::Spec BaseType::BaseMask     = 0x0004FFFF;

const BaseType::Spec BaseType::Signed       = 0x00100000;
const BaseType::Spec BaseType::UnSigned     = 0x00200000;
const BaseType::Spec BaseType::SignMask     = 0x00300000;

const BaseType::Spec BaseType::TypeError    = 0x10000000;

std::string BaseType::to_string(BaseType::Spec typemask)
{
  std::string retn;
  if (typemask & UnSigned) retn = "unsigned ";
  else if (typemask & Signed) retn = "signed ";
  
  if (typemask & Void) retn += "void ";
  else if (typemask & Bool) retn += "_Bool ";
  else if (typemask & Char) retn += "char ";
  else if (typemask & Int8) retn += "__int8 ";
  else if (typemask & Int16) retn += "__int16 ";
  else if (typemask & Int32) retn += "__int32 ";
  else if (typemask & Int64) retn += "__int64 ";
  else if (typemask & Short) retn += "short ";
  else if (typemask & LongLong) retn += "long long ";
  else if (typemask & Float) retn += "float ";
  else if ((typemask & Double) &&
	   (typemask & Long)) retn += "long double ";
  else if (typemask & Double) retn += "double ";
  else if (typemask & Ellipsis) retn += "...";
  else if (typemask & Long) retn += "long ";
  else if (typemask & Struct) retn += "struct ";
  else if (typemask & Union) retn += "union ";
  else if (typemask & Enum) retn += "enum ";
  else if (typemask & UserType) retn += "<user type> ";
  else retn += "int ";        // Default
  if (*retn.rbegin() == ' ') retn.erase(retn.end() - 1);
  return retn;
}

Type::Type(TypeType _type /* =TT_Base */)
{
  type      = _type;
  storage  = Storage::None;

  // Add us into the global list for destruction later.
  link = global_type_list;
  global_type_list = this;
}

Type::~Type()
{
  // assert(false);
}

void
Type::DeleteTypeList(Type* typeList)
{
  Type    *prev = NULL;
  Type    *curr = typeList;

  while (curr != NULL)
  {
    if(prev != NULL) delete prev;
    prev = curr;
    curr = curr->link;
  }

  if(prev != NULL) delete prev;
}

void
Type::printType(std::ostream& out, Symbol *name, bool showBase, int level) const
{
  if (showBase)
  {
    printBase(out,level);
    // FIXME: why do we need a space here ?
    // 'out' was once 'cout' due to a typo, and it appears the regression test
    // output was generated with that, so keep it that way for now.
    if (name != NULL)
      out << " ";
  }
  printBefore(out,name,level);
  printAfter(out);
}

BaseType::BaseType(BaseType::Spec bt /* =NoType */)
  : Type(TT_Base),
    typemask(bt),
    tag(0),
    typeName(0),
    stDefn(0),
    enDefn(0)
{
  qualifier = TypeQual::None;
}

BaseType::BaseType(StructDef *sd)
  : Type(TT_Base),
    typemask(sd->isUnion() ? Union : Struct),
    tag(sd->tag->dup()),
    typeName(0),
    stDefn(sd),
    enDefn(0)
{
  qualifier = TypeQual::None;
}

BaseType::BaseType(EnumDef *ed)
  : Type(TT_Base),
    typemask(Enum),
    tag(ed->tag->dup()),
    typeName(0),
    stDefn(0),
    enDefn(ed)
{
  qualifier = TypeQual::None;
}

BaseType::~BaseType()
{
  delete tag;
  delete typeName;
  delete stDefn;
  delete enDefn;
}

Type*
BaseType::dup0() const
{
  BaseType *ret = new BaseType();

  ret->storage = storage; 
  ret->qualifier = qualifier; 
  ret->typemask = typemask; 

  ret->tag = tag->dup();
  ret->typeName = typeName->dup();
  ret->stDefn = stDefn->dup();
  ret->enDefn = enDefn->dup();
    
  return ret;
}

void
BaseType::printBase(std::ostream& out, int level) const
{
  out << qualifier.to_string();

  if (typemask & UnSigned)
    out << "unsigned ";
  else if (typemask & Signed)
    out << "signed ";
  
  if (typemask & Void)
    out << "void ";
  else if (typemask & Bool)
    out << "_Bool ";
  else if (typemask & Char)
    out << "char ";
  else if (typemask & Int8)
    out << "__int8 ";
  else if (typemask & Int16)
    out << "__int16 ";
  else if (typemask & Int32)
    out << "__int32 ";
  else if (typemask & Int64)
    out << "__int64 ";
  else if (typemask & Short)
    out << "short ";
  else if (typemask & LongLong)
    out << "long long ";
  else if (typemask & Float)
    out << "float ";
  else if ((typemask & Double) && (typemask & Long))
    out << "long double ";
  else if (typemask & Double)
    out << "double ";
  else if (typemask & Ellipsis)
    out << "...";
  else if (typemask & Long)
    out << "long ";
  else if (typemask & Struct)
  {
    if (stDefn != NULL)
    {
      stDefn->print(out, NULL, level);
    }
    else
    {
      out << "struct ";
      
      if (tag)
	out << *tag << " ";
    }
  }
  else if (typemask & Union)
  {
    if (stDefn != NULL)
    {
      stDefn->print(out, NULL, level);
    }
    else
    {
      out << "union ";
      
      if (tag)
	out << *tag << " ";
    }
  }
  else if (typemask & Enum)
  {
    out << "enum ";
    if (enDefn != NULL)
    {
      enDefn->print(out, NULL, level);
    }
    else
    {
      if (tag)
	out << *tag << " ";
    }
  }
  else if (typemask & UserType)
  {
    if (typeName)
      out << *typeName << " ";
  }
  else
  {
    out << "int ";        // Default
  }
}

void
BaseType::printBefore(std::ostream& out, Symbol *name, int level ) const
{
  if (name)
  {
    out << *name;
  }
}

void
BaseType::printAfter( std::ostream& out ) const
{
}

void
BaseType::printForm(std::ostream& out) const
{
  out << "-Base";
  if (qualifier != TypeQual::None) out << ":" << qualifier.to_string();
}

void
BaseType::registerComponents()
{
  if ((typemask & Struct) | (typemask & Union))
  {
    stDefn->registerComponents();
  }
}

bool
BaseType::lookup( Symbol* sym ) const
{
  if ((typemask & Struct)
      || (typemask & Union))
  {
    if (stDefn != NULL)
    {
      return stDefn->lookup(sym);
    }
  }
  else if (typemask & UserType)
  {
    if (typeName)
    {
      SymEntry *typeEntry = typeName->entry;
      
      if (typeEntry)
      {
	return typeEntry->uVarDecl->lookup(sym);
      }
    }
  }
  
  return false;
}

Type*
PtrType::dup0() const
{
  PtrType *ret = new PtrType(qualifier.value);
  
  ret->subType = subType->dup();
  ret->storage = storage; 
    
  return ret;
}

Type*
PtrType::extend(Type* extension)
{
  if (subType) return subType->extend(extension);
        
  subType = extension;      
  return this;
}

void
PtrType::printBase( std::ostream& out, int level) const
{
  if (subType) subType->printBase(out,level);
}

void
PtrType::printBefore( std::ostream& out, Symbol *name, int level ) const
{
  if (subType)
  {
    subType->printBefore(out,NULL,level);
    
    bool paren = ! (subType->isPointer() || subType->isBaseType());
    
    if (paren) out << "(" ;
    
    out << "*" << qualifier.to_string();
    
  }
  
  if (name)
  {
    out << *name;
  }
}

void
PtrType::printAfter( std::ostream& out ) const
{
  if (subType)
  {
    bool paren = ! (subType->isPointer() || subType->isBaseType());
    
    if (paren) out << ")" ;
    
    subType->printAfter(out);
  }
}

void
PtrType::printForm(std::ostream& out) const
{
  out << "-Ptr";
  if (qualifier != TypeQual::None)
  {
    out << ":" << qualifier.to_string();
  }
  if (subType) subType->printForm(out);
}

void
PtrType::findExpr(fnExprCallback cb)
{
  if (subType) subType->findExpr(cb);
}

bool
PtrType::lookup( Symbol* sym ) const
{
  if (subType) return subType->lookup(sym);
  else return false;
}

ArrayType::~ArrayType()
{
  // Handled by deleting the global type list
  // delete subType;
  delete size;
}

Type*
ArrayType::dup0() const
{
  ArrayType *ret  = new ArrayType(size->dup());

  ret->subType = subType->dup();
  ret->storage = storage;
  
  return ret;
}

Type*
ArrayType::extend(Type *extension)
{
  if (subType) return subType->extend(extension);
  subType = extension;
  return this ;
}

void
ArrayType::printBase( std::ostream& out, int level) const
{
  if (subType) subType->printBase(out,level);
}

void
ArrayType::printBefore( std::ostream& out, Symbol *name, int level ) const
{
  if (subType) subType->printBefore(out,name,level);
}

void
ArrayType::printAfter( std::ostream& out ) const
{
  out << "[";
  if (size) size->print(out);
  out << "]";
  
  if (subType) subType->printAfter(out);
}

void
ArrayType::printForm(std::ostream& out) const
{
  out << "-Array[";
  if (size) size->print(out);
  out << "]";
  if (subType) subType->printForm(out);
}

void
ArrayType::findExpr(fnExprCallback cb)
{
  if (subType) subType->findExpr(cb);
  if (size) size->findExpr(cb);
}

bool
ArrayType::lookup( Symbol* sym ) const
{
  if (subType) return subType->lookup(sym);
  else return false;
}

BitFieldType::~BitFieldType()
{
  // Handled by deleting the global type list
  // delete subType;
  delete size;
}

Type *
BitFieldType::extend(Type *extension)
{
  if (subType) return subType->extend(extension);
  
  subType = extension;
  return this;
}

Type*
BitFieldType::dup0() const
{
  BitFieldType *ret = new BitFieldType(size->dup());
  ret->storage = storage; 
    
  ret->subType = subType->dup();
    
  return ret;
}

void
BitFieldType::printBase(std::ostream& out, int level) const
{
  if (subType) subType->printBase(out,level);
}

void
BitFieldType::printBefore( std::ostream& out, Symbol *name, int level ) const
{
  if (subType) subType->printBefore(out, 0, level);

  if (name) out << *name;
  
  out << ":";

  if (size) size->print(out);
}

void
BitFieldType::printAfter( std::ostream& out ) const
{
}

void
BitFieldType::printForm(std::ostream& out) const
{
  out << "-BitField";
  if (subType) subType->printForm(out);
}

void
BitFieldType::findExpr( fnExprCallback cb )
{
  if (size) size->findExpr(cb);
}

bool
BitFieldType::lookup( Symbol* sym ) const
{
  if (subType) return subType->lookup(sym);
  else return false;
}

FunctionType::FunctionType(Decl *args_list /* = NULL */ )
  : Type(TT_Function), KnR_decl(false), nArgs(0), size(0), args(0), subType(0)
{
  addArgs (args_list);
}
        
FunctionType::~FunctionType()
{
  for (int j=0; j < nArgs; j++)
    delete args[j];

  delete [] args;

  // Handled by deleting the global type list
  // delete subType;
}

Type*
FunctionType::dup0() const
{
  FunctionType *ret = new FunctionType();
  ret->storage = storage; 
  ret->size    = size;
  ret->args = new Decl* [size];
  ret->KnR_decl = KnR_decl;

  for (int j=0; j < nArgs; j++)
    ret->addArg(args[j]->dup());
  
  ret->subType = subType->dup();
    
  return ret;
}

Type*
FunctionType::extend(Type *extension)
{
  if (subType) return subType->extend(extension);
    
  subType = extension;
  return this;
}

void
FunctionType::printBase( std::ostream& out, int level) const
{
  if (subType) subType->printBase(out,level);
}

void
FunctionType::printBefore( std::ostream& out, Symbol *name, int level ) const
{
  if (subType) subType->printBefore(out,name,level);
  else if(name) out << *name;
}

void
FunctionType::printAfter( std::ostream& out ) const
{
  if (KnR_decl)
  {
    out << "(";

    if (nArgs > 0)
    {
      out << *(args[0]->name);
      for (int j=1; j < nArgs; j++)
	out << ", " << *(args[j]->name);
    }
    
    out << ")\n";

    for (int j=0; j < nArgs; j++)
    {
      args[j]->print(out,true);
      out << ";\n";
    }
  }
  else
  {
    out << "(";
    
    if (nArgs > 0)
    {
      args[0]->print(out,true);
      for (int j=1; j < nArgs; j++)
      {
	out << ", ";
	args[j]->print(out,true);
      }
    }
    
    out << ")";
  }
  
  if (subType) subType->printAfter(out);
}

void
FunctionType::printForm(std::ostream& out) const
{
  out << "-Function";
  if (subType) subType->printForm(out);
}

void
FunctionType::addArg(Decl *arg)
{
  if (size == nArgs)
  {
    if (size == 0) size = 4;
    else size += size;

    Decl   **oldArgs = args;

    args = new Decl* [size];

    for (int j=0; j < nArgs; j++)
      args[j] = oldArgs[j];

    delete [] oldArgs;
  }

  args[nArgs] = arg;
  nArgs++;
}

void
FunctionType::addArgs(Decl *args)
{
  Decl *arg = args;

  while (args != NULL)
  {
    args = args->next;
    arg->next = NULL;
    addArg(arg);
    arg = args;
  }
}

void
FunctionType::findExpr( fnExprCallback cb )
{
  if (subType) subType->findExpr(cb);
  for (int j=0; j < nArgs; j++)
    args[j]->findExpr(cb);
}

bool
FunctionType::lookup( Symbol* sym ) const
{
  if (subType) return subType->lookup(sym);
  else return false;
}

StructDef::StructDef( bool _is_union /* =false */ )
{
  _isUnion = _is_union;
  tag = NULL;
  size = 0;
  nComponents = 0;

  components = NULL;
}

StructDef::~StructDef()
{
  delete tag;
  
  for (int j=0; j < nComponents; j++)
    delete components[j];

  delete [] components;
}

Decl*
StructDef::lastDecl()
{
  return components[nComponents-1];
}

StructDef*
StructDef::dup() const
{
  StructDef *ret = this ? dup0() : NULL;
  return ret;
}

StructDef*
StructDef::dup0() const
{
  StructDef *ret = new StructDef();
  ret->size    = size;
  ret->_isUnion = _isUnion;
  ret->components = new Decl* [size];

  for (int j=0; j < nComponents; j++)
    ret->addComponent(components[j]->dup());

  ret->tag = tag->dup();
  
  return ret;
}

void
StructDef::print(std::ostream& out, Symbol *name, int level) const
{
  if (isUnion()) out << "union ";
  else out << "struct ";

  if (tag) out << *tag << " ";

  out << "{\n"; 

  for (int j=0; j < nComponents; j++)
  {
    indent(out,level+1);
    components[j]->print(out,true,level+1);

    Decl *decl = components[j]->next;
    while (decl != NULL)
    {
      out << ", ";
      decl->print(out,false,level+1);
      decl = decl->next;
    }

    out << ";\n";
  }

  indent(out,level);
  out << "}"; 

  if (name) out << " " << *name;
}

void
StructDef::findExpr( fnExprCallback cb )
{
  for (int j=0; j < nComponents; j++)
    components[j]->findExpr(cb);
}

void
StructDef::addComponent(Decl *comp)
{
  if (size == nComponents)
  {
    if (size == 0) size = 4;
    else size += size;

    Decl **oldComps = components;

    components = new Decl* [size];

    for (int j=0; j < nComponents; j++)
      components[j] = oldComps[j];

    delete [] oldComps;
  }

  components[nComponents] = comp;
  nComponents++;

  do
  {
    // Hook this component's symtable entry back to here.
    if ((comp->name != NULL) && (comp->name->entry != NULL))
    {
      comp->name->entry->uComponent = comp;
      comp->name->entry->u2Container = this;
      // The entry was inserted by gram.y as a ComponentEntry.
      assert (comp->name->entry->IsComponentDecl());
    }
    comp = comp->next;
  } while (comp);
}

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
void
StructDef::registerComponents()
{
  for (int j = 0; j < nComponents; j++)
  {
    Decl *decl = components[j];
    while (decl != NULL)
    {
      Symbol *ident = decl->name;
      if (ident != NULL)
      {
	ident->entry = global_parse_env->transUnit->my_contxt.syms
	  ->Insert(mk_component(ident->name,decl,this));
      }

      // Register any sub-components also.
      decl->form->registerComponents();

      decl = decl->next;
    }
  }
}

bool StructDef::lookup( Symbol* sym ) const
{
  for (int j = 0; j < nComponents; j++)
  {
    Decl *decl = components[j];
    while (decl)
    {
      Symbol *ident = decl->name;
      if (ident && ident->name == sym->name)
      {
	sym->entry = ident->entry;
	return true;
      }   
      decl = decl->next;
    }
  }
  return false;
}

EnumDef::EnumDef()
  : tag(0),
    size(0),
    nElements(0),
    names(0),
    values(0)
{
}

EnumDef::~EnumDef()
{
  delete tag;

  for (int j=0; j < nElements; j++)
  {
    delete names[j];
    delete values[j];
  }

  delete [] names;
  delete [] values;
}

void
EnumDef::addElement(Symbol *nme, Expression *val /* =NULL */ )
{
  if (size == nElements)
  {
    if (size == 0) size = 4;
    else size += size;

    Symbol     **oldNames = names;
    Expression **oldVals = values;

    names  = new Symbol* [size];
    values = new Expression* [size];
    
    for (int j=0; j < nElements; j++)
    {
      names[j]  = oldNames[j];
      values[j] = oldVals[j];
    }

    delete [] oldNames;
    delete [] oldVals;
  }

  names[nElements]  = nme;
  values[nElements] = val;
  nElements++;
  
  if ((nme->entry != NULL) && (nme->entry->type == EnumConstEntry))
  {
    assert(nme->entry->type == EnumConstEntry);
    nme->entry->uEnumValue = val;
    nme->entry->u2EnumDef = this;
  }
}

void
EnumDef::addElement( EnumConstant* ec )
{
  addElement(ec->name, ec->value);
  
  ec->name = NULL;
  ec->value = NULL;

  delete ec;
}

EnumDef*
EnumDef::dup() const
{
  EnumDef *ret = this ? dup0() : NULL;
  return ret;
}

EnumDef*
EnumDef::dup0() const
{
  EnumDef *ret = new EnumDef();
  ret->size  = size;
  ret->names = new Symbol* [size];
  ret->values = new Expression* [size];

  for (int j=0; j < nElements; j++)
  {
    Expression *val = values[j]->dup();
    ret->addElement(names[j]->dup(),val);
  }

  ret->tag = tag->dup();
    
  return ret;
}

void
EnumDef::print(std::ostream& out, Symbol*, int level) const
{
  if (tag) out << *tag << " ";

  out << "{ "; 

  if (nElements > 0)
  {
    out << *names[0];
    if (values[0])
    {
      out << " = ";
      values[0]->print(out);
    }

    for (int j=1; j < nElements; j++)
    {
      out << ", " << *names[j];

      if (values[j])
      {
	out << " = ";
	values[j]->print(out);
      }
    }
  }

  out << " }"; 
}

void
EnumDef::findExpr( fnExprCallback cb )
{
  for (int j=0; j < nElements; j++)
  {
    if (values[j] != NULL)
      values[j]->findExpr(cb);
  }
}

GccAttrib::GccAttrib(GccAttrib::Type t) : type(t), value(0), mode(0) {}
GccAttrib::~GccAttrib() { delete mode;}

std::string GccAttrib::to_string() const
{
  std::string attr;
  switch (type)
  {
    case Aligned:
    {
      std::ostringstream oss;
      oss << "aligned (" << value << ")";
      attr = oss.str();
      break;
    }
    case Packed:
      attr = "packed";
      break;

    case CDecl:
      attr = "__cdecl__";
      break;

    case Mode:
      attr = "__mode__ (" + mode->name + ")";
      break;
   
    case Format:
    {
      std::ostringstream oss;
      oss << "format (" << *mode << "," << strIdx << "," << first << ")";
      attr = oss.str();
      break;
    }
    case Const:
      attr = "__const__";
      break;

    case TransparentUnion:
      attr = "__transparent_union__";
      break;

    case Pure:
      attr = "__pure__";
      break;

    case NoReturn:
      attr = "__noreturn__";
      break;

    case Malloc:
      attr = "__malloc__";
      break;

    case Unsupported:
  default:
    attr = "<unsupported gcc attribute>";
    break;
  }
}

GccAttrib*
GccAttrib::dup() const
{
  GccAttrib *ret = this ? dup0() : 0;
  return ret;
}

GccAttrib*
GccAttrib::dup0() const
{
  GccAttrib    *ret = new GccAttrib(type);
  ret->value = value;
  ret->mode = mode->dup();
  return ret;
}

Decl::Decl( Symbol* sym /* =NULL */ )
{
  clear();
  name = sym;
}

Decl::Decl( Type* type )
{
  clear();

  form = type;
  storage = type->storage;
}

Decl::~Decl()
{
  // Handled by deleting the global type list
  // delete form;
  delete attrib;
  delete initializer;
}

void
Decl::clear()
{
  storage     = Storage::None;

  name        = NULL;
  form        = NULL;
  attrib      = NULL;
  initializer = NULL;
  next        = NULL;
}

Type*
Decl::extend( Type* type )
{
  if (storage == Storage::None)
    storage = type->storage;
    
  if (form != NULL)
    return form->extend(type);
  
  form = type;
  return NULL;
}

Decl*
Decl::dup() const
{
  Decl *ret = this ? dup0() : NULL;
  return ret;
}

Decl*
Decl::dup0() const
{
  Decl *ret = new Decl();
  ret->storage    = storage;
  ret->form = form;
  
  ret->name     = name->dup();
  ret->attrib = attrib->dup();
  ret->initializer = initializer->dup();
  ret->next = next->dup(); 
  
  return ret;
}

void
Decl::copy(const Decl& decl)
{
  storage     = decl.storage;
  name        = decl.name;
  form        = decl.form;
  attrib      = decl.attrib;
  initializer = decl.initializer;
}

void
Decl::print(std::ostream& out, bool showBase, int level) const
{
  assert(this != NULL);

  if (showBase)
  {
    std::string st = storage.to_string();
    if (!st.empty()) out << st << ' ';
    
    // Hack to fix K&R non-declarations
    if (form == NULL)
      out << "int ";
  }

  if (form) form->printType(out,name,showBase,level);
  else if (name) out << *name;

  if (attrib) out << " __attribute__ ((" << attrib->to_string() << "))";

  if (initializer)
  {
    out << " = ";
    initializer->print(out);
  }

  /*
  if (!form->isBaseType())
  {
    out << "  [FORM: ";
    form->printForm(out);
    out << " ]";
  }
  */
}

void
Decl::findExpr( fnExprCallback cb )
{
  if (form) form->findExpr(cb);

  if (initializer) initializer->findExpr(cb);
}

bool
Decl::lookup( Symbol* sym ) const
{
  if (form) return form->lookup(sym);
  else return false;
}

Decl*
ReverseList(Decl *dList)
{
  Decl*    head = NULL;

  while (dList != NULL)
  {
    Decl*    dcl = dList;
    
    dList = dList->next;

    dcl->next = head;
    head = dcl;
  }

  return head; 
}
