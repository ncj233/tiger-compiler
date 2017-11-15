#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "translate.h"

Tr_access Tr_Access(Tr_level level, F_access access) {
    Tr_access a = malloc(sizeof(struct Tr_access_));
    a->level = level;
    a->access = access;
    return a;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList accessList = malloc(sizeof(struct Tr_accessList_));
    accessList->head = head;
    accessList->tail = tail;
    return accessList;
}

Tr_level Tr_outermost(void) {
    Temp_label temp = Temp_newlabel();
    Tr_level l = Tr_newLevel(NULL, temp, NULL);
    return l;
}

Tr_accessList Tr_AccessList_Convert(Tr_level level, F_accessList faccessList) {
    if (faccessList == NULL)
        return NULL;

    return Tr_AccessList(Tr_Access(level, faccessList->head), Tr_AccessList_Convert(level, faccessList->tail));
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
    Tr_level l = malloc(sizeof(struct Tr_level_));
    l->upper = parent;
    l->func_name = name;
    l->frame = F_newFrame(name, formals);
    l->formals = Tr_AccessList_Convert(l, l->frame->formals);

    return l;
}

Tr_accessList Tr_formals(Tr_level level) {
    return level->formals;
}


Tr_access Tr_allocLocal(Tr_level level, bool escape) {
    Tr_access access = malloc(sizeof(struct Tr_access_));
    access->level = level;
    access->access = F_allocLocal(level->frame, escape);
    return access;
}

patchList PatchList(Temp_label *head, patchList tail) {
    patchList l = malloc(sizeof(struct patchList_));
    l->head = head;
    l->tail = tail;
    return l;
}

Tr_exp Tr_Ex(T_exp ex) {
    Tr_exp exp = malloc(sizeof(struct Tr_exp_));
    exp->kind = Tr_ex;
    exp->u.ex = ex;
    return exp;
}

Tr_exp Tr_Nx(T_stm nx) {
    Tr_exp exp = malloc(sizeof(struct Tr_exp_));
    exp->kind = Tr_nx;
    exp->u.nx = nx;
    return exp;
}

Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
    Tr_exp exp = malloc(sizeof(struct Tr_exp_));
    exp->kind = Tr_cx;
    struct Cx cx;
    cx.trues = trues;
    cx.falses = falses;
    cx.stm = stm;
    exp->u.cx = cx;
    return exp;
}

T_exp unEx(Tr_exp e) {
    assert(e);
    switch (e->kind) {
        case Tr_ex: {
            return e->u.ex;
        }

        case Tr_cx: {
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel();
            Temp_label f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
                          T_Eseq(e->u.cx.stm,
                                 T_Eseq(T_Label(f),
                                        T_Eseq(T_Move(T_Temp(r), T_Const(0)),
                                               T_Eseq(T_Label(t), T_Temp(r))))));
        }

        case Tr_nx: {
            return T_Eseq(e->u.nx, T_Const(0));
        }

        default:
            assert(0);
    }
}

T_stm unNx(Tr_exp e) {
    assert(e);
    switch (e->kind) {
        case Tr_ex: {
            return T_Exp(e->u.ex);
        }

        case Tr_nx: {
            return e->u.nx;
        }

        case Tr_cx: {
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel();
            Temp_label f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Exp(T_Eseq(T_Move(T_Temp(r), T_Const(1)),
                                T_Eseq(e->u.cx.stm,
                                       T_Eseq(T_Label(f),
                                              T_Eseq(T_Move(T_Temp(r), T_Const(0)),
                                                     T_Eseq(T_Label(t), T_Temp(r)))))));
        }

        default:
            assert(0);
    }
}

struct Cx unCx(Tr_exp e) {
    assert(e);
    switch (e->kind) {
        case Tr_ex: {
            struct Cx c;
            c.stm = T_Cjump(T_eq, e->u.ex, T_Const(0), NULL, NULL);
            c.trues = PatchList(&(c.stm->u.CJUMP.false), NULL);
            c.falses = PatchList(&(c.stm->u.CJUMP.true), NULL);
            return c;
        }

        case Tr_nx:
            assert(0);

        case Tr_cx: {
            return e->u.cx;
        }

        default:
            assert(0);
    }
}

void doPatch(patchList tList, Temp_label label) {
    for (; tList; tList = tList->tail) {
        *(tList->head) = label;
    }
}

