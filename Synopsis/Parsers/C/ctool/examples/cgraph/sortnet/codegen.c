/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
    o+
    o+     File:         codegen.c
    o+
    o+     Programmer:   Shaun Flisakowski
    o+     Date:         Aug 20, 1998
    o+
    o+     Generate the C code for the sorting network.
    o+
    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

#include "codegen.h"

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
static
void
prntNode(FILE *out, CmpNode *node)
{
    if (node)
        switch (node->type)
        {
            case Input:
                fprintf(out, "i%d", node->num);
                break;
    
            case Compare:
                fprintf(out, "c%d", node->num);
                break;
    
            case Output:
                fprintf(out, "o%d", node->num);
                break;
        }
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void
prntRef(FILE *out, CmpNode *node, CmpNode *parent)
{
    switch (node->type)
    {
        case Input:
            fprintf(out,"in[%d]",node->num - 1);
            break;

        case Compare:
            if (node == parent->hiNode)
                fprintf(out,"hi%d", parent->num);
            else if (node == parent->loNode)
                fprintf(out,"lo%d", parent->num);
            else if (node->hiNode == parent)
                fprintf(out,"hi%d", node->num);
            else if (node->loNode == parent)
                fprintf(out,"lo%d", node->num);
            break;

        case Output:
            fprintf(out,"out[%d]",node->num - 1);
            break;
    }
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void
prntHeader(FILE *out, int nIn, int genTest)
{
    if (genTest)
        fputs("\n#include <stdio.h>\n\n",out);

    fputs("\n", out);
    fputs("typedef int sntype;\n", out);
    fputs("\n", out);
    fputs("void cmp(sntype in1, sntype in2, sntype *hi, sntype *lo);\n",out);
    fputs("\n", out);
    fprintf(out,"void sortnet%d(sntype *in);\n", nIn);
    fputs("\n", out);
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void
prntCmp(FILE *out, int sortIncr)
{
    fputs("\n", out);
    fputs("void\n",out);
    fputs("cmp(sntype in1, sntype in2, sntype *hi, sntype *lo)\n",out);
    fputs("{\n", out);

    if (sortIncr)
        fputs("    if (in1 >= in2)\n", out);
    else
        fputs("    if (in1 <= in2)\n", out);

    fputs("    {\n", out);
    fputs("        *lo = in1;\n", out);
    fputs("        *hi = in2;\n", out);
    fputs("    }\n", out);
    fputs("    else\n", out);
    fputs("    {\n", out);
    fputs("        *lo = in2;\n", out);
    fputs("        *hi = in1;\n", out);
    fputs("    }\n", out);
    fputs("}\n\n", out);
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void
prntNetwork(FILE *out, CmpProg *net)
{
    CmpNode *node, *begin, *nextBegin;
    int      done;

    /* Clear all marks, except inputs */
    for (node=net->netwrk->head; node; node=node->next)
        node->mark = (node->type == Input) ? 2 : 0;

    fputs("\n", out);
    fputs("void\n",out);
    fprintf(out,"sortnet%d(sntype *in)\n", net->nIn);
    fputs("{\n", out);

    fputs("    sntype *out = in;\n\n", out);

    for (node=net->netwrk->head; node; node=node->next)
    {
        if (node->type == Compare)
        {
            if (   (node->hiNode->type != Output)
                && (node->loNode->type != Output) )
            {
                fprintf(out, "    sntype\thi%d,\tlo%d;\n",
                                   node->num, node->num);
            }
            else if (node->hiNode->type != Output)
            {
                fprintf(out, "    sntype\thi%d;\n", node->num);
            }
            else if (node->loNode->type != Output)
            {
                fprintf(out, "    sntype\t\tlo%d;\n", node->num);
            }
        }
    }

    begin = net->netwrk->head;
    nextBegin = NULL;
    done = 0;

    while (!done)
    {    
        fputs("\n", out);
        fputs("    {\n", out);

        for (node=begin; node; node=node->next)
        {

            if (   (node->type == Compare)
                && !node->mark )
            {
                if (   (node->in[0]->mark >= 2)
                    && (node->in[1]->mark >= 2) )
                {
                    /* we can do this node now. */
                    fputs("      cmp(",out);
                    prntRef(out,node->in[0],node); 
                    fputs(",",out);
                    prntRef(out,node->in[1],node); 
                    fputs(",&",out);
                    prntRef(out,node->hiNode,node); 
                    fputs(",&",out);
                    prntRef(out,node->loNode,node); 
                    fputs(");\n",out);
    
                    node->mark = 1;
                }
                else if (!nextBegin)
                    nextBegin = node;
            }
        }

        fputs("    }\n", out);

        for (node=begin; node; node=node->next)
            if (node->mark == 1)
                (node->mark)++;

        done = (nextBegin == NULL);
        begin = nextBegin;
        nextBegin = NULL;
    }

    fputs("}\n\n", out);

}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void
prntTest(FILE *out, int nIn, int sortIncr, int debugOn)
{
    fputs("\nint\n",out);
    fputs("main(int ac, char **av)\n",out);
    fputs("{\n", out);
    fprintf(out, "#define SORT_SIZE %d", nIn);
    fputs("\n",out);

    fputs("    int  j, k;\n", out);
    fputs("    int  loop[SORT_SIZE];\n", out);
    fputs("    int  val[SORT_SIZE];\n", out);
    fputs("\n",out);
   
    fputs("    unsigned long valid=0, invalid=0;\n", out);
    fputs("\n",out);

    fputs("    /* Because of the zero-one principle, it is sufficient\n", out);
    fputs("       to test only all possible binary vectors. */\n", out);
    fputs("    for (j=0; j < SORT_SIZE; j++)\n", out);
    fputs("        loop[j] = 0;\n", out);
    fputs("\n",out);

    fputs("    do {\n", out);
    fputs("        for (j=0; j < SORT_SIZE; j++)\n", out);
    fputs("            val[j] = loop[j];\n", out);
    fputs("\n", out);

    if (debugOn)
    {
        fputs("        fprintf(stdout, \"%d\", val[0]);\n", out);
        fputs("        for (j=1; j < SORT_SIZE; j++)\n", out);
        fputs("            fprintf(stdout, \",%d\", val[j]);\n", out);
        fputs("\n", out);
        fputs("        fputs(\"  ->  \", stdout);\n", out);
        fputs("\n", out);
    }

    fprintf(out,"        sortnet%d(val);\n", nIn);
    fputs("\n", out);

    if (debugOn)
    {
        fputs("        fprintf(stdout, \"%d\", val[0]);\n", out);
        fputs("        for (j=1; j < SORT_SIZE; j++)\n", out);
        fputs("            fprintf(stdout, \",%d\", val[j]);\n", out);
        fputs("\n", out);
    }

    fputs("        for (j=1; j < SORT_SIZE; j++)\n", out);
    if (sortIncr)
        fputs("            if (val[j-1] > val[j])\n", out);
    else
        fputs("            if (val[j-1] < val[j])\n", out);

    fputs("                break;\n", out);
    fputs("\n", out);
    fputs("        if (j == SORT_SIZE)\n", out);
    fputs("            valid++;\n", out);
    fputs("        else\n", out);
    fputs("            invalid++;\n", out);

    if (debugOn)
    {
        fputs("\n", out);
        fputs("        if (j != SORT_SIZE)\n", out);
        fputs("            fputs(\"    -INCORRECT-\", stdout);\n", out);
        fputs("        fputs(\"\\n\", stdout);\n", out);
    }

    fputs("\n", out);
    fputs("        for (k=0; k < SORT_SIZE; k++)\n", out);
    fputs("        {\n", out);
    fputs("            loop[k]++;\n", out);
    fputs("\n", out);
    fputs("            if (loop[k] <= 1)\n", out);
    fputs("                break;\n", out);
    fputs("\n", out);
    fputs("            loop[k] = 0;\n", out);
    fputs("        }\n", out);
    fputs("\n", out);
    fputs("    } while (k < SORT_SIZE);\n", out);
    fputs("\n", out);
    fputs("    fprintf(stdout, \"%lu correctly sorted, "
                 "%lu incorrectly sorted.\\n\",\n", out);
    fputs("                    valid, invalid );\n", out);
    fputs("\n", out);
    fputs("    if (invalid == 0)\n", out);
    fputs("        fputs(\"perfect!\\n\",stdout);\n", out);
    fputs("\n", out);
    fputs("    return 0;\n", out);
    fputs("}\n", out);
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
void
prntDescription(FILE *out, CmpProg *net)
{
    CmpNode *node;

    fprintf(out,"\t\t   Node\n");

    for (node=net->netwrk->head; node; node=node->next)
    {
        fputs("      ",out);

        if (node->in[1])
        {
            fputs(" <",out);
            prntNode(out,node->in[0]);
            fputs(",",out);
            prntNode(out,node->in[1]);
            fputs(">\t",out);
            fputs(" -> ",out);
        }
        else if (node->in[0])
        {
            fputs("  ",out);
            prntNode(out,node->in[0]);
            fputs("\t -> ",out);
        }
        else
        {
            fputs("\t\t    ",out);
        }

        prntNode(out,node);

        if (node->hiNode)
        {
            fputs("\t->  <",out);
            prntNode(out,node->hiNode);
            fputs(",",out);
            prntNode(out,node->loNode);
            fputs(">",out);
        }
        else if (node->loNode)
        {
            fputs("\t->   ",out);
            prntNode(out,node->loNode);
        } 

        fputs("\n",out);
    }
}

/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */
