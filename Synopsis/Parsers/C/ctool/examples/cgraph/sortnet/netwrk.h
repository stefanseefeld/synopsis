/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
    o+
    o+     File:         netwrk.h
    o+
    o+     Programmer:   Shaun Flisakowski
    o+     Date:         Aug 19, 1998
    o+
    o+     Structures to hold the network.
    o+
    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

#ifndef    NETWRK_H
#define    NETWRK_H


/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
typedef enum
{
    Input,
    Compare,
    Output
} CmpType;


typedef struct CmpNode
{
    CmpType           type;
    int               num;

    struct CmpTuple  *tuple;

    struct CmpNode   *in[2];      // inputs

    struct CmpNode   *loNode;     // outputs
    struct CmpNode   *hiNode;

    int               mark;
    struct CmpNode   *next;
    
} CmpNode;

typedef struct CmpTuple
{
    CmpNode   *hi;
    CmpNode   *lo;
} CmpTuple;


/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

typedef struct CmpList
{
    int         cnt;
    CmpNode    *head;
    CmpNode    *tail;
} CmpList;

typedef struct CmpProg
{
    int        nIn;
    CmpList    *hdr;
    CmpList    *netwrk;
} CmpProg;

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

CmpNode  *makeNode(CmpType type, int num);
CmpTuple *makeTuple(CmpNode *hi, CmpNode *lo);
void      freeTuple(CmpTuple *tuple);

CmpList  *makeList(CmpNode *node);
void      append(CmpList *list, CmpNode *node);
void      freeList(CmpList *list);

CmpProg  *makeProg(CmpList *hdr, CmpList *netwrk);

CmpProg  *linkNetwork(CmpProg *net);

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

#endif  /* NETWRK_H */

