/*
  Copyright (C) 1997,1998 Shigeru Chiba, University of Tsukuba.

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/

#ifndef _env_h
#define _env_h

#include "types.h"
#include "ptree-core.h"

class Class;
class HashTable;
class Bind;
class Encoding;
class TypeInfo;
class Walker;

// class Environment

class OCXXMOP Environment : public LightObject {
public:
    Environment(Walker* w);
    Environment(Environment* e);
    Environment(Environment* e, Walker* w);
    bool IsEmpty();
    Environment* GetOuterEnvironment() { return next; }
    Environment* GetBottom();
    void AddBaseclassEnv(Environment* e) { baseclasses.Append(e); }
    Walker* GetWalker() { return walker; }
    void SetWalker(Walker* w) { walker = w; }

    Class* LookupClassMetaobject(Ptree* name);
    bool LookupType(const char* name, int len, Bind*& t);

    bool Lookup(Ptree* name, bool& is_type_name, TypeInfo& t);
    bool Lookup(Ptree* name, TypeInfo& t);
    bool Lookup(Ptree*, Bind*&);
    bool LookupTop(Ptree*, Bind*&);

    bool LookupTop(const char* name, int len, Bind*& t);
    bool LookupAll(const char* name, int len, Bind*& t);

    bool RecordVariable(char* name, Class* c);
    bool RecordPointerVariable(char* name, Class* c);

    int AddEntry(char*, int, Bind*);
    int AddDupEntry(char*, int, Bind*);

    void RecordTypedefName(Ptree*);
    void RecordEnumName(Ptree*);
    void RecordClassName(char*, Class*);
    void RecordTemplateClass(Ptree*, Class*);
    Environment* RecordTemplateFunction(Ptree*, Ptree*);
    Environment* RecordDeclarator(Ptree*);
    Environment* DontRecordDeclarator(Ptree*);
    void RecordMetaclassName(Ptree*);
    Ptree* LookupMetaclass(Ptree*);
    static bool RecordClasskeyword(char*, char*);
    static Ptree* LookupClasskeyword(Ptree*);

    void SetMetaobject(Class* m) { metaobject = m; }
    Class* IsClassEnvironment() { return metaobject; }
    Class* LookupThis();
    Environment* IsMember(Ptree*);

    void Dump();
    void Dump(int);

public:
    class OCXXMOP Array : public LightObject {
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
    PtreeArray		metaclasses;
    static PtreeArray*	classkeywords;
    Array		baseclasses;
};

// class Bind and its subclasses

class Bind : public LightObject {
public:
    enum Kind {
	isVarName, isTypedefName, isClassName, isEnumName, isTemplateClass,
	isTemplateFunction
     };
    virtual Kind What() = nil;
    virtual void GetType(TypeInfo&, Environment*) = nil;
    virtual char* GetEncodedType();
    virtual bool IsType();
    virtual Class* ClassMetaobject();
    virtual void SetClassMetaobject(Class*);
};

class BindVarName : public Bind {
public:
    BindVarName(char* t) { type = t; }
    Kind What();
    void GetType(TypeInfo&, Environment*);
    char* GetEncodedType();
    bool IsType();

private:
    char* type;
};

class BindTypedefName : public Bind {
public:
    BindTypedefName(char* t) { type = t; }
    Kind What();
    void GetType(TypeInfo&, Environment*);
    char* GetEncodedType();

private:
    char* type;
};

class BindClassName : public Bind {
public:
    BindClassName(Class* c) { metaobject = c; }
    Kind What();
    void GetType(TypeInfo&, Environment*);
    Class* ClassMetaobject();
    void SetClassMetaobject(Class*);

private:
    Class* metaobject;
};

class BindEnumName : public Bind {
public:
    BindEnumName(char*, Ptree*);
    Kind What();
    void GetType(TypeInfo&, Environment*);
    Ptree* GetSpecification() { return specification; }

private:
    char* type;
    Ptree* specification;
};

class BindTemplateClass : public Bind {
public:
    BindTemplateClass(Class* c) { metaobject = c; }
    Kind What();
    void GetType(TypeInfo&, Environment*);
    Class* ClassMetaobject();
    void SetClassMetaobject(Class*);

private:
    Class* metaobject;
};

class BindTemplateFunction : public Bind {
public:
    BindTemplateFunction(Ptree* d) { decl = d; }
    Kind What();
    void GetType(TypeInfo&, Environment*);
    bool IsType();

private:
    Ptree* decl;
};

#endif /* _env_h */
