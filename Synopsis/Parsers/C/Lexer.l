
%{
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

#undef yywrap
extern "C" int yywrap(void);

#define  YY_BUFFER_STATE_DEFD

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cerrno>

#include <config.hh>
#include "Grammar.hh"
#include <Lexer.hh>
#include <Token.hh>
#include <Symbol.hh>
#include <Expression.hh>
#include <Statement.hh>
#include <File.hh>

#ifdef __WIN32__
# ifndef  __STDC__
#  define  __STDC__    1
# endif

int isatty(int) { return 1; }

#endif  /* __WIN32__ */

int cnt_nl(char *txt, int len);

#undef YY_INPUT

/* Scan input from a file pointer (default) */

#define YY_INPUT(buf,result,max_size) \
    do { \
        global_parse_env->last_line=global_parse_env->line; \
        for(result=0; result<max_size; result++) \
         { \
          char ch; \
          if(!global_parse_env->yyinstream->read(&ch, 1)) \
           { \
            if(result==0) result=YY_NULL; \
            break; \
           } \
          if(result==0) global_parse_env->line=""; \
          global_parse_env->line+=ch; \
          buf[result]=ch; \
          if(ch=='\n') { result++; break; } \
         } \
    } while(0)

#define YY_USER_ACTION \
    { \
        global_parse_env->yycolno = global_parse_env->yynxtcol; \
        global_parse_env->yynxtcol += yyleng; \
    }


#undef HERE
#define HERE Location(global_parse_env->yylineno, \
       global_parse_env->yycolno, global_parse_env->filename )

#undef SETPOS_TOKEN
#define SETPOS_TOKEN(TOKEN) \
	do { \
          lvalp->loc = new Location( \
                global_parse_env->yylineno, \
                global_parse_env->yycolno,  \
                global_parse_env->filename ); \
	} while(0)

#undef  LAST_STATE
#define LAST_STATE() \
        do { \
          if (global_parse_env->in_preprocess) \
            BEGIN(PP); \
          else \
            BEGIN(INITIAL); \
        } while (0)  

#undef  BEGIN_PP
#define BEGIN_PP() \
        do { \
          BEGIN(PP); \
          global_parse_env->in_preprocess = 1; \
        } while(0)

YYSTYPE my_var;

EXTERN int yylex ARGS((YYSTYPE *lvalp));

/*  To give some args to yylex */
#undef YY_DECL
#ifdef  __STDC__
/*  #define YY_DECL int yylex YY_PROTO((YYSTYPE *lvalp))  */
#define YY_DECL int yylex (YYSTYPE *lvalp)
#else
#define YY_DECL int yylex(lvalp) YYSTYPE *lvalp;
#endif

%}
%x Start PP CMMT CC STR PPLN PAREN_ELIM GCC_ATTRIB MSC_ASM_ELIM
%a 9000
%o 9000
/* %option nounput */

dot		"."
digit		[0-9]
octdigit	[0-7]
hexdigit	[0-9a-fA-F]
digits          {digit}+
alpha		[a-zA-Z_$]
alphanum	{alpha}|{digit}

usuffix         [uU]
lsuffix         [lL]
intsuffix       {usuffix}{lsuffix}?|{lsuffix}{usuffix}?
intnum		{digit}+{intsuffix}?
octnum		0{octdigit}+{intsuffix}?
hexnum		0[xX]{hexdigit}+{intsuffix}?

exponent	[Ee][+-]?{digits}
floatsuffix     [fFlL]

whitespace      [ \t\f\v]
allwhite        [ \t\f\b\v\r\n]

pp_strt         ^{whitespace}*"#"{whitespace}*

%%

<INITIAL>"/*"       { BEGIN(CMMT); }
<INITIAL>"'"        { BEGIN(CC); global_parse_env->isWide = false; }
<INITIAL>L"'"       { BEGIN(CC); global_parse_env->isWide = true; }
<INITIAL>\"         { BEGIN(STR); global_parse_env->tokn_ptr = global_parse_env->tokn_buff; global_parse_env->isWide = false; }
<INITIAL>L\"        { BEGIN(STR); global_parse_env->tokn_ptr = global_parse_env->tokn_buff; global_parse_env->isWide = true; }

<INITIAL>"//*"      { // Ambiguous C++ style comment, must parse as
                      //    '/' '/*' to be Ansi compliant 
                         yywarn(
                            "Ambiguous C++ comment, use '/ /*' -or- '// *'");
                         yyless(2);
                         SETPOS_TOKEN(DIV);
                         return(DIV);
                    }

<INITIAL>"//"[^*].*$ { /* C++ style comment */
                         if (global_parse_env->cpp_comment_warn){
                             global_parse_env->cpp_comment_warn = 0;
                             yywarn("C++ style comment encountered");
                         }
                     }

<INITIAL>"auto"     {
                      lvalp->storage = Storage::Auto;
                      return(global_parse_env->err_tok = AUTO);
                    }
<INITIAL>"extern"   {
                      lvalp->storage = Storage::Extern;
                      return(global_parse_env->err_tok = EXTRN);
                    }
<INITIAL>"register" {
                      lvalp->storage = Storage::Register;
                      return(global_parse_env->err_tok = REGISTR);
                    }
<INITIAL>"static"   {
                      lvalp->storage = Storage::Static;
                      return(global_parse_env->err_tok = STATIC);
                    }