patchList joinPatch(patchList first, patchList second) {
    if (first == NULL)
        return second;

    patchList copyFirst = first;
    for (; first->tail; first = first->tail);

    first->tail = second;
    return copyFirst;
}

/* -------------------------------------------------------------- */

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level) {
    assert(level && access);
    T_exp t = NULL;
    if (level == access->level) {
        return Tr_Ex(F_Exp(access->access, T_Temp(F_Fp())));
    }
    for (; level!=access->level; level = level->upper) {
        if (t == NULL) {
            t = T_Mem(T_Binop(T_plus, T_Temp(F_Fp()), T_Const(4)));
        }
        else {
            t = T_Mem(T_Binop(T_plus, t, T_Const(4)));
        }
    }
    return Tr_Ex(F_Exp(access->access, t));

//    if (access->level == level) {
//        return Tr_Ex(F_Exp(access->access, T_Temp(F_Fp())));
//    }
//    else {
//        Tr_exp texp = Tr_simpleVar(access, level->upper);
//        return Tr_Ex(F_Exp(access->access, unEx(texp)));
//        return Tr_Ex(F_Exp(access->access, T_Mem(T_Binop(T_plus, unEx(texp), T_Const(4)))));
//    }
}

Tr_exp Tr_subscript(Tr_exp left, Tr_exp index) {
    return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(left), T_Binop(T_mul, unEx(index), T_Const(4)))));
}

Tr_exp Tr_field(Tr_exp left, int offset) {
    return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(left), T_Const(offset))));
}

Tr_exp Tr_binOp_int(A_oper op, Tr_exp left, Tr_exp right) {
    switch(op) {
        case A_plusOp: {
            return Tr_Ex(T_Binop(T_plus, unEx(left), unEx(right)));
        }

        case A_minusOp: {
            return Tr_Ex(T_Binop(T_minus, unEx(left), unEx(right)));
        }

        case A_timesOp: {
            return Tr_Ex(T_Binop(T_mul, unEx(left), unEx(right)));
        }

        case A_divideOp: {
            return Tr_Ex(T_Binop(T_div, unEx(left), unEx(right)));
        }

        default:
            assert(0);
    }
}

Tr_exp Tr_compare_int(A_oper op, Tr_exp left, Tr_exp right) {
    T_relOp comp;
    switch (op) {
        case A_eqOp: {
            comp = T_eq;
            break;
        }
        case A_neqOp: {
            comp = T_ne;
            break;
        }

        case A_ltOp: {
            comp = T_lt;
            break;
        }

        case A_gtOp: {
            comp = T_gt;
            break;
        }

        case A_leOp: {
            comp = T_le;
            break;
        }

        case A_geOp: {
            comp = T_ge;
            break;
        }

        default:
            assert(0);
    }
    T_stm stm = T_Cjump(comp, unEx(left), unEx(right), NULL, NULL);
    patchList trueList = PatchList(&(stm->u.CJUMP.true), NULL);
    patchList falseList = PatchList(&(stm->u.CJUMP.false), NULL);
    return Tr_Cx(trueList, falseList, stm);
}

Tr_exp Tr_if(Tr_exp cond, Tr_exp then) {
    Temp_label true = Temp_newlabel();
    Temp_label false = Temp_newlabel();
    struct Cx cx = unCx(cond);
    doPatch(cx.trues, true);
    doPatch(cx.falses, false);
    switch (then->kind) {
        case Tr_ex: {
            return Tr_Nx(T_Seq(cx.stm, T_Seq(T_Label(true), T_Seq(T_Exp(unEx(then)), T_Label(false)))));
        }

        case Tr_nx: {
            return Tr_Nx(T_Seq(cx.stm, T_Seq(T_Label(true), T_Seq(then->u.nx, T_Label(false)))));
        }

        case Tr_cx: {
            return Tr_Nx(T_Seq(cx.stm, T_Seq(T_Label(true), T_Seq(then->u.cx.stm, T_Label(false)))));
        }

        default:
            assert(0);
    }
}

