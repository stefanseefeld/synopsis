//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef _Lexer_hh
#define _Lexer_hh

#include <vector>
#include <map>
#include <deque>
#include <stdexcept>

class Buffer;

//. A Token is what the Lexer splits an input stream into.
//. It refers to a region in the underlaying buffer and it
//. has a type
//.
//. line directive:
//.               ^"#"{blank}*{digit}+({blank}+.*)?\n
//.
//. pragma directive:
//.               ^"#"{blank}*"pragma".*\n
//. 
//. Constant	  {digit}+{int_suffix}*
//. 		  "0"{xletter}{hexdigit}+{int_suffix}*
//. 		  {digit}*\.{digit}+{float_suffix}*
//. 		  {digit}+\.{float_suffix}*
//. 		  {digit}*\.{digit}+"e"("+"|"-")*{digit}+{float_suffix}*
//. 		  {digit}+\."e"("+"|"-")*{digit}+{float_suffix}*
//. 		  {digit}+"e"("+"|"-")*{digit}+{float_suffix}*
//. 
//. CharConst	  \'([^'\n]|\\[^\n])\'
//. WideCharConst L\'([^'\n]|\\[^\n])\'
//. 
//. StringL	  \"([^"\n]|\\["\n])*\"
//. WideStringL	  L\"([^"\n]|\\["\n])*\"
//. 
//. Identifier	  {letter}+({letter}|{digit})*
//. 
//. AssignOp	*= /= %= += -= &= ^= <<= >>=
//. 
//. EqualOp		== !=
//. 
//. RelOp		<= >=
//. 
//. ShiftOp		<< >>
//. 
//. LogOrOp		||
//. 
//. LogAndOp	&&
//. 
//. IncOp		++ --
//. 
//. Scope		::
//. 
//. Ellipsis	...
//. 
//. PmOp		.* ->*
//. 
//. ArrowOp		->
//. 
//. others		!%^&*()-+={}|~[];:<>?,./
//. 
//. BadToken	others
//. 
struct Token 
{
  typedef int Type;
  enum
  {
    Identifier                = 258,
    Constant                  = 262,
    CharConst                 = 263,
    StringL                   = 264,
    AssignOp                  = 267,
    EqualOp                   = 268,
    RelOp                     = 269,
    ShiftOp                   = 270,
    LogOrOp                   = 271,
    LogAndOp                  = 272,
    IncOp                     = 273,
    Scope                     = 274,
    Ellipsis                  = 275,
    PmOp                      = 276,
    ArrowOp                   = 277,
    BadToken                  = 278,
    AUTO                      = 281,
    CHAR                      = 282,
    CLASS                     = 283,
    CONST                     = 284,
    DELETE                    = 285,
    DOUBLE                    = 286,
    ENUM                      = 287,
    EXTERN                    = 288,
    FLOAT                     = 289,
    FRIEND                    = 290,
    INLINE                    = 291,
    INT                       = 292,
    LONG                      = 293,
    NEW                       = 294,
    OPERATOR                  = 295,
    PRIVATE                   = 296,
    PROTECTED                 = 297,
    PUBLIC                    = 298,
    REGISTER                  = 299,
    SHORT                     = 300,
    SIGNED                    = 301,
    STATIC                    = 302,
    STRUCT                    = 303,
    TYPEDEF                   = 304,
    UNION                     = 305,
    UNSIGNED                  = 306,
    VIRTUAL                   = 307,
    VOID                      = 308,
    VOLATILE                  = 309,
    TEMPLATE                  = 310,
    MUTABLE                   = 311,
    BREAK                     = 312,
    CASE                      = 313,
    CONTINUE                  = 314,
    DEFAULT                   = 315,
    DO                        = 316,
    ELSE                      = 317,
    FOR                       = 318,
    GOTO                      = 319,
    IF                        = 320,
    RETURN                    = 321,
    SIZEOF                    = 322,
    SWITCH                    = 323,
    THIS                      = 324,
    WHILE                     = 325,
    ATTRIBUTE                 = 326, //=g++,
    METACLASS                 = 327, //=opencxx
    UserKeyword               = 328,
    UserKeyword2              = 329,
    UserKeyword3              = 330,
    UserKeyword4              = 331,
    BOOLEAN                   = 332,
    EXTENSION                 = 333, //=g++,
    TRY                       = 334,
    CATCH                     = 335,
    THROW                     = 336,
    UserKeyword5              = 337,
    NAMESPACE                 = 338,
    USING                     = 339,
    TYPEID                    = 340,
    TYPEOF                    = 341,
    WideStringL               = 342,
    WideCharConst             = 343,
    WCHAR                     = 344,
    
    //=non terminals,

    ntDeclarator              = 400,
    ntName                    = 401,
    ntFstyleCast              = 402,
    ntClassSpec               = 403,
    ntEnumSpec                = 404,
    ntDeclaration             = 405,
    ntTypedef                 = 406,
    ntTemplateDecl            = 407,
    ntMetaclassDecl           = 408,
    ntLinkageSpec             = 409,
    ntAccessSpec              = 410,
    ntUserAccessSpec          = 411,
    ntUserdefKeyword          = 412,
    ntExternTemplate          = 413,
    ntAccessDecl              = 414,
    ntNamespaceSpec           = 415,
    ntUsing                   = 416,
    ntTemplateInstantiation   = 417,
    ntNamespaceAlias          = 418,
    ntIfStatement             = 420,
    ntSwitchStatement         = 421,
    ntWhileStatement          = 422,
    ntDoStatement             = 423,
    ntForStatement            = 424,
    ntBreakStatement          = 425,
    ntContinueStatement       = 426,
    ntReturnStatement         = 427,
    ntGotoStatement           = 428,
    ntCaseStatement           = 429,
    ntDefaultStatement        = 430,
    ntLabelStatement          = 431,
    ntExprStatement           = 432,
    ntTryStatement            = 433,

