//
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_Token_hh_
#define Synopsis_Token_hh_

#include <cstring>

namespace Synopsis
{

//. A Token is what the Lexer splits an input stream into.
//. It refers to a region in the underlaying buffer and it
//. has a type
//.
//. line directive:   ^"#"{blank}*{digit}+({blank}+.*)?\n
//.
//. pragma directive: ^"#"{blank}*"pragma".*\n
//. 
//. Constant	      {digit}+{int_suffix}*
//. 		      "0"{xletter}{hexdigit}+{int_suffix}*
//. 		      {digit}*\.{digit}+{float_suffix}*
//. 		      {digit}+\.{float_suffix}*
//. 		      {digit}*\.{digit}+"e"("+"|"-")*{digit}+{float_suffix}*
//. 		      {digit}+\."e"("+"|"-")*{digit}+{float_suffix}*
//. 		      {digit}+"e"("+"|"-")*{digit}+{float_suffix}*
//. 
//. CharConst	      \'([^'\n]|\\[^\n])\'
//. WideCharConst     L\'([^'\n]|\\[^\n])\'
//. 
//. StringL	      \"([^"\n]|\\["\n])*\"
//. WideStringL	      L\"([^"\n]|\\["\n])*\"
//. 
//. Identifier	      {letter}+({letter}|{digit})*
//. 
//. AssignOp	      *= /= %= += -= &= ^= <<= >>=
//. 
//. EqualOp	      == !=
//. 
//. RelOp	      <= >=
//. 
//. ShiftOp	      << >>
//. 
//. LogOrOp	      ||
//. 
//. LogAndOp	      &&
//. 
//. IncOp	      ++ --
//. 
//. Scope	      ::
//. 
//. Ellipsis	      ...
//. 
//. PmOp	      .* ->*
//. 
//. ArrowOp	      ->
//. 
//. others	      !%^&*()-+={}|~[];:<>?,./
//. 
//. BadToken	      others
//. 
struct Token 
{
  typedef int Type;
  enum
  {
    Identifier = 258, //.< The first 256 are representing character literals.
    Constant,
    CharConst,
    StringL,
    AssignOp,
    EqualOp,
    RelOp,
    ShiftOp,
    LogOrOp,
    LogAndOp,
    IncOp,
    Scope,
    Ellipsis,
    PmOp,
    ArrowOp,
    BadToken,
    AUTO,
    CHAR,
    CLASS,
    CONST,
    DELETE,
    DOUBLE,
    ENUM,
    EXPLICIT,
    EXPORT,
    EXTERN,
    FLOAT,
    FRIEND,
    INLINE,
    INT,
    LONG,
    NEW,
    OPERATOR,
    PRIVATE,
    PROTECTED,
    PUBLIC,
    REGISTER,
    SHORT,
    SIGNED,
    STATIC,
    STRUCT,
    TYPEDEF,
    TYPENAME,
    UNION,
    UNSIGNED,
    VIRTUAL,
    VOID,
    VOLATILE,
    TEMPLATE,
    MUTABLE,
    BREAK,
    CASE,
    CONTINUE,
    DEFAULT,
    DO,
    ELSE,
    FOR,
    GOTO,
    IF,
    OFFSETOF,
    RETURN,
    SIZEOF,
    SWITCH,
    THIS,
    WHILE,
    CONST_CAST,
    DYNAMIC_CAST,
    STATIC_CAST,
    REINTERPRET_CAST,
    ATTRIBUTE,    //=g++,
    BOOLEAN,
    EXTENSION,    //=g++,
    TRY,
    CATCH,
    THROW,
    NAMESPACE,
    USING,
    TYPEID,
    TYPEOF,
    WideStringL,
    WideCharConst,
    WCHAR,

    Ignore  = 500,
    ASM,
    DECLSPEC,
    PRAGMA,
    INT64,
    Comment // This could eventually be used to report all comments.
  };

  Token() : ptr(0), length(0), type(BadToken) {}
  Token(const char *s, size_t l, Type t) : ptr(s), length(l), type(t) {}
  bool operator == (char c) const { return *ptr == c && length == 1;}
  const char *ptr;
  size_t      length;
  Type        type;
};

}

#endif
