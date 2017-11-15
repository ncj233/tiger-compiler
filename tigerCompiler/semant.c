#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "symbol.h"
#include "table.h"
#include "types.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "translate.h"
#include "env.h"
#include "semant.h"
#include "errormsg.h"
#include "printtree.h"

Tr_level global_level = NULL;
Temp_labelList labelList = NULL;

static int loopLevel = 0;
void enterLoop(Temp_label done) {
    loopLevel++;
    labelList = Temp_LabelList(done, labelList);
}
void escapeLoop() {
    loopLevel--;
    assert(labelList);
    labelList = labelList->tail;
}
int isInLoop() {
    return (loopLevel? 1 : 0);
}

/* this part of program is a test for env.c */
void show_venv(void *k, void *va) {
    S_symbol key = k;
    E_enventry  val = va;
    printf("%s ", S_name(key));
    if (val && val->kind == E_varEntry) {
        Ty_print(val->u.var.ty);
    } else if (val && val->kind == E_funEntry) {
        TyList_print(val->u.fun.formals);
        Ty_print(val->u.fun.result);
    }
    printf("\n");
    return;
}

void show_tenv(void *k, void *va) {
    S_symbol key = k;
    Ty_ty ty = va;
    printf("%s ", S_name(key));
    Ty_print(ty);
    while (ty && ty->kind == Ty_name) {
        printf("-->");
        ty = ty->u.name.ty;
        Ty_print(ty);
    }
    printf("\n");
    return;
}

void printenv(S_table venv, S_table tenv) {
    printf("\n");
    printf("==-------------------------=---------------------\n");
    TAB_dump(tenv, show_tenv);
    printf("-------------------------------------------------\n");
    TAB_dump(venv, show_venv);
    printf("--------------------------------------------==---\n");
}

T_expList T_expList_Append(T_expList args, Tr_exp e) {
    T_expList pre = args;
    if (args == NULL)
        return T_ExpList(unEx(e), NULL);
    for (; args->tail; args = args->tail);
    args->tail = T_ExpList(unEx(e), NULL);
    return pre;
}

struct expty expTy(Tr_exp exp, Ty_ty ty) {
    struct expty e;
    e.exp = exp;
    e.ty = ty;
    return e;
}

Ty_fieldList genTyFieldList(S_table tenv, A_fieldList fieldList) {
    if (fieldList == NULL) {
        return NULL;
    }

    A_field field = fieldList->head;
    Ty_ty e = S_look(tenv, field->typ);
    if (!e) {
        EM_error(field->pos, "undefined type: %s", S_name(field->typ));
        e = Ty_Int();
    }
    return Ty_FieldList(Ty_Field(field->name, e), genTyFieldList(tenv, fieldList->tail));
}

U_boolList makeUBoolList(A_fieldList fieldList) {
    if (fieldList == NULL)
        return NULL;

    return U_BoolList(TRUE, makeUBoolList(fieldList->tail));
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList args) {
    if (args == NULL)
        return NULL;

    A_field arg = args->head;
    Ty_ty ty = S_look(tenv, arg->typ);
    if (!ty) {
        EM_error(arg->pos, "undefiend type: %s", S_name(arg->typ));
        ty = Ty_Int();
    }
    return Ty_TyList(ty, makeFormalTyList(tenv, args->tail));
}

