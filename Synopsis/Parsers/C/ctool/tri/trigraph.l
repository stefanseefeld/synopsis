
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

#undef YY_INPUT

#    ifdef STRING_PARSE

#define    min(X,Y)    ((X)<(Y) ? (X) : (Y))

    /* Scan input from a string instead */
#define YY_INPUT(buf,result,max_size) \
  do { \
     int n = min(max_size, parseEnv->parselimit - parseEnv->parseptr);\
     if (n>0){ \
        memcpy(buf,parseEnv->parseptr,(unsigned int) n); \
        parseEnv->parseptr += n; \
        result = n; \
     } else \
        result = YY_NULL; \
  } while(0)

#    else    /* not STRING_PARSE */

    /* Scan input from a file pointer (default) */
#define YY_INPUT(buf,result,max_size) \
    do { \
        if (fgets(buf,max_size,yyin) == NULL) { \
           result = YY_NULL; \
        } else { \
           result = strlen(buf); \
        } \
    } while(0)

#    endif    /* not STRING_PARSE */

#if 0
EXTERN int yylex ARGS((YYSTYPE *lvalp));

/*  To give some args to yylex */
#undef YY_DECL
#define YY_DECL int yylex YY_PROTO((YYSTYPE *lvalp))
#endif

%}
%x Start STR
%a 9000
%o 9000

tg              "??"

%%

<INITIAL,STR>{tg}"("        { fputs("[",stdout); }
<INITIAL,STR>{tg}")"        { fputs("]",stdout); }
<INITIAL,STR>{tg}"<"        { fputs("{",stdout); }
<INITIAL,STR>{tg}">"        { fputs("}",stdout); }
<INITIAL,STR>{tg}"/"        { fputs("\\",stdout); }
<INITIAL,STR>{tg}"!"        { fputs("|",stdout); }
<INITIAL,STR>{tg}"'"        { fputs("^",stdout); }
<INITIAL,STR>{tg}"-"        { fputs("~",stdout); }
<INITIAL,STR>{tg}"="        { fputs("#",stdout); }

<INITIAL>\"    { fputs("\"",stdout); BEGIN(STR); }
<STR>\"        { fputs("\"",stdout); BEGIN(INITIAL); }
<STR>\\\"      { fputs("\\\"",stdout); }
<STR>\\\\      { fputs("\\",stdout); }

<INITIAL>"<:"           { fputs("[",stdout); }
<INITIAL>":>"           { fputs("]",stdout); }
<INITIAL>"<%"           { fputs("{",stdout); }
<INITIAL>"%>"           { fputs("}",stdout); }
<INITIAL>"%:"           { fputs("#",stdout); }

%%

/***********************************************************************/
