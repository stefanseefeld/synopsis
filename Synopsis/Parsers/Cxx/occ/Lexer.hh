/*
  Copyright (C) 1997-2001 Shigeru Chiba, Tokyo Institute of Technology.

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/

/*
  Copyright (c) 1995, 1996 Xerox Corporation.
  All Rights Reserved.

  Use and copying of this software and preparation of derivative works
  based upon this software are permitted. Any copy of this software or
  of any derivative work must include the above copyright notice of
  Xerox Corporation, this paragraph and the one after it.  Any
  distribution of this software or derivative works must comply with all
  applicable United States export control laws.

  This software is made available AS IS, and XEROX CORPORATION DISCLAIMS
  ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE, AND NOTWITHSTANDING ANY OTHER PROVISION CONTAINED HEREIN, ANY
  LIABILITY FOR DAMAGES RESULTING FROM THE SOFTWARE OR ITS USE IS
  EXPRESSLY DISCLAIMED, WHETHER ARISING IN CONTRACT, TORT (INCLUDING
  NEGLIGENCE) OR STRICT LIABILITY, EVEN IF XEROX CORPORATION IS ADVISED
  OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#ifndef _Lexer_hh
#define _Lexer_hh

#include "types.h"

class Buffer;
class HashTable;
class Ptree;

// class Token

struct Token 
{
  bool Eq(char c) { return(*ptr == c && len == 1);}
  char* ptr;
  int len;
  int kind;
};

// class Lexer

class Lexer : public Object 
{
public:
  Lexer(Buffer *);
  int GetToken(Token&);
  int LookAhead(int);
  int LookAhead(int, Token&);

  char* Save();
  void Restore(char*);
  void GetOnlyClosingBracket(Token&);

  Ptree* GetComments();
  Ptree* GetComments2();

  uint LineNumber(char*, char*&, int&);

  static bool RecordKeyword(char*, int);
  static bool Reify(Ptree*, unsigned int&);
  static bool Reify(Ptree* t, char*&);

private:
  class TokenFifo 
  {
  public:
    TokenFifo(Lexer*);
    ~TokenFifo();
    void Clear();
    void Push(int, char*, int);
    int Pop(char*&, int&);
    int Peek(int);
    int Peek(int, char*&, int&);
  private:
    int Peek2(int);
    Lexer *lexer;
    int head;
    int tail;
    int size;
    struct Slot 
    {
      int token;
      char* pos;
      int len;
    }* ring;
  };

  friend class TokenFifo;

  uint Tokenp() { return tokenp; }
  int TokenLen() { return token_len; }
  char* TokenPosition();
  char Ref(uint i);
  void Rewind(char*);

  int ReadToken(char*&, int&);
  void SkipAttributeToken();
  int SkipExtensionToken(char*&, int&);

#if defined(_MSC_VER) || defined(PARSE_MSVC)
  void SkipAsmToken();
  void SkipDeclspecToken();
#endif

  char GetNextNonWhiteChar();
  int ReadLine();
  bool ReadCharConst(uint top);
  bool ReadStrConst(uint top);
  int ReadNumber(char c, uint top);
  int ReadFloat(uint top);
  bool ReadLineDirective();
  int ReadIdentifier(uint top);
  int Screening(char *identifier, int len);
  int ReadSeparator(char c, uint top);
  int SingleCharOp(unsigned char c);
  int ReadComment(char c, uint top);

private:
  Buffer *file;
  TokenFifo fifo;
  uint tokenp;
  int token_len;
  int last_token;

  static HashTable* user_keywords;
  static Ptree* comments;
};

// convenient functions

inline bool is_blank(char c)
{
  return(c == ' ' || c == '\t' || c == '\f' || c == '\r');
}

inline bool is_letter(char c)
{
  return('A' <= c && c <= 'Z' || 'a' <= c && c <= 'z' || c == '_'
	 || c == '$');
}

inline bool is_digit(char c){ return('0' <= c && c <= '9'); }

inline bool is_xletter(char c){ return(c == 'X' || c == 'x'); }

inline bool is_eletter(char c){ return(c == 'E' || c == 'e'); }

inline bool is_hexdigit(char c)
{
  return(is_digit(c) || 'A' <= c && c <= 'F' || 'a' <= c && c <= 'f');
}

inline bool is_int_suffix(char c)
{
  return(c == 'U' || c == 'u' || c == 'L' || c == 'l');
}

inline bool is_float_suffix(char c)
{
  return(c == 'F' || c == 'f' || c == 'L' || c == 'l');
}

// tokens

enum {

Identifier=	258,
Constant=	262,
CharConst=	263,
StringL	=	264,
AssignOp=	267,
EqualOp	=	268,
RelOp	=	269,
ShiftOp	=	270,
LogOrOp	=	271,
LogAndOp=	272,
IncOp	=	273,
Scope	=	274,
Ellipsis=	275,
PmOp	=	276,
ArrowOp	=	277,
BadToken=	278,
AUTO	=	281,
CHAR	=	282,
CLASS	=	283,
CONST	=	284,
DELETE	=	285,
DOUBLE	=	286,
ENUM	=	287,
EXTERN	=	288,
FLOAT	=	289,
FRIEND	=	290,
INLINE	=	291,
INT	=	292,
LONG	=	293,
NEW	=	294,
OPERATOR=	295,
PRIVATE	=	296	/*=must be consistent with Class::Private */,
PROTECTED=	297	/*=must be consistent with Class::Protected */,
PUBLIC	=	298	/*=must be consistent with Class::Public */,
REGISTER=	299,
SHORT	=	300,
SIGNED	=	301,
STATIC	=	302,
STRUCT	=	303,
TYPEDEF	=	304,
UNION	=	305,
UNSIGNED=	306,
VIRTUAL	=	307,
VOID	=	308,
VOLATILE=	309,
TEMPLATE=	310,
MUTABLE	=	311,
BREAK	=	312,
CASE	=	313,
CONTINUE=	314,
DEFAULT	=	315,
DO	=	316,
ELSE	=	317,
FOR	=	318,
GOTO	=	319,
IF	=	320,
RETURN	=	321,
SIZEOF	=	322,
SWITCH	=	323,
THIS	=	324,
WHILE	=	325,
ATTRIBUTE=	326,		//=g++,
METACLASS=	327,
UserKeyword=	328,
UserKeyword2=	329,
UserKeyword3=	330,
UserKeyword4=	331,
BOOLEAN	=	332,
EXTENSION=	333,		//=g++,
TRY	=	334,
CATCH	=	335,
THROW	=	336,
UserKeyword5=	337,
NAMESPACE=	338,
USING	=	339,
TYPEID=         340,
TYPEOF=         341,
WideStringL =   342,
WideCharConst = 343,
WCHAR =         344,
    