Tr_exp Tr_ifElse(Tr_exp cond, Tr_exp then, Tr_exp elsee) {
    Temp_label true = Temp_newlabel();
    Temp_label false = Temp_newlabel();
    Temp_label exitt = Temp_newlabel();
    T_stm jump = T_Jump(T_Name(exitt), Temp_LabelList(exitt, NULL));
    struct Cx cx = unCx(cond);
    doPatch(cx.trues, true);
    doPatch(cx.falses, false);

    if (then->kind == Tr_ex || elsee->kind == Tr_ex) {
        Temp_temp result = Temp_newtemp();
        return Tr_Ex(T_Eseq(cx.stm, T_Eseq(T_Label(true), T_Eseq(T_Move(T_Temp(result), unEx(then)),
                                                                 T_Eseq(jump, T_Eseq(T_Label(false), T_Eseq(T_Move(T_Temp(result), unEx(elsee)),
                                                                                                            T_Eseq(jump, T_Eseq(T_Label(exitt), T_Temp(result))))))))));
    }

    switch(then->kind) {
        case Tr_ex: {
            Temp_temp result = Temp_newtemp();
            return Tr_Ex(T_Eseq(cx.stm, T_Eseq(T_Label(true), T_Eseq(T_Move(T_Temp(result), unEx(then)),
            T_Eseq(jump, T_Eseq(T_Label(false), T_Eseq(T_Move(T_Temp(result), unEx(elsee)),
            T_Eseq(jump, T_Eseq(T_Label(exitt), T_Temp(result))))))))));
        }

        case Tr_nx: case Tr_cx: {
            T_stm then_stm, else_stm;
            if (then->kind == Tr_nx) {
                then_stm = then->u.nx;
            }
            else if (then->kind == Tr_cx) {
                then_stm = then->u.cx.stm;
            }
            else {
                assert(0);
            }

            if (elsee->kind == Tr_ex) {
                else_stm = T_Exp(then->u.ex);
            }
            else if (elsee->kind == Tr_nx) {
                else_stm = elsee->u.nx;
            }
            else if (elsee->kind == Tr_cx) {
                else_stm = elsee->u.cx.stm;
            }
            else {
                assert(0);
            }

            return Tr_Nx(T_Seq(cx.stm, T_Seq(T_Label(true), T_Seq(then_stm, T_Seq(jump, T_Seq(T_Label(false),
            T_Seq(else_stm, T_Seq(jump, T_Label(exitt)))))))));
        }

        default:
            assert(0);
    }
}

Tr_exp Tr_while(Tr_exp cond, Tr_exp body, Temp_label done) {
    Temp_label l_cond = Temp_newlabel();
    Temp_label l_body = Temp_newlabel();
    T_stm jump = T_Jump(T_Name(l_cond), Temp_LabelList(l_cond, NULL));
    return Tr_Ex(T_Eseq(jump, T_Eseq(T_Label(l_body), T_Eseq(unNx(body), T_Eseq(T_Label(l_cond),
    T_Eseq(T_Cjump(T_eq, unEx(cond), T_Const(0), done, l_body), T_Eseq(T_Label(done), T_Const(0))))))));
}

Tr_exp Tr_call(Temp_label func, T_expList list) {
    return Tr_Ex(T_Call(T_Name(func), list));
}

Tr_exp Tr_compare_string(A_oper op, Tr_exp left, Tr_exp right) {
    switch (op) {
        case A_eqOp: {
            return Tr_call(Temp_namedlabel("str_eq"), T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
        }
        case A_neqOp: {
            return Tr_call(Temp_namedlabel("str_neq"), T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
        }

        case A_ltOp: {
            return Tr_call(Temp_namedlabel("str_lt"), T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
        }

        case A_gtOp: {
            return Tr_call(Temp_namedlabel("str_gt"), T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
        }

        case A_leOp: {
            return Tr_call(Temp_namedlabel("str_le"), T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
        }

        case A_geOp: {
            return Tr_call(Temp_namedlabel("str_ge"), T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
        }

        default:
            assert(0);
    }
}

Tr_exp Tr_for(Tr_access access, Tr_level level, Tr_exp left, Tr_exp right, Tr_exp body, Temp_label done) {
    Temp_temp limit = Temp_newtemp();
    T_stm assign = T_Seq(T_Move(unEx(Tr_simpleVar(access, level)), unEx(left)),
    T_Move(T_Temp(limit), unEx(right)));
    Tr_exp whilee = Tr_while(Tr_compare_int(A_leOp, Tr_simpleVar(access, level), Tr_Ex(T_Temp(limit))), body, done);
    return Tr_Nx(T_Seq(assign, unNx(whilee)));
}