struct expty transVar(S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
        case A_simpleVar: {
            E_enventry x = S_look(venv, v->u.simple);
            if (x && x->kind == E_varEntry) {
                Tr_exp t = Tr_simpleVar(x->u.var.access, global_level);
//                return expTy(NULL, actual_ty(x->u.var.ty));
//                printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
//                printStmList (stdout, T_StmList(T_Exp(unEx(t)), NULL));
                return expTy(t, actual_ty(x->u.var.ty));
            }
            else {
                EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
                return expTy(NULL, Ty_Int());
            }
            assert(0);
        }

        case A_fieldVar: {
            struct expty left = transVar(venv, tenv, v->u.field.var);
            Ty_ty ty = left.ty;
            if (ty->kind != Ty_record) {
                EM_error(v->pos, "leftval before %s is not a field var", S_name(v->u.field.sym));
                return expTy(NULL, Ty_Int());
            }
            Ty_ty subTy = NULL;
            //add in translate
            int cnt = 0;
            for (Ty_fieldList list = ty->u.record; list; list = list->tail) {
                Ty_field field = list->head;
                if (field->name == v->u.field.sym) {
                    subTy = field->ty;
                    break;
                }
                //add in translate
                cnt++;
            }
            if (subTy) {
                Tr_exp exp = Tr_field(left.exp, cnt * WORD);
                return expTy(exp, actual_ty(subTy));
//                return expTy(NULL, actual_ty(subTy));
            }
            else {
                EM_error(v->pos, "left var do not have %s attribute", S_name(v->u.field.sym));
                return expTy(NULL, Ty_Int());
            }
            assert(0);
        }

        case A_subscriptVar: {
            struct expty left = transVar(venv, tenv, v->u.subscript.var);
            if (left.ty->kind != Ty_array) {
                EM_error(v->pos, "this should be an array");
                return expTy(NULL, Ty_Int());
            }
            struct expty index = transExp(venv, tenv, v->u.subscript.exp);
            if (index.ty->kind != Ty_int) {
                EM_error(v->pos, "index exp should be type int");
                return expTy(NULL, Ty_Int());
            }

            Tr_exp exp = Tr_subscript(left.exp, index.exp);
            return expTy(exp, actual_ty(left.ty->u.array));
//            return expTy(NULL, actual_ty(left.ty->u.array));
        }

        default:
            assert(0);
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp a) {
    switch (a->kind) {
        case A_varExp: {
            return transVar(venv, tenv, a->u.var);
        }

        case A_nilExp: {
//            return expTy(NULL, Ty_Nil());
            return expTy(Tr_Ex(T_Const(0)), Ty_Nil());
        }

        case A_intExp: {
//            return expTy(NULL, Ty_Int());
            return expTy(Tr_Ex(T_Const(a->u.intt)), Ty_Int());
        }

        case A_stringExp: {
//            return expTy(NULL, Ty_String());
            Temp_label str = Temp_newlabel();
            return expTy(Tr_Ex(T_Name(str)), Ty_String());
        }

        case A_callExp: {
            E_enventry x = S_look(venv, a->u.call.func);
            if (!(x && x->kind == E_funEntry)) {
                EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
                return expTy(NULL, Ty_Void());
            }
            A_expList expList = a->u.call.args;
            Ty_tyList tyList = x->u.fun.formals;
            // add in translate
            T_expList t_args = NULL;
            for (; expList && tyList; expList = expList->tail, tyList = tyList->tail) {
                A_exp a_arg = expList->head;
                struct expty arg = transExp(venv, tenv, a_arg);
                // add in translate
                t_args = T_expList_Append(t_args, arg.exp);
                Ty_ty ty_para = tyList->head;
                if (arg.ty->kind != actual_ty(ty_para)->kind && arg.ty->kind != Ty_nil) {
                    EM_error(a_arg->pos, "type incompatible");
                    return expTy(NULL, Ty_Void());
                }
            }
            if (tyList || expList) {
                EM_error(a->pos, "number of arguments in function call is incompatible");
                return expTy(NULL, Ty_Void());
            }

            //add in translate
            t_args = T_expList_Append(t_args, Tr_Ex(T_Binop(T_plus, T_Temp(F_Fp()), T_Const(4))));
            Tr_exp trexp = Tr_call(x->u.fun.label, t_args);
            return expTy(trexp, actual_ty(x->u.fun.result));
//            return expTy(NULL, actual_ty(x->u.fun.result));
        }

        case A_opExp: {
            A_oper oper = a->u.op.oper;
            struct expty left = transExp(venv, tenv, a->u.op.left);
            struct expty right = transExp(venv, tenv, a->u.op.right);
            if (left.ty != right.ty && left.ty->kind != Ty_nil && right.ty->kind != Ty_nil) {
                EM_error(a->pos, "two operators do not have the same type");
                return expTy(NULL, Ty_Int());
            }
            if (oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp) {
                if (left.ty->kind != Ty_int)
                    EM_error(a->u.op.left->pos, "integer required");
                if (right.ty->kind != Ty_int)
                    EM_error(a->u.op.right->pos, "integer required");
//                return expTy(NULL, Ty_Int());
                Tr_exp exp = Tr_binOp_int(oper, left.exp, right.exp);
                return expTy(exp, Ty_Int());
            }
            else if (oper == A_eqOp || oper == A_neqOp) {
                //TODO
                if (left.ty->kind == Ty_int) {
//                    return expTy(NULL, Ty_Int());
                    return expTy(Tr_compare_int(oper, left.exp, right.exp), Ty_Int());
                }
                else if (left.ty->kind == Ty_string) {
//                    return expTy(NULL, Ty_Int());
                    return expTy(Tr_compare_string(oper, left.exp, right.exp), Ty_Int());
                }
                else {
//                    return expTy(NULL, Ty_Int());
                    return expTy(Tr_compare_int(oper, left.exp, right.exp), Ty_Int());
                }
            }
            else if (oper == A_ltOp || oper == A_leOp || oper == A_gtOp || oper == A_geOp) {
                //TODO
                if (left.ty->kind == Ty_int) {
//                    return expTy(NULL, Ty_Int());
                    return expTy(Tr_compare_int(oper, left.exp, right.exp), Ty_Int());
                }
                else if (left.ty->kind == Ty_string) {
//                    return expTy(NULL, Ty_Int());
                    return expTy(Tr_compare_string(oper, left.exp, right.exp), Ty_Int());
                }
                else {
                    EM_error(a->pos, "<, >, <=, >= can only compared by int or string");
                    return expTy(NULL, Ty_Int());
                }
            }
            assert(0);
        }

        case A_recordExp: {
            Ty_ty x = S_look(tenv, a->u.record.typ);
            if (!x) {
                EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
                return expTy(NULL, Ty_Nil());
            }
            x = actual_ty(x);
            if (!(x && x->kind == Ty_record)) {
                EM_error(a->pos, "type = %s should be a record type", S_name(a->u.record.typ));
                return expTy(NULL, Ty_Nil());
            }
            //E_enventry x = S_look(tenv, a->u.record.typ);
            //if (!(x && x->kind == E_varEntry)) {
            //    EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
            //    return expTy(NULL, Ty_Nil());
            //}
            A_efieldList fieldList = a->u.record.fields;
            Ty_fieldList tyfieldList = x->u.record;
            // add in translate
            for (; fieldList && tyfieldList; fieldList = fieldList->tail, tyfieldList = tyfieldList->tail) {
                A_efield field = fieldList->head;
                Ty_field tyfield = tyfieldList->head;
                if (field->name != tyfield->name) {
                    EM_error(a->pos, "field name incompatible, %s %s", S_name(field->name), S_name(tyfield->name));
                    return expTy(NULL, Ty_Nil());
                }
                struct expty inExp = transExp(venv, tenv, field->exp);
                if (inExp.ty != actual_ty(tyfield->ty) && inExp.ty->kind != Ty_nil) {
                    EM_error(field->exp->pos, "exp type should be %s", S_name(field->name));
                    return expTy(NULL, Ty_Nil());
                }
            }
            //TODO
            return expTy(Tr_Ex(T_Const(0)), x);
        }

        case A_seqExp: {
            A_expList expList = a->u.seq;
            if (!expList) {
                return expTy(NULL, Ty_Void());
            }
            //TODO
            T_exp result = NULL;
            for (; expList->tail; expList = expList->tail) {
                struct expty it = transExp(venv, tenv, expList->head);
                if (result == NULL) {
                    result = unEx(it.exp);
                }
                else {
                    result = T_Eseq(T_Exp(unEx(it.exp)), result);
                }
            }
            struct expty last = transExp(venv, tenv, expList->head);
            if (result == NULL) {
                result = unEx(last.exp);
            }
            else {
                result = T_Eseq(T_Exp(unEx(last.exp)), result);
            }
            return expTy(Tr_Ex(result), last.ty);
        }

        case A_assignExp: {
            struct expty left = transVar(venv, tenv, a->u.assign.var);
            struct expty right = transExp(venv, tenv, a->u.assign.exp);
            if (left.ty != right.ty && right.ty->kind != Ty_nil) {
                EM_error(a->pos, "assign expression left and right incompatible");
                return expTy(NULL, Ty_Void());
            }
//            return expTy(NULL, Ty_Void());
            return expTy(Tr_Nx(T_Move(unEx(left.exp), unEx(right.exp))), Ty_Void());
        }

        case A_ifExp: {
            struct expty test = transExp(venv, tenv, a->u.iff.test);
            if (test.ty->kind != Ty_int) {
                EM_error(a->pos, "test exp should be integer");
                return expTy(NULL, Ty_Void());
            }
            struct expty then = transExp(venv, tenv, a->u.iff.then);
            if (a->u.iff.elsee) {
                struct expty elsee = transExp(venv, tenv, a->u.iff.elsee);
                if (then.ty != elsee.ty && then.ty->kind != Ty_nil && elsee.ty->kind != Ty_nil) {
                    EM_error(a->pos, "then_exp and elsee_exp have different types");
                }
//                return expTy(NULL, then.ty);
                return expTy(Tr_ifElse(test.exp, then.exp, elsee.exp), then.ty);
            }
            else {
                if (then.ty->kind != Ty_void) {
                    EM_error(a->pos, "if expression should not return value");
                }
//                return expTy(NULL, Ty_Void());
                return expTy(Tr_if(test.exp, then.exp), Ty_Void());
            }
            assert(0);
        }

        case A_whileExp: {
            // add in translate
            Temp_label l_exit = Temp_newlabel();
            enterLoop(l_exit);
            struct expty test = transExp(venv, tenv, a->u.whilee.test);
            struct expty body = transExp(venv, tenv, a->u.whilee.body);
            escapeLoop();
            if (test.ty->kind != Ty_int) {
                EM_error(a->pos, "test exp should be integer");
                return expTy(NULL, Ty_Void());
            }
            if (body.ty->kind != Ty_void) {
                EM_error(a->pos, "whild body should not return value");
                return expTy(NULL, Ty_Void());
            }
//            return expTy(NULL, Ty_Void());
            return expTy(Tr_while(test.exp, body.exp, l_exit), Ty_Void());
        }

        case A_forExp: {
            //add in translate
            Temp_label l_exit = Temp_newlabel();
            enterLoop(l_exit);
            S_beginScope(venv);
            struct expty lower = transExp(venv, tenv, a->u.forr.lo);
            struct expty higher = transExp(venv, tenv, a->u.forr.hi);

            //add in frame
            Tr_access var_access = Tr_allocLocal(global_level, TRUE);
            S_enter(venv, a->u.forr.var, E_VarEntry(var_access, Ty_Int()));

            struct expty body = transExp(venv, tenv, a->u.forr.body);
            S_endScope(venv);
            escapeLoop();
            if (lower.ty->kind != Ty_int || higher.ty->kind != Ty_int) {
                EM_error(a->pos, "lo and hi should be int");
                return expTy(NULL, Ty_Void());
            }
            if (body.ty->kind != Ty_void) {
                EM_error(a->pos, "whild body should not return value");
                return expTy(NULL, Ty_Void());
            }
//            return expTy(NULL, Ty_Void());
            return expTy(Tr_for(var_access, global_level, lower.exp, higher.exp, body.exp, l_exit), Ty_Void());
        }

        case A_breakExp: {
            if (!isInLoop()) {
                EM_error(a->pos, "break is not in loop scope");
            }
//            return expTy(NULL, Ty_Void());
            assert(labelList);
            return expTy(Tr_Nx(T_Jump(T_Name(labelList->head), Temp_LabelList(labelList->head, NULL))), Ty_Void());
        }

        case A_letExp: {
            struct expty exp;
            A_decList d;
            S_beginScope(venv);
            S_beginScope(tenv);
            for (d = a->u.let.decs; d; d = d->tail)
                transDec(venv, tenv, d->head);

            //printenv(venv, tenv);

            exp = transExp(venv, tenv, a->u.let.body);
            S_endScope(tenv);
            S_endScope(venv);
            return exp;
        }

        case A_arrayExp: {
            struct expty size = transExp(venv, tenv, a->u.array.size);
            struct expty init = transExp(venv, tenv, a->u.array.init);
            if (size.ty->kind != Ty_int) {
                EM_error(a->pos, "array size should be int");
                return expTy(NULL, Ty_Void());
            }
            if (init.ty->kind == Ty_void) {
                EM_error(a->pos, "init value should not void");
                return expTy(NULL, Ty_Void());
            }
            Ty_ty x = S_look(tenv, a->u.array.typ);
            if (!x) {
                EM_error(a->pos, "undefined variable %s", S_name(a->u.array.typ));
                return expTy(NULL, Ty_Void());
            }
            if (actual_ty(x)->kind != Ty_array) {
                EM_error(a->pos, "variable %s is not an array variable", S_name(a->u.array.typ));
                return expTy(NULL, Ty_Void());
            }
            // TODO
            if (init.ty->kind == Ty_nil || actual_ty(actual_ty(x)->u.array) != init.ty) {
                EM_error(a->pos, "array type is diffierent from init type");
            }

            Tr_exp exp = Tr_call(Temp_namedlabel("init_array"), T_ExpList(unEx(init.exp),
                                                                                T_ExpList(unEx(size.exp), NULL)));
            return expTy(exp, actual_ty(x));
//            return expTy(NULL, actual_ty(x));
            //E_enventry x = S_look(tenv, a->u.array.typ);
            //if (!(x && x->kind == E_varEntry)) {
            //    EM_error(a->pos, "undefined variable %s", S_name(a->u.array.typ));
            //    return expTy(NULL, Ty_Void());
            //}
            //if (x->u.var.ty->kind != Ty_array) {
            //    EM_error(a->pos, "variable %s is not an array variable", S_name(a->u.array.typ));
            //    return expTy(NULL, Ty_Void());
            //}
            //if (x->u.var.ty->u.array != init.ty) {
            //    EM_error(a->pos, "array type is diffierent from init type");
            //}
            //return expTy(NULL, x->u.var.ty);
        }

        default:
            assert(0);
    }
}

