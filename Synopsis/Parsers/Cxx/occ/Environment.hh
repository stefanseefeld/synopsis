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

#ifndef _Environment_hh
#define _Environment_hh

#include <PTree.hh>

class Class;
class HashTable;
class Bind;
class Encoding;
class TypeInfo;
class Walker;

// class Environment

class Environment : public PTree::LightObject {
public:
    Environment(Walker* w);
    Environment(Environment* e);
    Environment(Environment* e, Walker* w);
    static void do_init_static();
    bool IsEmpty();
    Environment* GetOuterEnvironment() { return next; }
    Environment* GetBottom();
    void AddBaseclassEnv(Environment* e) { baseclasses.Append(e); }
    Walker* GetWalker() { return walker; }
    void SetWalker(Walker* w) { walker = w; }

    Class* LookupClassMetaobject(PTree::Node *name);
    bool LookupType(const char* name, int len, Bind*& t);

    bool Lookup(PTree::Node *name, bool& is_type_name, TypeInfo& t);
    bool Lookup(PTree::Node *name, TypeInfo& t);
  bool Lookup(PTree::Node *, Bind*&);
  bool LookupTop(PTree::Node *, Bind*&);

    bool LookupTop(const char* name, int len, Bind*& t);
    bool LookupAll(const char* name, int len, Bind*& t);

    bool RecordVariable(const char* name, Class* c);
    bool RecordPointerVariable(const char* name, Class* c);

    int AddEntry(const char*, int, Bind*);
    int AddDupEntry(const char*, int, Bind*);

  void RecordNamespace(PTree::Node *);
    bool LookupNamespace(const char*, int);
  void RecordTypedefName(PTree::Node *);
  void RecordEnumName(PTree::Node *);
  void RecordClassName(const char*, Class*);
  void RecordTemplateClass(PTree::Node *, Class*);
  Environment* RecordTemplateFunction(PTree::Node *, PTree::Node *);
  Environment* RecordDeclarator(PTree::Node *);
  Environment* DontRecordDeclarator(PTree::Node *);
  void RecordMetaclassName(PTree::Node *);
  PTree::Node *LookupMetaclass(PTree::Node *);
  static bool RecordClasskeyword(const char*, const char*);
  static PTree::Node *LookupClasskeyword(PTree::Node *);

    void SetMetaobject(Class* m) { metaobject = m; }
    Class* IsClassEnvironment() { return metaobject; }
    Class* LookupThis();
  Environment* IsMember(PTree::Node *);

    void Dump();
    void Dump(int);

//     PTree::Node *GetLineNumber(Ptree*, int&);

public:
  class Array : public PTree::LightObject {
    public:
	Array(int = 2);
	uint Number() { return num; }
	Environment* Ref(uint index);
	void Append(Environment*);
    private:
	uint num, size;
	Environment** array;
    };

private:
  Environment*	next;
  HashTable*		htable;
  Class*		metaobject;
  Walker*		walker;
  PTree::Array	metaclasses;
  static PTree::Array* classkeywords;
  Array		baseclasses;
  static HashTable*	namespace_table;
};

// class Bind and its subclasses

class Bind : public PTree::LightObject 
{
public:
  enum Kind 
  {
    isVarName, isTypedefName, isClassName, isEnumName, isTemplateClass,
    isTemplateFunction
  };
  virtual Kind What() = 0;
  virtual void GetType(TypeInfo&, Environment*) = 0;
  virtual const char *encoded_type();
  virtual bool IsType();
  virtual Class* ClassMetaobject();
  virtual void SetClassMetaobject(Class*);
};

class BindVarName : public Bind 
{
public:
  BindVarName(const char *t) : my_type(t) {}
  Kind What();
  void GetType(TypeInfo&, Environment*);
  const char *encoded_type();
  bool IsType();
private:
  const char *my_type;
};

class BindTypedefName : public Bind 
{
public:
  BindTypedefName(const char* t) : my_type(t) {}
  Kind What();
  void GetType(TypeInfo&, Environment*);
  const char *encoded_type();
private:
  const char *my_type;
};

class BindClassName : public Bind 
{
public:
  BindClassName(Class* c) { metaobject = c; }
  Kind What();
  void GetType(TypeInfo&, Environment*);
  Class* ClassMetaobject();
  void SetClassMetaobject(Class*);
private:
  Class* metaobject;
};

class BindEnumName : public Bind 
{
public:
  BindEnumName(const char *, PTree::Node *);
  Kind What();
  void GetType(TypeInfo&, Environment*);
  PTree::Node *GetSpecification() { return my_spec;}
private:
  const char *my_type;
  PTree::Node *my_spec;
};

class BindTemplateClass : public Bind 
{
public:
  BindTemplateClass(Class* c) { metaobject = c; }
  Kind What();
  void GetType(TypeInfo&, Environment*);
  Class* ClassMetaobject();
  void SetClassMetaobject(Class*);
private:
  Class* metaobject;
};

class BindTemplateFunction : public Bind 
{
public:
  BindTemplateFunction(PTree::Node *d) { decl = d; }
  Kind What();
  void GetType(TypeInfo&, Environment*);
  bool IsType();
private:
  PTree::Node *decl;
};

#endif
