
/*  ###############################################################
    ##
    ##     C Tree Builder
    ##
    ##     File:         tree.h
    ##
    ##     Programmer:   Shawn Flisakowski
    ##     Date:         Jan 21, 1995
    ##
    ##
    ###############################################################  */

#ifndef   TREE_H
#define   TREE_H

#include  "config.h"
#include  "nmetab.h"

BEGIN_HEADER

/*  ###############################################################  */

typedef enum {

    TN_EMPTY,

    TN_FUNC_DEF,
    TN_FUNC_DECL,
    TN_FUNC_CALL,
    TN_BLOCK,
    TN_DECL,
    TN_ARRAY_DECL,

    TN_TRANS_LIST,
    TN_DECL_LIST,
    TN_STEMNT_LIST,
    TN_EXPR_LIST,
    TN_NAME_LIST,
    TN_ENUM_LIST,
    TN_FIELD_LIST,
    TN_PARAM_LIST,
    TN_IDENT_LIST,

    TN_DECLS,

        /* Dumb, but I'm not sure what to do yet */
    TN_COMP_DECL,
    TN_BIT_FIELD,
    TN_PNTR,

            /* Stores a list of type specifiers/qualifers (int,const,etc) */
    TN_TYPE_LIST,
    TN_TYPE_NME,

            /* Stores initial values for arrays */
    TN_INIT_LIST,
    TN_INIT_BLK,

    TN_OBJ_DEF,    /* Definition of struct, union, or enum */
    TN_OBJ_REF,    /* Reference to struct, union, or enum */

        /* More vague */
    TN_CAST,
    TN_IF,
    TN_ASSIGN,
    TN_JUMP,
    TN_FOR,
    TN_WHILE,
    TN_DOWHILE,
    TN_SWITCH,
    TN_LABEL,
    TN_STEMNT,

    TN_INDEX,     /* Index with [] */
    TN_ADDROF,    /* Address of via & */
    TN_DEREF,     /* Dereference with * */
    TN_SELECT,    /* -> and . */

    TN_EXPR,
    TN_COND_EXPR,

    TN_COMMENT,
    TN_CPP,

    TN_ELLIPSIS,
    TN_IDENT,
    TN_TYPE,
    TN_STRING,
    TN_INT,
    TN_REAL

} tn_t;


typedef enum {

    NONE_T,
    LEAF_T,
    IF_T,
    FOR_T,
    NODE_T

} node_type;

/*  ###############################################################  */

struct common {
    node_type which;
    int       line;
    int       col;
    int       tok;
    tn_t      type;
};

typedef struct treenode {
    struct common    hdr; 
    struct treenode *lnode;
    struct treenode *rnode;
} treenode;

#define TREENODE_SZE        (sizeof(treenode))

/*  ###############################################################  */

typedef struct if_node {
    struct common    hdr;
    struct treenode *cond;
    struct treenode *then_n;
    struct treenode *else_n;
} if_node;

#define IFNODE_SZE        (sizeof(if_node))

/*  ###############################################################  */

typedef struct for_node {
    struct common    hdr;
    struct treenode *init;
    struct treenode *test;
    struct treenode *incr;
    struct treenode *stemnt;
} for_node;

#define FORNODE_SZE        (sizeof(for_node))

/*  ###############################################################  */

struct type_item;

typedef struct leafnode {

    struct common       hdr;

    /* The node in the symbol table. */
    struct symentry    *syment;

    union {
      int               cval;
      str_t            *sval;
      char             *str;
      int               ival;
      long int          l_ival;
      unsigned int      u_ival;
      unsigned long int ul_ival; 
      float             fval;
      double            dval;
      long double       l_dval;
    } data;

} leafnode;

#define LEAFNODE_SZE        (sizeof(leafnode))

/*  ###############################################################  */

    /*  External Declarations */

typedef  void (*FindFunction)(leafnode*, treenode*, treenode*);

char *name_of_node        ARGS((tn_t));
char *name_of_nodetype    ARGS((node_type));

leafnode *make_leaf       ARGS((tn_t));
if_node  *make_if         ARGS((tn_t));
for_node *make_for        ARGS((tn_t));
treenode *make_node       ARGS((tn_t));

void      free_tree       ARGS((treenode*));

treenode *copy_tree       ARGS((treenode*));

leafnode *leftmost        ARGS((treenode*));
leafnode *rightmost       ARGS((treenode*));

void find_typedef_name    ARGS((treenode*,treenode*, FindFunction ));
void find_ident_name      ARGS((treenode*,treenode*,treenode*, FindFunction));
leafnode *find_func_name  ARGS((treenode*));
void find_params          ARGS((treenode*, FindFunction));
void find_components      ARGS((treenode*,treenode*,treenode*,FindFunction));

/*  ###############################################################  */

END_HEADER

#endif    /* TREE_H */