<INITIAL>"typedef"  {
                      lvalp->storage = Storage::Typedef;
                      return(global_parse_env->err_tok = TYPEDEF);
                    }

<INITIAL>"const"    {
                      lvalp->typeQual = TypeQual::Const;
                      return(global_parse_env->err_tok = CONST);
                    }
<INITIAL>"volatile" {
                      lvalp->typeQual = TypeQual::Volatile;
                      return(global_parse_env->err_tok = VOLATILE);
                    }

<INITIAL>"void"     {
                      lvalp->base = new BaseType(BaseType::Void);
                      return(global_parse_env->err_tok = VOID);
                    }
<INITIAL>"_Bool"     {
                      lvalp->base = new BaseType(BaseType::Bool);
                      return(global_parse_env->err_tok = BOOL);
                    }
<INITIAL>"char"     {
                      lvalp->base = new BaseType(BaseType::Char);
                      return(global_parse_env->err_tok = CHAR);
                    }
<INITIAL>"short"    {
                      lvalp->base = new BaseType(BaseType::Short);
                      return(global_parse_env->err_tok = SHORT);
                    }
<INITIAL>"int"      {
                      lvalp->base = new BaseType(BaseType::Int);
                      return(global_parse_env->err_tok = INT);
                    }
<INITIAL>"__int64"  {
                      lvalp->base = new BaseType(BaseType::Int64);
                      return(global_parse_env->err_tok = INT64);
                    }
<INITIAL>"__int32"  {
                      lvalp->base = new BaseType(BaseType::Int32);
                      return(global_parse_env->err_tok = INT32);
                    }
<INITIAL>"__int16"  {
                      lvalp->base = new BaseType(BaseType::Int16);
                      return(global_parse_env->err_tok = INT16);
                    }
<INITIAL>"__int8"   {
                      lvalp->base = new BaseType(BaseType::Int8);
                      return(global_parse_env->err_tok = INT8);
                    }
<INITIAL>"long"     {
                      lvalp->base = new BaseType(BaseType::Long);
                      return(global_parse_env->err_tok = LONG);
                    }
<INITIAL>"float"    {
                      lvalp->base = new BaseType(BaseType::Float);
                      return(global_parse_env->err_tok = FLOAT);
                    }
<INITIAL>"double"   {
                      lvalp->base = new BaseType(BaseType::Double);
                      return(global_parse_env->err_tok = DOUBLE);
                    }
<INITIAL>"signed"   {
                      lvalp->base = new BaseType(BaseType::Signed);
                      return(global_parse_env->err_tok = SGNED);
                    }
<INITIAL>"unsigned" {
                      lvalp->base = new BaseType(BaseType::UnSigned);
                      return(global_parse_env->err_tok = UNSGNED);
                    }
<INITIAL>"enum"     {
                      lvalp->typeSpec = BaseType::Enum;
                      global_parse_env->possibleTag = true;
                      return(global_parse_env->err_tok = ENUM);
                    }
<INITIAL>"struct"   { 
                      lvalp->typeSpec = BaseType::Struct;
                      global_parse_env->possibleTag = true;
                      return(global_parse_env->err_tok = STRUCT);
                    }
<INITIAL>"union"    { 
                      lvalp->typeSpec = BaseType::Union;
                      global_parse_env->possibleTag = true;
                      return(global_parse_env->err_tok = UNION);
                    }

<INITIAL>"__builtin_va_list"    { 	/* A GCC extension */
   /* TODO - give it a real type */
                      lvalp->base = new BaseType(BaseType::Int);
                      return(global_parse_env->err_tok = INT);
/* 					  SETPOS_TOKEN(ELLIPSIS); return(ELLIPSIS); */
                    }
<INITIAL>"break"    { SETPOS_TOKEN(BREAK); return(BREAK); }
<INITIAL>"case"     { SETPOS_TOKEN(CASE); return(CASE); }
<INITIAL>"continue" { SETPOS_TOKEN(CONT); return(CONT); }
<INITIAL>"default"  { SETPOS_TOKEN(DEFLT); return(DEFLT); }
<INITIAL>"do"       { SETPOS_TOKEN(DO); return(DO); }
<INITIAL>"else"     { SETPOS_TOKEN(ELSE); return(ELSE); }
<INITIAL>"for"      { SETPOS_TOKEN(FOR); return(FOR); }
<INITIAL>"goto"     { SETPOS_TOKEN(GOTO); 
                      global_parse_env->possibleLabel = true;
                      return(GOTO); }
<INITIAL>"if"       { SETPOS_TOKEN(IF); return(IF); }
<INITIAL>"return"   { SETPOS_TOKEN(RETURN); return(RETURN); }
<INITIAL>"sizeof"   { SETPOS_TOKEN(SIZEOF); return(SIZEOF); }
<INITIAL>"switch"   { SETPOS_TOKEN(SWITCH); return(SWITCH); }
<INITIAL>"while"    { SETPOS_TOKEN(WHILE); return(WHILE); }

  /* Microsoft Extensions */
<INITIAL>"__asm"      { BEGIN(MSC_ASM_ELIM); }
<INITIAL>"__declspec" { BEGIN(PAREN_ELIM); }

  /* GCC2 Extensions */
<INITIAL>"asm"        { BEGIN(PAREN_ELIM); }
<INITIAL>"__asm__"    { BEGIN(PAREN_ELIM); }

