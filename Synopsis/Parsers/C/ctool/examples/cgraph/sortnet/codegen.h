/*  o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o
    o+
    o+     File:         codegen.h
    o+
    o+     Programmer:   Shaun Flisakowski
    o+     Date:         Aug 20, 1998
    o+
    o+     Generate the C code for the sorting network.
    o+
    o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o+o  */

#ifndef    CODEGEN_H
#define    CODEGEN_H

#include <stdio.h>

#include "netwrk.h"

void prntRef(FILE *out, CmpNode *node, CmpNode *parent);

void prntHeader(FILE *out, int nIn, int genTest);
void prntCmp(FILE *out, int sortIncr);
void prntNetwork(FILE *out, CmpProg *net);

void prntTest(FILE *out, int nIn, int sortIncr, int debugOn);

void prntDescription(FILE *out, CmpProg *net);

#endif  /* CODEGEN_H */

