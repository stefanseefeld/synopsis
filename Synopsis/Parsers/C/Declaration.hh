//
// Copyright (C) 1998 Shaun Flisakowski
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef _Declaration_hh
#define _Declaration_hh

#include <Symbol.hh>
#include <Callback.hh>
#include <Location.hh>
#include <Dup.hh>
#include <Visitor.hh>

#include <iostream>
#include <vector>
#include <cassert>

class Constant;
class Expression;
class EnumConstant;

struct TypeQual
{
  enum { None = 0x0000, Const = 0x0001, Volatile = 0x0002};
  TypeQual &operator = (short v) { value = v;}
  bool operator == (short v) const { return value == v;}
  bool operator != (short v) const { return value != v;}
  TypeQual &operator |= (const TypeQual &v) { value |= v.value; return *this;}
  bool operator & (const TypeQual &v) const { return value & v.value;}
  bool operator | (const TypeQual &v) const { return value | v.value;}
  std::string to_string() const;
  short value;  
};

struct Storage
{
  enum {None = 0, Auto, Extern, Register, Static, Typedef};
  // can't define constructor since Storage is part of a union.
  // always initialize explicitely !
  Storage &operator = (short v) { value = v;}
  bool operator == (short v) const { return value == v;}
  bool operator != (short v) const { return value != v;}
  std::string to_string() const;
  short value;
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
enum TypeKind
{
    TK_Base = 0,
    TK_TypeDef,
    TK_UserType
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
enum TypeType
{
    TT_Base,        // a simple base type, T
    TT_Pointer,     // pointer to T
    TT_Array,       // an array of T
    TT_BitField,    // a bitfield
    TT_Function     // <args> -> <result>

    /*    These are now considered TT_Base
    TT_Struct,     
    TT_Union,
    TT_Enum
    */
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o

class Decl;

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
class StructDef
{
  public:
    StructDef( bool _is_union = false );
    ~StructDef();

    bool    isUnion() const { return _isUnion; }

    StructDef*   dup0() const;
    StructDef*   dup() const;    // deep-copy

    void    print(std::ostream& out, Symbol *name, int level) const;

    void    findExpr( fnExprCallback cb );

    void    addComponent(Decl *comp);

    void    registerComponents();

    // Lookup this symbol in this struct/union and set its entry
    // if its a component of it.
    bool    lookup( Symbol* sym ) const;

    bool            _isUnion;
    Symbol         *tag;

    int             size;          // size of the array.
    int             nComponents;
    Decl          **components;

  private:
    Decl   *lastDecl();
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
class EnumDef
{
  public:
    EnumDef();
    ~EnumDef();

    EnumDef*   dup0() const;
    EnumDef*   dup() const;    // deep-copy

    void print(std::ostream& out, Symbol *name, int level) const;
    void findExpr( fnExprCallback cb );

    void addElement( Symbol *nme, Expression *val = NULL );

    void addElement( EnumConstant* ec );

    Symbol         *tag;

    int             size;       // size of the arrays.
    int             nElements;
    Symbol        **names;
    Expression    **values;
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o

class Type : public Dup<Type>
{
  public:
    Type(TypeType _type=TT_Base);
    virtual ~Type();

    virtual void accept(TypeVisitor *) = 0;

    virtual int     precedence() const { return 16; }
    virtual Type*   dup0() const =0;    // deep-copy

    virtual Type*   extend(Type *extension)=0;

    // This function handles the complexity of printing a type.
    void    printType( std::ostream& out, Symbol *name,
                       bool showBase, int level ) const;

    virtual void printBase( std::ostream& out, int level ) const {}
    virtual void printBefore( std::ostream& out, Symbol *name, int level) const {}
    virtual void printAfter( std::ostream& out ) const {}

    virtual void printForm(std::ostream& out) const { out << "-[ABT]"; }

    virtual void registerComponents() {}

    virtual void findExpr( fnExprCallback cb ) {}

    virtual bool lookup( Symbol* sym ) const { return false; }

    bool    isBaseType() const { return (type == TT_Base); }
    bool    isPointer() const { return (type == TT_Pointer); }
    bool    isFunction() const { return (type == TT_Function); }
    bool    isArray() const { return (type == TT_Array); }

    // Delete all types stored in this list.
    static void    DeleteTypeList(Type* typelist);


    TypeType type;

    // Temporary - is moved into the declaration (Decl).
    Storage  storage;

  private:
    Type*    link;    // For linking all type classes togather.
};

class BaseType : public Type
{
public:
  typedef unsigned long Spec;

  static const Spec NoType;  // no type provided
  static const Spec Void;    // explicitly no type
  static const Spec Bool;
  static const Spec Char;
  static const Spec Short;
  static const Spec Int;
  static const Spec Int64;
  static const Spec Int32;
  static const Spec Int16;
  static const Spec Int8;
  static const Spec Long;
  static const Spec LongLong;// a likely C9X addition
  static const Spec Float;
  static const Spec Double;
  static const Spec Ellipsis;
  
  static const Spec Struct;
  static const Spec Union;
  static const Spec Enum;
  static const Spec UserType;
  static const Spec BaseMask;
  
  // Sign indicator
  static const Spec Signed;
  static const Spec UnSigned;
  static const Spec SignMask;
  
  static const Spec TypeError;

  static std::string to_string(Spec);


  BaseType(Spec = NoType);
  BaseType(StructDef *sd);
  BaseType(EnumDef *ed);
  ~BaseType();

  virtual void accept(TypeVisitor *v) { v->traverse_base(this);}