    ntCommaExpr               = 450,
    ntAssignExpr              = 451,
    ntCondExpr                = 452,
    ntInfixExpr               = 453,
    ntPmExpr                  = 454,
    ntCastExpr                = 455,
    ntUnaryExpr               = 456,
    ntSizeofExpr              = 457,
    ntNewExpr                 = 458,
    ntDeleteExpr              = 459,
    ntArrayExpr               = 460,
    ntFuncallExpr             = 461,
    ntPostfixExpr             = 462,
    ntUserStatementExpr       = 463,
    ntDotMemberExpr           = 464,
    ntArrowMemberExpr         = 465,
    ntParenExpr               = 466,
    ntStaticUserStatementExpr = 467,
    ntThrowExpr               = 468,
    ntTypeidExpr              = 469,
    ntTypeofExpr              = 470,

    Ignore                    = 500,
    ASM                       = 501,
    DECLSPEC                  = 502,
    INT64                     = 503,
    Comment                   = 504 // this could eventually be used to report all comments
  };

  Token() : ptr(0), length(0), type(BadToken) {}
  Token(const char *s, size_t l, Type t) : ptr(s), length(l), type(t) {}
  bool operator == (char c) const { return *ptr == c && length == 1;}
  const char *ptr;
  size_t      length;
  Type        type;
};

//. a Lexer reads tokens from a stream.
class Lexer
{
public:
  typedef std::vector<Token> Comments;

  struct InvalidChar : std::runtime_error
  {
    InvalidChar(const std::string &msg) : std::runtime_error(msg) {}
  };

  Lexer(Buffer *);
  Token::Type get_token(Token &);
  Token::Type look_ahead(size_t);
  Token::Type look_ahead(size_t, Token &);

  const char *save();
  void restore(const char *);

  Comments get_comments();

  //. Return the origin of the given pointer
  //. (filename and line number)
  unsigned long origin(const char *, std::string &) const;
private:
  //. a Queue is used to read in tokens from a stream
  //. without consuming them
  class Queue
  {
  public:
    typedef std::deque<Token> Container;
    typedef Container::size_type size_type;

    bool empty() const { return my_container.empty();}
    size_type size() const { return my_container.size();}
    const Token &front() const { return my_container.front();}
    const Token &back() const { return my_container.back();}
    const Token &at(size_type i) const { return my_container.at(i);}
    void push(const Token &t) { my_container.push_back(t);}
    void pop() { my_container.pop_front();}
    void clear() { my_container.clear();}
  private:
    Container my_container;
  };
  typedef std::map<std::string, Token::Type> Dictionary;

  void rewind(const char *);

  Token::Type read_token(const char *&, size_t &);
  //. try to fill the token cache to contain
  //. at least o tokens. Returns false if
  //. there are not enough tokens.
  bool fill(size_t o);

  //. skip till end of paren
  void skip_paren();
  //. skip till end of line
  void skip_line();
  //. skip __attribute__(...), ___asm__(...), ...
  void skip_attribute();
  //. skip __extension__(...).
  Token::Type skip_extension(const char *&, size_t &);
  //. skip __asm ...
  void skip_asm();
  //. skip __declspec(...).
  void skip_declspec();

  char get_next_non_white_char();
  Token::Type read_line();
  bool read_char_const(unsigned long top);
  bool read_str_const(unsigned long top);
  Token::Type read_number(char c, unsigned long top);
  Token::Type read_float(unsigned long top);
  Token::Type read_identifier(unsigned long top);
  Token::Type screen(const char *identifier, size_t len);
  Token::Type read_separator(char c, unsigned long top);
  Token::Type single_char_op(unsigned char c);
  Token::Type read_comment(char c, unsigned long top);

  Buffer    *my_buffer;
  Queue      my_tokens;
  Dictionary my_keywords;
  Token      my_token;
  Comments   my_comments;
};

inline bool is_blank(char c)
{
  return c == ' ' || c == '\t' || c == '\f' || c == '\r';
}

inline bool is_letter(char c)
{
  return 'A' <= c && c <= 'Z' || 'a' <= c && c <= 'z' || c == '_' || c == '$';
}

inline bool is_digit(char c){ return '0' <= c && c <= '9';}

inline bool is_xletter(char c){ return c == 'X' || c == 'x';}

inline bool is_eletter(char c){ return c == 'E' || c == 'e';}

inline bool is_hexdigit(char c)
{
  return is_digit(c) || 'A' <= c && c <= 'F' || 'a' <= c && c <= 'f';
}

inline bool is_int_suffix(char c)
{
  return c == 'U' || c == 'u' || c == 'L' || c == 'l';
}

inline bool is_float_suffix(char c)
{
  return c == 'F' || c == 'f' || c == 'L' || c == 'l';
}

#endif
