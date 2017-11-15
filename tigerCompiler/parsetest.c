#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "parse.h"
#include "prabsyn.h"
#include "types.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "translate.h"
#include "semant.h"

int main(int argc, char **argv) {
     if (argc!=2) {
         fprintf(stderr,"usage: a.out filename\n");
         exit(1);
     }
     A_exp root = parse(argv[1]);
    printf("absyn.tree\n");
    printf("---------------------------------------------------\n");
//    A_exp root = parse("../test/test4.tig");
    if (root != NULL) pr_exp(stdout, root, 4);
    SEM_transProg(root);
    return 0;
}
