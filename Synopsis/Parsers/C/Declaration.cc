
/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o

    CTool Library
    Copyright (C) 1998-2001	Shaun Flisakowski

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
    o+
    o+     File:         decl.cpp
    o+
    o+     Programmer:   Shaun Flisakowski
    o+     Date:         Aug 9, 1998
    o+
    o+     A high-level view of declarations.
    o+
    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

#include <cassert>
#include <cstring>

#include <Declaration.hh>
#include <Expression.hh>
#include <Statement.hh>
#include <Token.hh>
#include "Grammar.hh"
#include <Project.hh>

static void
printStorage( std::ostream& out, StorageType storage)
{
  switch (storage)
  {
    case ST_None:
      break;
      
    case ST_Typedef:
      out << "typedef ";
      break;

    case ST_Auto:
      out << "auto ";
      break;

    case ST_Register:
      out << "register ";
      break;

    case ST_Static:
      out << "static ";
      break;

    case ST_Extern:
      out << "extern ";
      break;
  }
}

static void
printQual(std::ostream& out, TypeQual qualifier)
{
  if (qualifier & TQ_Const)
    out << "const ";

  if (qualifier & TQ_Volatile)
    out << "volatile ";
}

Type::Type(TypeType _type /* =TT_Base */)
{
  type      = _type;
  storage  = ST_None;

  // Add us into the global list for destruction later.
  link = gProject->typeList;
  gProject->typeList = this;
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

    if (name != NULL)
      out << " ";
  }

  printBefore(out,name,level);
  printAfter(out);
}

BaseType::BaseType(BaseTypeSpec bt /* =BT_NoType */)
  : Type(TT_Base)
{
  typemask = bt;
  qualifier = TQ_None;

  tag  = NULL;
  typeName = NULL;
  stDefn = NULL;
  enDefn = NULL;
}

BaseType::BaseType( StructDef *sd )
  : Type(TT_Base)
{
  typemask = (sd->isUnion()) ? BT_Union : BT_Struct;
  qualifier = TQ_None;

  tag  = sd->tag->dup();
    
  typeName = NULL;
  stDefn = sd;
  enDefn = NULL;
}

BaseType::BaseType( EnumDef *ed )
  : Type(TT_Base)
{
  typemask = BT_Enum;
  qualifier = TQ_None;

  tag  = ed->tag->dup();
  typeName = NULL;
  stDefn = NULL;
  enDefn = ed;
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
  printQual(out,qualifier);

  if (typemask & BT_UnSigned)
    out << "unsigned ";
  else if (typemask & BT_Signed)
    out << "signed ";
  
  if (typemask & BT_Void)
    out << "void ";
  else if (typemask & BT_Bool)
    out << "_Bool ";
  else if (typemask & BT_Char)
    out << "char ";
  else if (typemask & BT_Int8)
    out << "__int8 ";
  else if (typemask & BT_Int16)
    out << "__int16 ";
  else if (typemask & BT_Int32)
    out << "__int32 ";
  else if (typemask & BT_Int64)
    out << "__int64 ";
  else if (typemask & BT_Short)
    out << "short ";
  else if (typemask & BT_LongLong)
    out << "long long ";
  else if (typemask & BT_Float)
    out << "float ";
  else if ((typemask & BT_Double) && (typemask & BT_Long))
    out << "long double ";
  else if (typemask & BT_Double)
    out << "double ";
  else if (typemask & BT_Ellipsis)
    out << "...";
  else if (typemask & BT_Long)
    out << "long ";
  else if (typemask & BT_Struct)
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
  else if (typemask & BT_Union)
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
  else if (typemask & BT_Enum)
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
  else if (typemask & BT_UserType)
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
  if (qualifier != TQ_None)
  {
    out << ":";
    printQual(out,qualifier);
  }
}

void
BaseType::registerComponents()
{
  if ((typemask & BT_Struct) | (typemask & BT_Union))
  {
    stDefn->registerComponents();
  }
}

bool
BaseType::lookup( Symbol* sym ) const
{
  if ((typemask & BT_Struct)
      || (typemask & BT_Union))
  {
    if (stDefn != NULL)
    {
      return stDefn->lookup(sym);
    }
  }
  else if (typemask & BT_UserType)
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
  PtrType *ret = new PtrType(qualifier);
  
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
    
    out << "*" ;
    printQual(out,qualifier);
    
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
  if (qualifier != TQ_None)
  {
    out << ":";
    printQual(out,qualifier);
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
	ident->entry = gProject->Parse_TOS->transUnit->contxt.syms
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
{
  tag = NULL;
  size = 0;
  nElements = 0;

  names = NULL;
  values = NULL;
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

GccAttrib::GccAttrib( GccAttribType gccType )
{
  type = gccType;
  value = 0;
  mode = NULL;
}

GccAttrib::~GccAttrib()
{
  delete mode;
}

void
GccAttrib::print( std::ostream& out ) const
{
  out << " __attribute__ ((";

  switch (type)
  {
    case GCC_Aligned:
      out << "aligned (" << value << ")";
      break;

    case GCC_Packed:
      out << "packed";
      break;

    case GCC_CDecl:
      out << "__cdecl__";
      break;

    case GCC_Mode:
      out << "__mode__ (" << *mode << ")";
      break;
   
    case GCC_Format:
      out << "format (" << *mode << "," << strIdx << "," << first << ")";
      break;

    case GCC_Const:
      out << "__const__";
      break;

    case GCC_NoReturn:
      out << "__noreturn__";
      break;

    case GCC_Malloc:
      out << "__malloc__";
      break;

    case GCC_Unsupported:
  default:
    out << "<unsupported gcc attribute>";
    break;
  }

  out << "))";
}

GccAttrib*
GccAttrib::dup() const
{
  GccAttrib *ret = this ? dup0() : NULL;
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
  storage    = ST_None;

  name        = NULL;
  form        = NULL;
  attrib      = NULL;
  initializer = NULL;
  next        = NULL;
}

Type*
Decl::extend( Type* type )
{
  if (storage == ST_None)
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
    printStorage(out,storage);
    
    // Hack to fix K&R non-declarations
    if (form == NULL)
      out << "int ";
    }

    if (form) form->printType(out,name,showBase,level);
    else if (name) out << *name;

    if (attrib) attrib->print(out);

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