  Type* dup0() const;    // deep-copy

  Type* extend(Type *extension) { assert(0); return NULL; }

  void printBase( std::ostream& out, int level ) const;
  void printBefore( std::ostream& out, Symbol *name, int level) const;
  void printAfter( std::ostream& out ) const;

  void printForm(std::ostream& out) const;

  void registerComponents();

  bool lookup( Symbol* sym ) const;

  Spec       typemask;

  TypeQual   qualifier;

  Symbol    *tag;        // tag for struct/union/enum
  Symbol    *typeName;   // typedef name for a UserType

  StructDef *stDefn;     // optional definition of struct/union
  EnumDef   *enDefn;     // optional definition of enum 
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
class PtrType : public Type
{
  public:
    PtrType(short qual) : Type(TT_Pointer), subType(0) { qualifier = qual;};
    ~PtrType(){};

    virtual void accept(TypeVisitor *v) { v->traverse_ptr(this);}

    int    precedence() const { return 15; }

    Type* dup0() const;    // deep-copy

    Type* extend(Type *extension);

    void printBase( std::ostream& out, int level ) const;
    void printBefore( std::ostream& out, Symbol *name, int level) const;
    void printAfter( std::ostream& out ) const;

    void printForm(std::ostream& out) const;

    void registerComponents() { if (subType) subType->registerComponents(); }

    void findExpr( fnExprCallback cb );

    bool lookup( Symbol* sym ) const;

    TypeQual        qualifier;
    Type           *subType;
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
class ArrayType : public Type
{
  public:
    ArrayType(Expression *s)
          : Type(TT_Array), subType(NULL), size(s) {};

    ~ArrayType();

    virtual void accept(TypeVisitor *v) { v->traverse_array(this);}

    Type* dup0() const;    // deep-copy

    Type* extend(Type *extension);

    void printBase( std::ostream& out, int level ) const;
    void printBefore( std::ostream& out, Symbol *name, int level) const;
    void printAfter( std::ostream& out ) const;

    void printForm(std::ostream& out) const;

    void registerComponents() { if (subType) subType->registerComponents(); }

    void findExpr( fnExprCallback cb );

    bool lookup( Symbol* sym ) const;

    Type           *subType;

    Expression     *size;
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
class BitFieldType : public Type
{
  public:
    BitFieldType(Expression * s = NULL)
          : Type(TT_BitField),size(s),subType(NULL) {};
    ~BitFieldType();

    virtual void accept(TypeVisitor *v) { v->traverse_bit_field(this);}

    Type* dup0() const;    // deep-copy

    Type* extend(Type *extension);

    void printBase( std::ostream& out, int level ) const;
    void printBefore( std::ostream& out, Symbol *name, int level) const;
    void printAfter( std::ostream& out ) const;

    void printForm(std::ostream& out) const;

    void registerComponents() { if (subType) subType->registerComponents(); }

    void findExpr( fnExprCallback cb );

    bool lookup( Symbol* sym ) const;

    Expression     *size;

    Type           *subType;
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
class FunctionType : public Type
{
  public:
    FunctionType(Decl *args_list = NULL);
    ~FunctionType();

    virtual void accept(TypeVisitor *v) { v->traverse_function(this);}

    Type* dup0() const;    // deep-copy

    Type* extend(Type *extension);

    void printBase( std::ostream& out, int level ) const;
    void printBefore( std::ostream& out, Symbol *name, int level) const;
    void printAfter( std::ostream& out ) const;

    void printForm(std::ostream& out) const;

    void registerComponents() { if (subType) subType->registerComponents(); }

    void addArg(Decl *arg);
    void addArgs(Decl *args);

    void findExpr( fnExprCallback cb );

    bool lookup( Symbol* sym ) const;

    bool            KnR_decl;    // old-style function declaration?
    int             nArgs;
    int             size;
    Decl          **args;

    Type           *subType;    // The return type
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
class GccAttrib
{
public:
  enum Type
  {
    Unsupported = 0,
    Aligned, Packed, CDecl, Mode, Format,
   
    Const, NoReturn, Malloc, TransparentUnion, Pure
  };


  GccAttrib(Type);
  ~GccAttrib();

  GccAttrib* dup0() const;
  GccAttrib* dup() const;

  std::string to_string() const;

  Type    type;

  uint    value;    // For use with Aligned
  Symbol *mode;     // For use with Mode, Format

  uint    strIdx;   // For use with Format
  uint    first;    // For use with Format
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
class Decl
{
public:
  Decl(Symbol *sym = 0);
  Decl(Type *type);
  ~Decl();

  Type*   extend( Type* type );

  bool    isTypedef() const { return storage == Storage::Typedef; }
  bool    isStatic() const { return storage == Storage::Static; }

  void    clear();

  Decl*   dup0() const;
  Decl*   dup() const;        // deep-copy

  void    copy(const Decl& decl);    // shallow copy

  void    print(std::ostream& out, bool showBase, int level=0) const;
  void    printBase(std::ostream& out, Symbol *name,
		    bool showBase, int level) const;

  void    findExpr( fnExprCallback cb );

  bool lookup( Symbol* sym ) const;

  Storage     storage;

  Type       *form;    // i.e., int *x[5] 

  Symbol     *name;    // The symbol being declared.

  GccAttrib  *attrib;  // optional gcc attribute

  Expression *initializer;

  Decl       *next;    // For linking into lists
};

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o

typedef    std::vector<Decl*>    DeclVector;

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o

Decl*	ReverseList( Decl* dList );

// o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o

#endif  /* DECL_H */