//=non terminals,

ntDeclarator	=400,
ntName		=401,
ntFstyleCast	=402,
ntClassSpec	=403,
ntEnumSpec	=404,
ntDeclaration	=405,
ntTypedef	=406,
ntTemplateDecl	=407,
ntMetaclassDecl	=408,
ntLinkageSpec	=409,
ntAccessSpec	=410,
ntUserAccessSpec=411,
ntUserdefKeyword=412,
ntExternTemplate=413,
ntAccessDecl	=414,
ntNamespaceSpec	=415,
ntUsing		=416,
ntTemplateInstantiation=	417,
ntNamespaceAlias=418,

ntIfStatement	=	420,
ntSwitchStatement=	421,
ntWhileStatement=	422,
ntDoStatement	=	423,
ntForStatement	=	424,
ntBreakStatement=	425,
ntContinueStatement=	426,
ntReturnStatement=	427,
ntGotoStatement	=	428,
ntCaseStatement	=	429,
ntDefaultStatement=	430,
ntLabelStatement=	431,
ntExprStatement	=	432,
ntTryStatement	=	433,

ntCommaExpr	=	450,
ntAssignExpr	=	451,
ntCondExpr	=	452,
ntInfixExpr	=	453,
ntPmExpr	=	454,
ntCastExpr	=	455,
ntUnaryExpr	=	456,
ntSizeofExpr	=	457,
ntNewExpr	=	458,
ntDeleteExpr	=	459,
ntArrayExpr	=	460,
ntFuncallExpr	=	461,
ntPostfixExpr	=	462,
ntUserStatementExpr=	463,
ntDotMemberExpr	=	464,
ntArrowMemberExpr=	465,
ntParenExpr	=	466,
ntStaticUserStatementExpr=467,
ntThrowExpr	=	468,
ntTypeidExpr=           469,
ntTypeofExpr=           470,

Ignore	=	500,
ASM	=	501,
DECLSPEC=       502,
INT64	=	503
};

#endif
