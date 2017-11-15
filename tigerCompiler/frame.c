#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"

F_access InFrame(int offset) {
    F_access access = malloc(sizeof(struct F_access_));
    access->kind = inFrame;
    access->u.offset = offset;
    return access;
}

F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList accessList = malloc(sizeof(struct F_accessList_));
    accessList->head = head;
    accessList->tail = tail;
    return accessList;
}

F_accessList getFormals(U_boolList formals) {
    const int base = 2;
    if (formals == NULL)
        return NULL;

    F_accessList accessList = getFormals(formals->tail);
    if (accessList) {
        return F_AccessList(InFrame(accessList->head->u.offset + WORD), accessList);
    }
    else {
        return F_AccessList(InFrame(base * WORD), NULL);
    }
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
    F_frame frame = malloc(sizeof(struct F_frame_));
    frame->name = name;
    frame->argv_cnt = 0;
    frame->locv_cnt = 0;
//    F_accessList accessList = NULL;
    // the list is reversed, but not importan
//    for (U_boolList p = formals; p; p = p->tail) {
//        accessList = F_AccessList(InFrame((frame->argv_cnt + 2) * WORD), accessList);
//        frame->argv_cnt++;
//    }
//    frame->formals = accessList;

    frame->formals = getFormals(formals);

    return frame;
}

F_access F_allocLocal(F_frame f, bool escape) {
    f->locv_cnt++;
    F_access access = InFrame((f->locv_cnt) * (-WORD));
//    f->formals = F_AccessList(access, f->formals);
    return access;
}

Temp_temp F_Fp(void) {
    static Temp_temp tmp = NULL;
    if (tmp == NULL) {
        tmp = Temp_newtemp();
    }
    return tmp;
}

T_exp F_Exp(F_access access, T_exp fp) {
    return T_Mem(T_Binop(T_plus, fp, T_Const(access->u.offset)));
}