<PAREN_ELIM>"("[^()]*   { global_parse_env->gcc2_paren_cnt++; }
<PAREN_ELIM>[^()]*      { /* Throw away */ }
<PAREN_ELIM>")"         { if (--global_parse_env->gcc2_paren_cnt == 0)
                          BEGIN(INITIAL);
                      }

<MSC_ASM_ELIM>[^}]*   { /* Throw away */ }
<MSC_ASM_ELIM>"}"     { BEGIN(INITIAL); }

<INITIAL>"__attribute"    { BEGIN(GCC_ATTRIB); return(global_parse_env->err_tok = ATTRIBUTE); }
<INITIAL>"__attribute__"  { BEGIN(GCC_ATTRIB); return(global_parse_env->err_tok = ATTRIBUTE); }

<GCC_ATTRIB>"aligned"     { BEGIN(INITIAL); return(global_parse_env->err_tok = ALIGNED); }
<GCC_ATTRIB>"__aligned"   { BEGIN(INITIAL); return(global_parse_env->err_tok = ALIGNED); }
<GCC_ATTRIB>"__aligned__" { BEGIN(INITIAL); return(global_parse_env->err_tok = ALIGNED); }
<GCC_ATTRIB>"packed"      { BEGIN(INITIAL); return(global_parse_env->err_tok = PACKED); }
<GCC_ATTRIB>"__packed"    { BEGIN(INITIAL); return(global_parse_env->err_tok = PACKED); }
<GCC_ATTRIB>"__packed__"  { BEGIN(INITIAL); return(global_parse_env->err_tok = PACKED); }
<GCC_ATTRIB>"cdecl"       { BEGIN(INITIAL); return(global_parse_env->err_tok = CDECL); }
<GCC_ATTRIB>"__cdecl"     { BEGIN(INITIAL); return(global_parse_env->err_tok = CDECL); }
<GCC_ATTRIB>"__cdecl__"   { BEGIN(INITIAL); return(global_parse_env->err_tok = CDECL); }

<GCC_ATTRIB>{whitespace}+ {  /* space/tab/formfeed/vertical tab (ignore) */ }
<GCC_ATTRIB>"("           { SETPOS_TOKEN(LPAREN); return(LPAREN); }

<GCC_ATTRIB>"format"      { BEGIN(INITIAL); return(global_parse_env->err_tok = FORMAT); }
<GCC_ATTRIB>"__format"    { BEGIN(INITIAL); return(global_parse_env->err_tok = FORMAT); }
<GCC_ATTRIB>"__format__"  { BEGIN(INITIAL); return(global_parse_env->err_tok = FORMAT); }
<GCC_ATTRIB>"mode"        { BEGIN(INITIAL); return(global_parse_env->err_tok = MODE); }
<GCC_ATTRIB>"__mode"      { BEGIN(INITIAL); return(global_parse_env->err_tok = MODE); }
<GCC_ATTRIB>"__mode__"    { BEGIN(INITIAL); return(global_parse_env->err_tok = MODE); }
<GCC_ATTRIB>"const"       { BEGIN(INITIAL); return(global_parse_env->err_tok = CONST); }
<GCC_ATTRIB>"__const"     { BEGIN(INITIAL); return(global_parse_env->err_tok = CONST); }
<GCC_ATTRIB>"__const__"   { BEGIN(INITIAL); return(global_parse_env->err_tok = CONST); }
<GCC_ATTRIB>"noreturn"    { BEGIN(INITIAL); return(global_parse_env->err_tok = NORETURN); }
<GCC_ATTRIB>"__noreturn"  { BEGIN(INITIAL); return(global_parse_env->err_tok = NORETURN); }
<GCC_ATTRIB>"__noreturn__" { BEGIN(INITIAL); return(global_parse_env->err_tok = NORETURN); }
<GCC_ATTRIB>"__malloc__" { BEGIN(INITIAL); return(global_parse_env->err_tok = MALLOC); }
<GCC_ATTRIB>"__transparent_union__" { BEGIN(INITIAL); return(global_parse_env->err_tok = TRANSPARENT_UNION); }
<GCC_ATTRIB>"__pure__" { BEGIN(INITIAL); return(global_parse_env->err_tok = PURE); }

<INITIAL>"__const"  {
                      lvalp->typeQual = TypeQual::Const;
                      return(global_parse_env->err_tok = CONST);
                    }

<INITIAL>"__const__"  {
                        lvalp->typeQual = TypeQual::Const;
                        return(global_parse_env->err_tok = CONST);
                      }
<INITIAL>"__signed" {
                      lvalp->type = new BaseType(BaseType::Signed);
                      return(global_parse_env->err_tok = SGNED);
                    }
<INITIAL>"__signed__" {
                        lvalp->type = new BaseType(BaseType::Signed);
                        return(global_parse_env->err_tok = SGNED);
                      }
<INITIAL>"__volatile" {
                        lvalp->typeQual = TypeQual::Volatile;
                        return(global_parse_env->err_tok = VOLATILE);
                      }
<INITIAL>"__volatile__" {
                          lvalp->typeQual = TypeQual::Volatile;
                          return(global_parse_env->err_tok = VOLATILE);
                        }

<INITIAL>"typeof"      { }
<INITIAL>"__typeof"    { }
<INITIAL>"__typeof__"  { }

<INITIAL>"inline"      { }
<INITIAL>"__inline"    { }
<INITIAL>"__inline__"  { }

