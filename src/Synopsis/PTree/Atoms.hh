//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_PTree_Atoms_hh_
#define Synopsis_PTree_Atoms_hh_

#include "Synopsis/PTree/Node.hh"

namespace Synopsis
{
namespace PTree
{

class Literal : public Atom
{
public:
  Literal(const Token &tk) : Atom(tk), my_type(tk.type) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
  Token::Type type() const { return my_type;}
private:
  Token::Type my_type;
};

class CommentedAtom : public Atom
{
public:
  CommentedAtom(const Token &tk, Node *c = 0) : Atom(tk), my_comments(c) {}
  CommentedAtom(const char *p, size_t l, Node *c = 0) : Atom(p, l), my_comments(c) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}

  Node *get_comments() { return my_comments;}
  void set_comments(Node *c) { my_comments = c;}
private:
  Node *my_comments;
};

// class DupLeaf is used by Ptree::Make() and QuoteClass (qMake()).
// The string given to the constructors are duplicated.

class DupAtom : public CommentedAtom 
{
public:
  DupAtom(const char *, size_t);
  DupAtom(const char *, size_t, const char *, size_t);
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class Identifier : public CommentedAtom
{
public:
  Identifier(const char *p, size_t l) : CommentedAtom(p, l) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class Reserved : public CommentedAtom
{
public:
  Reserved(const Token &t) : CommentedAtom(t) {}
  Reserved(const char* str, int len) : CommentedAtom(str, len) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

class This : public Reserved
{
public:
  This(Token& t) : Reserved(t) {}
  virtual void accept(Visitor *visitor) { visitor->visit(this);}
};

template <typename T, Token::Type t>
class ReservedT : public Reserved
{
public:
  ReservedT(const Token &tk) : Reserved(tk) {}
  ReservedT(const char *ptr, size_t length) : Reserved(ptr, length) {}
  virtual void accept(Visitor *visitor) { visitor->visit(static_cast<T*>(this));}
};

struct AtomAUTO : ReservedT<AtomAUTO, Token::AUTO> 
{
  AtomAUTO(const Token &t) : ReservedT<AtomAUTO, Token::AUTO>(t) {}
  AtomAUTO(const char *p, size_t l) : ReservedT<AtomAUTO, Token::AUTO>(p, l) {}
};

struct AtomBOOLEAN : ReservedT<AtomBOOLEAN, Token::BOOLEAN> 
{
  AtomBOOLEAN(const Token &t) : ReservedT<AtomBOOLEAN, Token::BOOLEAN>(t) {}
  AtomBOOLEAN(const char *p, size_t l) : ReservedT<AtomBOOLEAN, Token::BOOLEAN>(p, l) {}
};

struct AtomCHAR : ReservedT<AtomCHAR, Token::CHAR> 
{
  AtomCHAR(const Token &t) : ReservedT<AtomCHAR, Token::CHAR>(t) {}
  AtomCHAR(const char *p, size_t l) : ReservedT<AtomCHAR, Token::CHAR>(p, l) {}
};

struct AtomWCHAR : ReservedT<AtomWCHAR, Token::WCHAR> 
{
  AtomWCHAR(const Token &t) : ReservedT<AtomWCHAR, Token::WCHAR>(t) {}
  AtomWCHAR(const char *p, size_t l) : ReservedT<AtomWCHAR, Token::WCHAR>(p, l) {}
};

struct AtomCONST : ReservedT<AtomCONST, Token::CONST> 
{
  AtomCONST(const Token &t) : ReservedT<AtomCONST, Token::CONST>(t) {}
  AtomCONST(const char *p, size_t l) : ReservedT<AtomCONST, Token::CONST>(p, l) {}
};

struct AtomDOUBLE : ReservedT<AtomDOUBLE, Token::DOUBLE> 
{
  AtomDOUBLE(const Token &t) : ReservedT<AtomDOUBLE, Token::DOUBLE>(t) {}
  AtomDOUBLE(const char *p, size_t l) : ReservedT<AtomDOUBLE, Token::DOUBLE>(p, l) {}
};

struct AtomEXTERN : ReservedT<AtomEXTERN, Token::EXTERN> 
{
  AtomEXTERN(const Token &t) : ReservedT<AtomEXTERN, Token::EXTERN>(t) {}
  AtomEXTERN(const char *p, size_t l) : ReservedT<AtomEXTERN, Token::EXTERN>(p, l) {}
};

struct AtomFLOAT : ReservedT<AtomFLOAT, Token::FLOAT> 
{ 
  AtomFLOAT(const Token &t) : ReservedT<AtomFLOAT, Token::FLOAT>(t) {}
  AtomFLOAT(const char *p, size_t l) : ReservedT<AtomFLOAT, Token::FLOAT>(p, l) {}
};

struct AtomFRIEND : ReservedT<AtomFRIEND, Token::FRIEND> 
{
  AtomFRIEND(const Token &t) : ReservedT<AtomFRIEND, Token::FRIEND>(t) {}
  AtomFRIEND(const char *p, size_t l) : ReservedT<AtomFRIEND, Token::FRIEND>(p, l) {}
};

struct AtomINLINE : ReservedT<AtomINLINE, Token::INLINE> 
{
  AtomINLINE(const Token &t) : ReservedT<AtomINLINE, Token::INLINE>(t) {}
  AtomINLINE(const char *p, size_t l) : ReservedT<AtomINLINE, Token::INLINE>(p, l) {}
};

struct AtomINT : ReservedT<AtomINT, Token::INT> 
{
  AtomINT(const Token &t) : ReservedT<AtomINT, Token::INT>(t) {}
  AtomINT(const char *p, size_t l) : ReservedT<AtomINT, Token::INT>(p, l) {}
};

struct AtomLONG : ReservedT<AtomLONG, Token::LONG> 
{
  AtomLONG(const Token &t) : ReservedT<AtomLONG, Token::LONG>(t) {}
  AtomLONG(const char *p, size_t l) : ReservedT<AtomLONG, Token::LONG>(p, l) {}
};

struct AtomMUTABLE : ReservedT<AtomMUTABLE, Token::MUTABLE> 
{
  AtomMUTABLE(const Token &t) : ReservedT<AtomMUTABLE, Token::MUTABLE>(t) {}
  AtomMUTABLE(const char *p, size_t l) : ReservedT<AtomMUTABLE, Token::MUTABLE>(p, l) {}
};

struct AtomNAMESPACE : ReservedT<AtomNAMESPACE, Token::NAMESPACE> 
{
  AtomNAMESPACE(const Token &t) : ReservedT<AtomNAMESPACE, Token::NAMESPACE>(t) {}
  AtomNAMESPACE(const char *p, size_t l) : ReservedT<AtomNAMESPACE, Token::NAMESPACE>(p, l) {}
};

struct AtomPRIVATE : ReservedT<AtomPRIVATE, Token::PRIVATE> 
{
  AtomPRIVATE(const Token &t) : ReservedT<AtomPRIVATE, Token::PRIVATE>(t) {}
  AtomPRIVATE(const char *p, size_t l) : ReservedT<AtomPRIVATE, Token::PRIVATE>(p, l) {}
};

struct AtomPROTECTED : ReservedT<AtomPROTECTED, Token::PROTECTED> 
{
  AtomPROTECTED(const Token &t) : ReservedT<AtomPROTECTED, Token::PROTECTED>(t) {}
  AtomPROTECTED(const char *p, size_t l) : ReservedT<AtomPROTECTED, Token::PROTECTED>(p, l) {}
};

struct AtomPUBLIC : ReservedT<AtomPUBLIC, Token::PUBLIC> 
{
  AtomPUBLIC(const Token &t) : ReservedT<AtomPUBLIC, Token::PUBLIC>(t) {}
  AtomPUBLIC(const char *p, size_t l) : ReservedT<AtomPUBLIC, Token::PUBLIC>(p, l) {}
};

struct AtomREGISTER : ReservedT<AtomREGISTER, Token::REGISTER> 
{
  AtomREGISTER(const Token &t) : ReservedT<AtomREGISTER, Token::REGISTER>(t) {}
  AtomREGISTER(const char *p, size_t l) : ReservedT<AtomREGISTER, Token::REGISTER>(p, l) {}
};

struct AtomSHORT : ReservedT<AtomSHORT, Token::SHORT> 
{
  AtomSHORT(const Token &t) : ReservedT<AtomSHORT, Token::SHORT>(t) {}
  AtomSHORT(const char *p, size_t l) : ReservedT<AtomSHORT, Token::SHORT>(p, l) {}
};

struct AtomSIGNED : ReservedT<AtomSIGNED, Token::SIGNED> 
{
  AtomSIGNED(const Token &t) : ReservedT<AtomSIGNED, Token::SIGNED>(t) {}
  AtomSIGNED(const char *p, size_t l) : ReservedT<AtomSIGNED, Token::SIGNED>(p, l) {}
};

struct AtomSTATIC : ReservedT<AtomSTATIC, Token::STATIC> 
{
  AtomSTATIC(const Token &t) : ReservedT<AtomSTATIC, Token::STATIC>(t) {}
  AtomSTATIC(const char *p, size_t l) : ReservedT<AtomSTATIC, Token::STATIC>(p, l) {}
};

struct AtomUNSIGNED : ReservedT<AtomUNSIGNED, Token::UNSIGNED> 
{
  AtomUNSIGNED(const Token &t) : ReservedT<AtomUNSIGNED, Token::UNSIGNED>(t) {}
  AtomUNSIGNED(const char *p, size_t l) : ReservedT<AtomUNSIGNED, Token::UNSIGNED>(p, l) {}
};

struct AtomUSING : ReservedT<AtomUSING, Token::USING> 
{
  AtomUSING(const Token &t) : ReservedT<AtomUSING, Token::USING>(t) {}
  AtomUSING(const char *p, size_t l) : ReservedT<AtomUSING, Token::USING>(p, l) {}
};

struct AtomVIRTUAL : ReservedT<AtomVIRTUAL, Token::VIRTUAL> 
{
  AtomVIRTUAL(const Token &t) : ReservedT<AtomVIRTUAL, Token::VIRTUAL>(t) {}
  AtomVIRTUAL(const char *p, size_t l) : ReservedT<AtomVIRTUAL, Token::VIRTUAL>(p, l) {}
};

struct AtomVOID : ReservedT<AtomVOID, Token::VOID> 
{
  AtomVOID(const Token &t) : ReservedT<AtomVOID, Token::VOID>(t) {}
  AtomVOID(const char *p, size_t l) : ReservedT<AtomVOID, Token::VOID>(p, l) {}
};

struct AtomVOLATILE : ReservedT<AtomVOLATILE, Token::VOLATILE> 
{
  AtomVOLATILE(const Token &t) : ReservedT<AtomVOLATILE, Token::VOLATILE>(t) {}
  AtomVOLATILE(const char *p, size_t l) : ReservedT<AtomVOLATILE, Token::VOLATILE>(p, l) {}
};

struct AtomUserKeyword2 : ReservedT<AtomUserKeyword2, Token::UserKeyword2> 
{
  AtomUserKeyword2(const Token &t) : ReservedT<AtomUserKeyword2, Token::UserKeyword2>(t) {}
  AtomUserKeyword2(const char *p, size_t l) : ReservedT<AtomUserKeyword2, Token::UserKeyword2>(p, l) {}
};

}
}

#endif