void transDec(S_table venv, S_table tenv, A_dec d) {
    switch (d->kind) {
        case A_functionDec: {
            A_fundecList fundecList;
            for (fundecList = d->u.function; fundecList; fundecList = fundecList->tail) {
                A_fundec fundec = fundecList->head;
                Ty_ty resultTy;
                if (fundec->result) {
                    resultTy = S_look(tenv, fundec->result);
                    if (!resultTy) {
                        EM_error(fundec->pos, "undefined type: %s", fundec->result);
                        resultTy = Ty_Int();
                    }
                }
                else {
                    resultTy = Ty_Void();
                }
                Ty_tyList argsTy = makeFormalTyList(tenv, fundec->params);
                //add in frame
                Temp_label temp = Temp_newlabel();
                S_enter(venv, fundec->name, E_FunEntry(Tr_newLevel(global_level, temp, makeUBoolList(fundec->params)),
                                                       temp,
                                                       argsTy,
                                                       resultTy));
            }

            for (fundecList = d->u.function; fundecList; fundecList = fundecList->tail) {
                A_fundec fundec = fundecList->head;
                A_fieldList fieldList = fundec->params;
                E_enventry t = S_look(venv, fundec->name);
                assert(t && t->kind == E_funEntry);
                Ty_tyList tyList = t->u.fun.formals;

                S_beginScope(venv);
                //add in frame
                global_level = t->u.fun.level;
                Tr_accessList accessList = Tr_formals(global_level);

                for (; fieldList && tyList; fieldList = fieldList->tail, tyList = tyList->tail) {
                    //add in frame
                    assert(accessList);
                    S_enter(venv, fieldList->head->name, E_VarEntry(accessList->head, tyList->head));
                    accessList = accessList->tail;
                }
                assert(fieldList == NULL && tyList == NULL);
                struct expty func = transExp(venv, tenv, fundec->body);

                //add in frame
                global_level = global_level->upper;
                S_endScope(venv);

                // TODO
                if (func.ty != actual_ty(t->u.fun.result) && func.ty->kind != Ty_nil) {
                    EM_error(fundec->pos, "function return value is diffierent from expressions type");
                }
                // TEST
                printf("\n--------------------------------------------------\n");
                printf("function: %s\n", S_name(fundec->name));

                printf("---------------------------------------------------\n");
                printStmList(stdout, T_StmList(T_Exp(unEx(func.exp)), NULL));
            }
            break;
        }

        case A_varDec: {
            struct expty e = transExp(venv, tenv, d->u.var.init);
            if (e.ty->kind == Ty_void) {
                EM_error(d->pos, "exp should not be void type");
                return;
            }

            if (d->u.var.typ) {
                Ty_ty typ = S_look(tenv, d->u.var.typ);
                if (!typ) {
                    EM_error(d->pos, "undefined type: %s", S_name(d->u.var.typ));
                    return;
                }
                // 每个变量的类型都是最终形式
                if (e.ty == actual_ty(typ) || e.ty->kind == Ty_nil) {
                    //add in frame
                    S_enter(venv, d->u.var.var, E_VarEntry(Tr_allocLocal(global_level, TRUE), actual_ty(typ)));
                }
                else {
                    EM_error(d->pos, "variable type and initial variable is incompaible");
                }
            }
            else {
                if (e.ty->kind != Ty_nil) {
                    //add in frame
                    S_enter(venv, d->u.var.var, E_VarEntry(Tr_allocLocal(global_level, TRUE), e.ty));
                }
                else {
                    EM_error(d->pos, "initial value should not be nil when no type");

                    //add in frame
                    S_enter(venv, d->u.var.var, E_VarEntry(Tr_allocLocal(global_level, TRUE), Ty_Int()));
                }
            }
            break;
        }

        case A_typeDec: {
            A_nametyList nametyList;
            for (nametyList = d->u.type; nametyList; nametyList = nametyList->tail) {
                S_enter(tenv, nametyList->head->name, Ty_Name(nametyList->head->name, NULL));
            }
            for (nametyList = d->u.type; nametyList; nametyList = nametyList->tail) {
                A_namety namety = nametyList->head;
                Ty_ty ty = S_look(tenv, namety->name);
                assert(ty && ty->kind == Ty_name);
                ty->u.name.ty = transTy(tenv, namety->ty);
            }
            // test actual tys safe
            for (nametyList = d->u.type; nametyList; nametyList = nametyList->tail) {
                A_namety namety = nametyList->head;
                Ty_ty ty = S_look(tenv, namety->name);
                assert(ty);
                test_safe_ty(ty);
            }
            //S_enter(tenv, d->u.type->head->name, transTy(tenv, d->u.type->head->ty));
            break;
        }

        default:
            assert(0);
    }
}

Ty_ty transTy(S_table tenv, A_ty a) {
    switch (a->kind) {
        case A_nameTy: {
            Ty_ty e = S_look(tenv, a->u.name);
            if (e) {
                return e;
            }
            else {
                EM_error(a->pos, "undefined type: %s", S_name(a->u.name));
                return Ty_Int();
            }
        }

        case A_recordTy: {
            A_fieldList  fieldList = a->u.record;
            return Ty_Record(genTyFieldList(tenv, fieldList));
        }

        case A_arrayTy: {
            Ty_ty ty = S_look(tenv, a->u.array);
            if (ty) {
                return Ty_Array(ty);
            }
            else {
                EM_error(a->pos, "undefined type: %s", S_name(a->u.array));
                return Ty_Int();
            }
        }

        default:
            assert(0);
    }
}

void SEM_transProg(A_exp exp) {
    global_level = Tr_outermost();
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();

    struct expty end = transExp(venv, tenv, exp);

//    printf("OK!");

//    Ty_print(end.ty);
    printf("\n-----------------------------------------\n");
    printf("main\n");
    printf("-------------------------------------------\n");
    printStmList(stdout, T_StmList(T_Exp(unEx(end.exp)), NULL));

    //printenv(venv, tenv);


}