<INITIAL>"__extension__"  { }

<INITIAL>"alignof"      { }
<INITIAL>"__alignof"    { }
<INITIAL>"__alignof__"  { }

<INITIAL>"__imag"     { }
<INITIAL>"__imag__"   { }
<INITIAL>"__real"     { }
<INITIAL>"__real__"   { }
<INITIAL>"__complex"    { }
<INITIAL>"__complex__"  { }
<INITIAL>"__iterator"   { }
<INITIAL>"__iterator__" { }
<INITIAL>"__label__"    { }

<INITIAL>"_cdecl"       { /* Windows only non-standard C crud */ }
<INITIAL>"__cdecl"      { /* Windows only non-standard C crud */ }
<INITIAL>"_stdcall"     { /* Windows only non-standard C crud */ }
<INITIAL>"__stdcall"    { /* Windows only non-standard C crud */ }
<INITIAL>"_fastcall"    { /* Windows only non-standard C crud */ }
<INITIAL>"__fastcall"   { /* Windows only non-standard C crud */ }


<INITIAL>"+"        { lvalp->binOp = BO_Plus; return(PLUS); }
<INITIAL>"-"        { lvalp->binOp = BO_Minus; return(MINUS); }
<INITIAL>"*"        { lvalp->binOp = BO_Mult; return(STAR); }
<INITIAL>"/"        { lvalp->binOp = BO_Div; return(DIV); }
<INITIAL>"%"        { lvalp->binOp = BO_Mod; return(MOD); }

<INITIAL>"+="       { lvalp->assignOp = AO_PlusEql; return(ASSIGN); }
<INITIAL>"-="       { lvalp->assignOp = AO_MinusEql; return(ASSIGN); }
<INITIAL>"*="       { lvalp->assignOp = AO_MultEql; return(ASSIGN); }
<INITIAL>"/="       { lvalp->assignOp = AO_DivEql; return(ASSIGN); }
<INITIAL>"%="       { lvalp->assignOp = AO_ModEql; return(ASSIGN); }

<INITIAL>"!"        { SETPOS_TOKEN(NOT); return(NOT); }
<INITIAL>"&&"       { SETPOS_TOKEN(AND); return(AND); }
<INITIAL>"||"       { SETPOS_TOKEN(OR); return(OR); }

<INITIAL>"~"        { SETPOS_TOKEN(B_NOT); return(B_NOT); }
<INITIAL>"&"        { SETPOS_TOKEN(B_AND); return(B_AND); }
<INITIAL>"|"        { SETPOS_TOKEN(B_OR); return(B_OR); }
<INITIAL>"^"        { SETPOS_TOKEN(B_XOR); return(B_XOR); }

<INITIAL>"&="       { lvalp->assignOp = AO_BitAndEql; return(ASSIGN); }
<INITIAL>"|="       { lvalp->assignOp = AO_BitOrEql; return(ASSIGN); }
<INITIAL>"^="       { lvalp->assignOp = AO_BitXorEql; return(ASSIGN); }

<INITIAL>"<<"       { lvalp->binOp = BO_Shl; return(L_SHIFT); }
<INITIAL>">>"       { lvalp->binOp = BO_Shr; return(R_SHIFT); }
<INITIAL>"<<="      { lvalp->assignOp = AO_ShlEql; return(ASSIGN); }
<INITIAL>">>="      { lvalp->assignOp = AO_ShrEql; return(ASSIGN); }

<INITIAL>"=="       { lvalp->relOp = RO_Equal; return(COMP_EQ); }
<INITIAL>"<"        { lvalp->relOp = RO_Less; return(COMP_ARITH); }
<INITIAL>"<="       { lvalp->relOp = RO_LessEql; return(COMP_ARITH); }
<INITIAL>">"        { lvalp->relOp = RO_Grtr; return(COMP_ARITH); }
<INITIAL>">="       { lvalp->relOp = RO_GrtrEql; return(COMP_ARITH); }
<INITIAL>"!="       { lvalp->relOp = RO_NotEqual; return(COMP_EQ); }

<INITIAL>"="        { lvalp->assignOp = AO_Equal; return(EQ); }
<INITIAL>"++"       { SETPOS_TOKEN(INCR); return(INCR); }
<INITIAL>"--"       { SETPOS_TOKEN(DECR); return(DECR); }

<INITIAL>"("        { SETPOS_TOKEN(LPAREN);
                      global_parse_env->possibleType = true;
                      return(LPAREN);
                    }
<INITIAL>")"        { SETPOS_TOKEN(RPAREN); return(RPAREN); }
<INITIAL>"["        { SETPOS_TOKEN(LBRCKT); return(LBRCKT); }
<INITIAL>"]"        { SETPOS_TOKEN(RBRCKT); return(RBRCKT); }
<INITIAL>"{"        { SETPOS_TOKEN(LBRACE); 
                      global_parse_env->possibleTag = false;
                      return(LBRACE); }
<INITIAL>"}"        { SETPOS_TOKEN(RBRACE); return(RBRACE); }

<INITIAL>"."        { SETPOS_TOKEN(DOT); return(DOT); }
<INITIAL>"->"       { SETPOS_TOKEN(ARROW); return(ARROW); }

