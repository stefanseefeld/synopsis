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

#ifndef _Encoding_hh
#define _Encoding_hh

#include <iosfwd>
#include <PTree.hh>

class Environment;
class Bind;

class Encoding {
public:
    enum { MaxNameLen = 4096 }; // const MaxNameLen = 256;
			       // MSVC doesn't compile
    Encoding() { len = 0; }
    Encoding(Encoding& e) { Reset(e); }
    static void do_init_static();
    void Clear() { len = 0; }
    void Reset(Encoding&);
    char* Get();
    bool IsEmpty() { return len == 0; }

  void CvQualify(PTree::Node *, PTree::Node * = 0);
    void SimpleConst() { Append("Ci", 2); }
    void GlobalScope();				// ::Type
  void SimpleName(PTree::Node *);
    void NoName();
  void Template(PTree::Node *, Encoding&);
    void Qualified(int);
  void Destructor(PTree::Node *);
    void PtrOperator(int);
    void PtrToMember(Encoding&, int);
    void CastOperator(Encoding&);
    void Array() { Insert('A'); }
    void Function(Encoding& args) { Insert((char*)args.name, args.len); }
    void Recursion(Encoding& e) { Insert((char*)e.name, e.len); }
    void StartFuncArgs() { Append((unsigned char)'F'); }
    void EndFuncArgs() { Append((unsigned char)'_'); }
    void Void() { Append((unsigned char)'v'); }
    void EllipsisArg() { Append((unsigned char)'e'); }
    void NoReturnType() { Append((unsigned char)'?'); }
    void ValueTempParam() { Append((unsigned char)'*'); }

    void Insert(unsigned char);
    void Insert(const char *, size_t);
    void Append(unsigned char);
    void Append(const char *, size_t);
    void Append(Encoding& e) { Append((const char*)e.name, e.len); }
    void AppendWithLen(const char *, size_t);
    void AppendWithLen(Encoding& e) { AppendWithLen((const char*)e.name, e.len); }

    static void print(std::ostream &, const char *);
  static const char *GetBaseName(const char *, int&, Environment*&);
  static PTree::Node *MakePtree(const unsigned char*&, PTree::Node *);

  static PTree::Node *MakeQname(const unsigned char*&);
  static PTree::Node *MakeLeaf(const unsigned char*&);
  static bool IsSimpleName(const unsigned char*);
  static PTree::Node *NameToPtree(const char*, int);

  static const unsigned char* GetTemplateArguments(const unsigned char*, int&);

private:
    static Environment* ResolveTypedefName(Environment*, const char*, int);
    static int GetBaseNameIfTemplate(const unsigned char*, Environment*&);

private:
    unsigned char name[MaxNameLen];
    int len;

public:
  static PTree::Node *bool_t, *char_t, *wchar_t_t, *int_t, *short_t, *long_t,
		 *float_t, *double_t, *void_t;

  static PTree::Node *signed_t, *unsigned_t, *const_t, *volatile_t;

  static PTree::Node *operator_name, *new_operator, *anew_operator,
		 *delete_operator, *adelete_operator;

  static PTree::Node *star, *ampersand, *comma, *dots, *scope, *tilder,
		 *left_paren, *right_paren, *left_bracket, *right_bracket,
		 *left_angle, *right_angle;
};

#endif
