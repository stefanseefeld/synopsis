/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
    o+
    o+     File:         netwrk.c
    o+
    o+     Programmer:   Shaun Flisakowski
    o+     Date:         Aug 19, 1998
    o+
    o+     Structures to hold the network.
    o+
    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

#include <stdio.h>
#include <stdlib.h>

#include "netwrk.h"

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
CmpNode*
makeNode(CmpType type, int num)
{
    CmpNode *node = malloc(sizeof(CmpNode));

    node->type = type;
    node->num  = num;

    node->tuple  = NULL;

    node->in[0]  = NULL;
    node->in[1]  = NULL;

    node->hiNode = NULL;
    node->loNode = NULL;

    node->mark = 0;
    node->next = NULL;

    return node;
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
CmpTuple*
makeTuple(CmpNode *hi, CmpNode *lo)
{
    CmpTuple *tuple = malloc(sizeof(CmpTuple));

    tuple->hi = hi;
    tuple->lo = lo;

    return tuple;
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void
freeTuple(CmpTuple *tuple)
{
    if (tuple)
    {
        free(tuple->hi);
        free(tuple->lo);
        free(tuple);
    }
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
CmpList*
makeList(CmpNode *node)
{
    CmpList *list = malloc(sizeof(CmpList));

    list->cnt = 0;
    list->head = list->tail = NULL;
    append(list,node);

    return list;
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void
append(CmpList *list, CmpNode *node)
{
    node->next = NULL;

    if (list->tail)
        list->tail->next = node;
    else
        list->head = node;

    list->tail = node;
    (list->cnt)++;
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void
freeList(CmpList *list)
{
    CmpNode *node, *prevNode = NULL;

    if (list)
    {
        for (node=list->head; node; node=node->next)
        {
            free(prevNode);
            freeTuple(node->tuple);
            prevNode = node;
        }
        free(prevNode);

        free(list);
    }
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
CmpProg*
makeProg(CmpList *hdr, CmpList *netwrk)
{
    CmpProg *prog = malloc(sizeof(CmpProg));

    prog->hdr = hdr;
    prog->netwrk = netwrk;
    prog->nIn = hdr->cnt;

    return prog;
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
CmpProg*
linkNetwork(CmpProg *net)
{
    CmpNode **table;
    CmpNode *node;
    int j, k = 0;
    int inBegin, cmpBegin, outBegin;

    table = malloc( sizeof(CmpNode*)
                    * (  net->hdr->cnt        /* inputs */
                       + net->netwrk->cnt     /* comparators */
                       + net->hdr->cnt        /* outputs */
                       + 1 ));                /* one extra. */

    /* Add all nodes to the table. */
    inBegin = k;
    for (node=net->hdr->head; node; node=node->next)
    {
        table[k] = makeNode(Input,k-inBegin+1);
        k++;
    }
    
    cmpBegin = k;
    for (node=net->netwrk->head; node; node=node->next)
    {
        table[k++] = node;
    }

    outBegin = k;
    for (node=net->hdr->head; node; node=node->next)
    {
        table[k] = makeNode(Output,k-outBegin+1);
        k++;
    }

    table[k] = NULL;

    /* Link the inputs to their comparators. */
    for (j=inBegin, node=net->hdr->head; node; node=node->next, j++)
    {
        switch (node->type)
        {
          case Compare:
              table[j]->loNode = table[cmpBegin + node->num - 1];
              break;

          case Output:
              fprintf(stderr, "Warning: Input node %d connected "
                              "directly to output.\n", j-inBegin+1);
              table[j]->loNode = table[outBegin + node->num - 1];
              break;

          case Input:
              fprintf(stderr, "Error: Input node %d trying to output "
                              "to another input node.\n", j-inBegin+1);
              break;
        }

        if (table[j]->loNode->in[0] == NULL)
            table[j]->loNode->in[0] = table[j];
        else
            table[j]->loNode->in[1] = table[j];

    }

    /* Hook up all comparators. */
    for (j=cmpBegin, node=net->netwrk->head; node; node=node->next, j++)
    {
        CmpTuple *tuple = node->tuple;

        if (tuple)
        {
            switch (tuple->hi->type)
            {
              case Compare:
                  table[j]->hiNode = table[cmpBegin + tuple->hi->num - 1];
                  break;

              case Output:
                  table[j]->hiNode = table[outBegin + tuple->hi->num - 1];
                  break;

              case Input:
                  fprintf(stderr, "Error: Compare node %d trying to output "
                            "to an input node.\n", j-cmpBegin+1);
                  break;
            }

            if (table[j]->hiNode->in[0] == NULL)
                table[j]->hiNode->in[0] = table[j];
            else
                table[j]->hiNode->in[1] = table[j];

            switch (tuple->lo->type)
            {
              case Compare:
                  table[j]->loNode = table[cmpBegin + tuple->lo->num - 1];
                  break;

              case Output:
                  table[j]->loNode = table[outBegin + tuple->lo->num - 1];
                  break;

              case Input:
                  fprintf(stderr, "Error: Compare node %d trying to output "
                            "to an input node.\n", j-cmpBegin+1);
                  break;
            }

            if (table[j]->loNode->in[0] == NULL)
                table[j]->loNode->in[0] = table[j];
            else
                table[j]->loNode->in[1] = table[j];

            freeTuple(tuple);
            node->tuple = NULL;
        }
    }

    freeList(net->hdr);
    net->hdr = NULL;

    /* Link all the nodes togather. */
    net->netwrk->head = net->netwrk->tail = NULL;
    net->netwrk->cnt = 0;

    for (j=0; j < k; j++)
        append(net->netwrk,table[j]);

    free(table);
    return net;
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