<INITIAL>"?"        { SETPOS_TOKEN(QUESTMARK); return(QUESTMARK); }
<INITIAL>":"        { SETPOS_TOKEN(COLON); return(COLON); }
<INITIAL>";"        { SETPOS_TOKEN(SEMICOLON); return(SEMICOLON); }
<INITIAL>","        { SETPOS_TOKEN(COMMA); return(COMMA); }
<INITIAL>"..."      { SETPOS_TOKEN(ELLIPSIS); return(ELLIPSIS); }
<INITIAL>"__restrict" { /* ignore */} /* GNU extension */

	/* Unused (invalid) characters */
<INITIAL>"`"        { SETPOS_TOKEN(BACKQUOTE); return(BACKQUOTE); }
<INITIAL>"@"        { SETPOS_TOKEN(AT); return(AT); }

	/* Preprocessor Stuff */
<INITIAL>{pp_strt}\n       { global_parse_env->yylineno++;
                             global_parse_env->yynxtcol = 0; }

<INITIAL>{pp_strt}"line"{whitespace}* { BEGIN(PPLN); }
<INITIAL>{pp_strt}"file"{whitespace}* { global_parse_env->file_ppln=true; BEGIN(PPLN); }
<INITIAL>{pp_strt}{digit} { BEGIN(PPLN); yyless(1); }
<INITIAL>{pp_strt} { BEGIN_PP(); }

<PPLN>[^\n]*	{ 
                       BEGIN_PP();
                       if (get_lineno(global_parse_env->file_ppln,yytext,&(lvalp->stemnt)))
							return (global_parse_env->err_tok = PP_LINE);
                }

<PP>\\(\n)             {  /* Preprocessor continuation line */
                         global_parse_env->yylineno++;
                         global_parse_env->yynxtcol = 0;
                       }

<PP>\n                 {  /* End of this preprocessor logical line */
                         global_parse_env->file_ppln=false;
                         global_parse_env->in_preprocess = 0;
                         yyless(0);        /* Hack! */
                         BEGIN(INITIAL);
                       }

<PP>[^\n\\]+           { /* Swallow cpp junk to prevent it being echo'd */ }

<INITIAL>{alpha}{alphanum}*  {    /* Identifier */

                         lvalp->symbol = new Symbol();
                       
                         lvalp->symbol->name = yytext;

                         if (global_parse_env->possibleTag)
                         {
                           /* Check the tag table and insert a  */
                           /* ptr back to the definition.  */
                           SymEntry *se
                              = global_parse_env->transUnit->my_contxt.tags->Lookup(
                                    lvalp->symbol->name);
                           lvalp->symbol->entry = se;
                           global_parse_env->possibleTag = false;
                           return(global_parse_env->err_tok = TAG_NAME);
                         }
                         else if (global_parse_env->possibleLabel)
                         {
                           /* Check the tag table and insert a  */
                           /* ptr back to the definition.  */
                           SymEntry *se
                              = global_parse_env->transUnit->my_contxt.labels->LookupAt(
                                    lvalp->symbol->name,FUNCTION_SCOPE);
                           lvalp->symbol->entry = se;
                           if (!se)
                               global_parse_env->transUnit->my_contxt.labels
                                   ->InsertAt(lvalp->symbol->entry = mk_label(lvalp->symbol->name, NULL), 
                                       FUNCTION_SCOPE);

                           global_parse_env->possibleLabel = false;
                           return(global_parse_env->err_tok = LABEL_NAME);
                         }
                         else {
                           /* Check the symbol table and insert a  */
                           /* ptr back to the definition.  */
                           SymEntry *se
                              = global_parse_env->transUnit->my_contxt.syms->Lookup(
                                    lvalp->symbol->name);
                           lvalp->symbol->entry = se;
 
                            if (global_parse_env->possibleType)
                            {
                              if (se && se->IsTypeDef())
                              {
                                  return(global_parse_env->err_tok = TYPEDEF_NAME);
                              }
                            }
                         }

                         return(global_parse_env->err_tok = IDENT);
                       }

<INITIAL>{octnum}  |
<INITIAL>{intnum}  |
<INITIAL>{hexnum}   {  /* An integer */

                         bool has_L=strchr(yytext, 'L') || strchr(yytext, 'l');
                         bool has_U=strchr(yytext, 'U') || strchr(yytext, 'u');

                         if(has_U)
                         {
                              ulong ui;
                              ui = strtoul(yytext, (char**) NULL, 0);
                              lvalp->consValue = new UIntConstant(ui,has_L,HERE);
                         }
                         else
                         {
                           errno=0; // reset errno

                           long i = strtol(yytext, (char**) NULL, 0);

                           if (errno == ERANGE)
                           {
                              ulong ui;
                              ui = strtoul(yytext, (char**) NULL, 0);
                              lvalp->consValue = new UIntConstant(ui,has_L,HERE);
                           }
                           else
                           { 
                               lvalp->consValue = new IntConstant(i,has_L,HERE);
                           } 
                         }

                         return(global_parse_env->err_tok = INUM);
                    }
          
<INITIAL>{digits}{dot}{digits}{exponent}?{floatsuffix}? |
<INITIAL>{digits}{dot}{exponent}?{floatsuffix}?         |
<INITIAL>{dot}{digits}{exponent}?{floatsuffix}?         |
<INITIAL>{digits}{exponent}{floatsuffix}? {
                         /*
                         ** Note: The floatsuffix, if any, will be
                         ** ignored by atof().
                         */
                         lvalp->consValue = new FloatConstant(yytext,HERE);
                         return(global_parse_env->err_tok = RNUM);
                       }

