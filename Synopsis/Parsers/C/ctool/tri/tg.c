
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

/*  Trigraphs are an annoying way to give people with international
    keyboards access to all the tokens required for ISO C.          */

/*  The trigraphs are:
    ??(    [
    ??)    ]
    ??<    {
    ??>    }
    ??/    \
    ??!    |
    ??'    ^
    ??-    ~
    ??=    #

    There are also respellings available for some of the more 
    commonly used trigraphs, these are referred to as digraphs,
    and they are not replaced within literal strings:

    <:    [
    :>    ]
    %:    #
    <%    {
    %>    }
    %:%;  ## 

*/
/*  ###############################################################  */

#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>
#include    <assert.h>

/*  ###############################################################  */
/*  
 *   The elimination of trigraphs and respellings can only make
 *   a line shorter, so do the converstion in-place and return
 *   a pointer to the string you recieved.
 */
/*  ###############################################################  */

char *convert_trigraphs(char *tri_buff)
{
    char *from, *to;
    int len;

    if (tri_buff){

        from = tri_buff;
        len = strlen(tri_buff);

        do {

            for (; *from; from++)
                if ( (*from == '?') || (*from == '%')
                        || (*from == '<') || (*from == ':') )
                    break;

            for (to=from; *from; from++,to++){
    
                switch (*from){

                case '?':
                    if (*(from+1) != '?'){
                        *to = *from;
                        break;
                    }
                    switch (*(from+2)){

                    case '(':
                        *to = '[';
                        from += 2;
                        len -= 2;
                        break;

                    case ')':
                        *to = ']';
                        from += 2;
                        len -= 2;
                        break;

                    case '<':
                        *to = '{';
                        from += 2;
                        len -= 2;
                        break;

                    case '>':
                        *to = '}';
                        from += 2;
                        len -= 2;
                        break;

                    case '/':
                        *to = '\';
                        from += 2;
                        len -= 2;
                        break;

                    case '!':
                        *to = '|';
                        from += 2;
                        len -= 2;
                        break;

                    case '\'':
                        *to = '^';
                        from += 2;
                        len -= 2;
                        break;

                    case '-':
                        *to = '~';
                        from += 2;
                        len -= 2;
                        break;

                    case '=':
                        *to = '#';
                        from += 2;
                        len -= 2;
                        break;

                    default:
                        *to = *from;
                        break;
                    }
                    break;

                case '%':
                    if (*(from+1) == ':'){
                        *to = '#';
                        from++;
                        len--;
                    } else if (*(from+1) == '>'){
                        *to = '}';
                        from++;
                        len--;
                    } else
                        *to = *from;
                    break;

                case '<':
                    if (*(from+1) == ':'){
                        *to = '[';
                        from++;
                        len--;
                    } else if (*(from+1) == '%'){
                        *to = '{';
                        from++;
                        len--;
                    } else
                        *to = *from;
                    break;

                case ':':
                    if (*(from+1) == '>'){
                        *to = ']';
                        from++;
                        len--;
                    } else
                        *to = *from;
                    break;

                default:
                    *to = *from;
                    break;
                }

                if (to == from)
                    break;
            }

            if ((to == from) && *from){
                to++, from++;
                continue;
            }

            break;
        } while(1);

        tri_buff[len] = '\0';
    }

    return(tri_buff);
}

/*  ###############################################################  */