<STR>\"{allwhite}*\"   {   /* String Pasting */
                         cnt_nl(yytext,yyleng);
                       }

<STR>\"                {   /* Closing quote */
                         LAST_STATE();
                         *global_parse_env->tokn_ptr = '\0';
                         lvalp->consValue =
                           new StringConstant(std::string(global_parse_env->tokn_buff,
                             global_parse_env->tokn_ptr-global_parse_env->tokn_buff),
                             HERE,global_parse_env->isWide);
                         return(global_parse_env->err_tok = STRING);
                       }

<STR>\n                { /* Error - unterminated string constant */
                         if(yyerr("Unterminated string constant starting"))
                           yyterminate();

                         LAST_STATE();
                         return(INVALID);
                       }

<STR>\\[0-7]+          { /* octal escape sequence */
                         uint result;
                         char tmpOct[20];

                         strncpy(tmpOct,yytext,yyleng);
                         tmpOct[yyleng] = '\0';
                         tmpOct[4] = '\0';

                         if (yyleng > 4)
                             yyless(4);

                         if (sscanf(tmpOct+1, "%o", &result ) < 1)
                         {
                             if(yyerr("Invalid octal escape sequence"))
                               yyterminate();

                             LAST_STATE();
                             return(INVALID);
                         }

                             /* error, constant is out-of-bounds */
                         if ( result > 0xff ){
                            if(yyerr("Escape sequence out-of-bounds"))
                              yyterminate();
                         }

                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                            *global_parse_env->tokn_ptr++ = result;
                         else
                            yywarn("String constant too long");
                       }

<STR>\\x{hexdigit}+    {   /* hex escape sequence - ISO C */
                         uint result;

                         if (sscanf(yytext+2, "%x", &result ) < 1)
                         {
                             if(yyerr("Invalid hex escape sequence"))
                               yyterminate();

                             LAST_STATE();
                             return(INVALID);
                         }

                             /* error, constant is out-of-bounds */
                         if ( result > 0xff ){
                            if(yyerr("Escape sequence out-of-bounds"))
                              yyterminate();
                         } else if (yyleng > 4){
                            yywarn("Hexadecimal escape exceeds two chars");
                         }

                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                            *global_parse_env->tokn_ptr++ = result;
                         else
                            yywarn("String constant too long");
                       }

<STR>\\{digits}        {  /* Bad escape sequence */
                         yywarn("Bad escape sequence in string");
                        *global_parse_env->tokn_ptr++ = '?';
                       }

<STR>\\n               {
                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                           *global_parse_env->tokn_ptr++ = '\n';
                         else
                            yywarn("String constant too long");
                       }

<STR>\\t               {
                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                           *global_parse_env->tokn_ptr++ = '\t';
                         else
                            yywarn("String constant too long");
                       }

<STR>\\r               {
                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                           *global_parse_env->tokn_ptr++ = '\r';
                         else
                            yywarn("String constant too long");
                       }

<STR>\\b               {
                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                           *global_parse_env->tokn_ptr++ = '\b';
                         else
                            yywarn("String constant too long");
                       }

<STR>\\f               {
                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                           *global_parse_env->tokn_ptr++ = '\f';
                         else
                            yywarn("String constant too long");
                       }

<STR>\\v               {
                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                           *global_parse_env->tokn_ptr++ = '\v';
                         else
                            yywarn("String constant too long");
                       }

<STR>\\a               {
                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                           *global_parse_env->tokn_ptr++ = '\a';
                         else
                            yywarn("String constant too long");
                       }

<STR>\\e               {
                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                           *global_parse_env->tokn_ptr++ = ESC_VAL;
                         else
                            yywarn("String constant too long");
                       }

<STR>\\(\n)            {  /* String continuation */
                         global_parse_env->yylineno++;
                         global_parse_env->yynxtcol = 0;
                       }

<STR>\\.               {
                         if (global_parse_env->tokn_ptr < global_parse_env->tokn_end)
                           *global_parse_env->tokn_ptr++ = yytext[1];
                         else
                            yywarn("String constant too long");
                       }

<STR>[^\\\n\"]+        {
                         int n = yyleng;
                         if (global_parse_env->tokn_ptr + n < global_parse_env->tokn_end) {
                             memcpy(global_parse_env->tokn_ptr,yytext,n);
                             global_parse_env->tokn_ptr += n;
                         } else 
                            yywarn("String constant too long");
                       }

<CMMT>[^*\n/\\]*       {   /* Inside C-style comment */ }

<CMMT>[^*\n/\\]*\n     {
                         global_parse_env->yylineno++;
                         global_parse_env->yynxtcol = 0;
                         global_parse_env->in_preprocess = 0;
                       }
<CMMT>"/"[^*\n]        { }
<CMMT>\\\n             {
                         global_parse_env->yylineno++;
                         global_parse_env->yynxtcol = 0;
                       }
<CMMT>\\[^\n]          {
                       }
<CMMT>"/"\n            {
                         global_parse_env->yylineno++;
                         global_parse_env->yynxtcol = 0;
                         global_parse_env->in_preprocess = 0;
                       }
<CMMT>"/*"             { yywarn("/* inside comment"); }
<CMMT>"*"+[^*/\n\\]*   {   /* Stars */ }
<CMMT>"*"+[^*/\n\\]*\n {
                         global_parse_env->yylineno++;
                         global_parse_env->yynxtcol = 0;
                         global_parse_env->in_preprocess = 0;
                       }
<CMMT>"*"+"/"          {
                         LAST_STATE();
                       }

<CC>\\[0-7]{1,3}"'"    {   /* octal escape sequence */
                         uint result;

                         if (sscanf(yytext+1, "%o", &result ) < 1)
                         {
                             if(yyerr("Invalid octal escape sequence"))
                               yyterminate();

                             LAST_STATE();
                             return(INVALID);
                         }
                         
                             /* error, constant is out-of-bounds */
                         if ( result > 0xff ){
                             yywarn("Escape sequence out-of-bounds");
                             result = 0;
                         }

                         lvalp->consValue = new CharConstant((char) result, HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\x{hexdigit}+"'"  {   /* hex escape sequence - ISO C */
                         uint result;

                         if (sscanf(yytext+2, "%x", &result ) < 1)
                         {
                             if(yyerr("Invalid hex escape sequence"))
                               yyterminate();

                             LAST_STATE();
                             return(INVALID);
                         }

                             /* error, constant is out-of-bounds */
                         if ( result > 0xff ){
                             yywarn("Escape sequence out-of-bounds");
                             result = 0;
                         } else if (yyleng > 5){
                             yywarn("Hexadecimal escape exceeds two chars");
                             result = 0;
                         }

                         lvalp->consValue = new CharConstant((char) result, HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\{digits}"'"      {  /* Bad escape sequence */
                         yywarn("Bad escape sequence in char constant");
                         lvalp->consValue = new CharConstant('\0',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\n                 { /* Error unterminated char constant */
                         global_parse_env->yylineno++;
                         global_parse_env->yynxtcol = 0;
                         yywarn("Unterminated char constant");
                         lvalp->consValue = new CharConstant('\0',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\n"'"             {
                         lvalp->consValue = new CharConstant('\n',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\t"'"             {
                         lvalp->consValue = new CharConstant('\t',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\r"'"             {
                         lvalp->consValue = new CharConstant('\r',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\b"'"             {
                         lvalp->consValue = new CharConstant('\b',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\f"'"             {
                         lvalp->consValue = new CharConstant('\f',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\v"'"             {
                         lvalp->consValue = new CharConstant('\v',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\a"'"             {
                         lvalp->consValue = new CharConstant('\a',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\e"'"             {
                         lvalp->consValue = new CharConstant(ESC_VAL,HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\\n"'"            {
                         global_parse_env->yylineno++;
                         global_parse_env->yynxtcol = 1;  /* Oddity */
                         lvalp->consValue = new CharConstant('\n',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>\\."'"             {
                         lvalp->consValue = new CharConstant(yytext[1],HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>[^']"'"            {
                         lvalp->consValue = new CharConstant(*yytext,HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>"'"                { /* Empty */
                         yywarn("Empty character constant");
                         lvalp->consValue = new CharConstant('\0',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<CC>[^\\\n][^']+"'"    { /* Multiple characters */
                         yywarn("Invalid character constant");
                         lvalp->consValue = new CharConstant('\0',HERE,global_parse_env->isWide);
                         LAST_STATE();
                         return(global_parse_env->err_tok = CHAR_CONST);
                       }

<INITIAL>{whitespace}+ {  /* space/tab/formfeed/vertical tab (ignore) */ }

<INITIAL,CMMT>\n|\r    { global_parse_env->yylineno++;
                         global_parse_env->yynxtcol = 0;
                         global_parse_env->in_preprocess = 0;
                       }

<CMMT><<EOF>>          { 
                         yyerr("EOF reached inside comment");
                         global_parse_env->in_preprocess = 0;
                         return(0);
                       }

<CC><<EOF>>            { 
                         yyerr("EOF reached inside character constant");
                         return(0);
                       }

<STR><<EOF>>           { 
                         yyerr("EOF reached inside string constant");
                         return(0);
                       }

.                      { /* Any unknown char is an error */
                        *global_parse_env->yyerrstream
                           << "Error: Line " << global_parse_env->yylineno
                           << ": Illegal Character";
                        if ((yytext[0] >= ' ') && (yytext[0] <= '~'))
                          *global_parse_env->yyerrstream
                           << ": '" << yytext[0] << "'\n";
                        else
                          *global_parse_env->yyerrstream
                           << ", ASCII: " << std::oct << (int)(yytext[0]) 
                                 << " (octal)\n" << std::dec;
                       }

%%

/************************************************************************
//
// yywrap() - This function is called by the lexer [yylex() or sslex()] when
//            the end-of-file (or end-of-string for sslex()) is reached.
//            It gets the next file/string ready (if any) and returns 1
//            which indicates to the lexer that there are no more files
//            remaining.  The lexer returns 0 to the parser, indicating
//            no more tokens are remaining. The parser function, yyparse(),
//            then returns 0, indicating that the parse is done.  This
//            behaviour allows a single item to be parsed to be placed
//            on the stack and parsed without the entire stack being consumed.
//
// **********************************************************************/

int yywrap(void)
{
    global_parse_env->in_preprocess = 0;
    return 1;
}

/***********************************************************************/
int cnt_nl(char *txt, int len)
{
  int ret = 0;

  do {
    switch (*txt++) {
      case '\n':
        ret++;
        global_parse_env->yylineno++;
        global_parse_env->yynxtcol = 0;
        break;

      default:
        break;
    }
  } while(--len > 0);

  return ret;
}

/***********************************************************************/
/*  Process a #line directive */
int
get_lineno(bool file_ppln, char *txt, Statement **stement)
{
    int   ln = 0;
    *stement = NULL ;
    int   oldyylineno = global_parse_env->yylineno;
    do {
        if (txt)
        {
            if (file_ppln)
                ln = 0;
            else
            {
                if (sscanf(txt,"%d", &ln) < 1)
                    break;

                global_parse_env->yylineno = ln - 1;
            }
           
            txt = strchr(txt,'"');

            if (txt)
            {
                char *pnt_end;
    
                txt++;

                pnt_end = strchr(txt,'"');
                if (pnt_end)
                    *pnt_end = '\0';
                else
                    break;
                        
                int entered_or_exited;
                if (sscanf(pnt_end+1,"%d", &entered_or_exited) < 1)
                    entered_or_exited = -1;
               
                switch (entered_or_exited)
				{
                  case 2:
                      // new line number into the previous included file
                      global_parse_env->yylineno = oldyylineno;
                      #if 1 // whole idea is flawed
                      *stement = new EndInclStemnt(HERE);
                      #endif
                      if (! file_ppln)
                          global_parse_env->yylineno = ln - 1;
                      global_parse_env->filename = txt;
                      return (*stement != NULL);
                      
                  case 1:
                      // new line number into a new include file
                      global_parse_env->yylineno = oldyylineno;
                      if (! file_ppln)
                          global_parse_env->yylineno = ln - 1;
                      global_parse_env->filename = txt;
                      #if 1 // whole idea is flawed
                      *stement = new InclStemnt(global_parse_env->filename, HERE);
                      #endif
                      return (*stement != NULL);
                    
                  case 3:
                      // new line number introduced by the gcc preprocessor 
                      // into the same file
                      if (global_parse_env->filename == txt && 
                          ln > oldyylineno)
                      {
                          global_parse_env->yylineno = ln - 1;
                          return 0;
                      }
                      /* No break */
                  default:
                      yywarn("Malformed #line directive");
                      /* No break */
                  case -1:
                      // new line number into the same file
                      global_parse_env->yylineno = oldyylineno;
                      global_parse_env->filename = txt;
                      #if 0 // whole idea is flawed
                      *stement = new FileLineStemnt(global_parse_env->filename, ln, HERE);
                      #endif
                      if (! file_ppln)
                          global_parse_env->yylineno = ln - 1;
                      return (*stement != NULL);
                  }
              }
          }

        return 0;
      } while (0);

    yywarn("Malformed #line directive");
    return 0;
}

/***********************************************************************/

void print_caret(const std::string &s, int pos)
{
    const char *str;
    int i = 0;

    for (str=s.c_str(); *str && (i<pos); i++, str++)
        if (*str=='\t')
            *global_parse_env->yyerrstream << "\t";
        else
            *global_parse_env->yyerrstream << " ";

    *global_parse_env->yyerrstream << "^\n";
}
 
/***********************************************************************/

void yywarn(char *s)
{
    *global_parse_env->yyerrstream
         << global_parse_env->filename << ":" << global_parse_env->yylineno
         << ": Warning - " << s << ":\n" << global_parse_env->line;
    print_caret(global_parse_env->line, global_parse_env->yycolno);
}

/***********************************************************************/

int yyerr(char *s, const std::string & str)
{
    *global_parse_env->yyerrstream
      << global_parse_env->filename << ":" << global_parse_env->yylineno
      << ": " << s << str << ":\n" << global_parse_env->line;
    print_caret(global_parse_env->line, global_parse_env->yycolno);

    ++global_parse_env->err_top_level;
    if (++global_parse_env->err_cnt >= 10){
        *global_parse_env->yyerrstream
          << "Too many errors (" << global_parse_env->err_cnt << " detected) - quitting.\n";
        return(1);
    }
    return(0); 
}

/***********************************************************************/

int yyerr(char *s)
{
    *global_parse_env->yyerrstream
      << global_parse_env->filename << ":" << global_parse_env->yylineno
      << ": " << s << ":\n" << global_parse_env->line;
    print_caret(global_parse_env->line, global_parse_env->yycolno);

    ++global_parse_env->err_top_level;
    if (++global_parse_env->err_cnt >= 10){
        *global_parse_env->yyerrstream
          << "Too many errors (" << global_parse_env->err_cnt << " detected) - quitting.\n";
        return(1);
    }
    return(0); 
}

/***********************************************************************/

int yyerror(char *s)
{
    *global_parse_env->yyerrstream
      << global_parse_env->filename << ":" << global_parse_env->yylineno
      << ": Error (" << s << ") before '"
      << toksym(global_parse_env->err_tok,0) << "'\n" << global_parse_env->line;
    print_caret(global_parse_env->line, global_parse_env->yycolno);

    if (++global_parse_env->err_cnt >= 10){
        *global_parse_env->yyerrstream
          << "Too many errors (" << global_parse_env->err_cnt << " detected) - quitting.\n";
        return(1);
    }
    return(0);
}

/***********************************************************************/
/* created 11/6/99 */
void function_to_eliminate_gcc_warning()
{
  yyunput(0,"");
}

/***********************************************************************/
