#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "table.h"
#include "types.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "absyn.h"
#include "translate.h"
#include "env.h"

extern Tr_level global_level;

E_enventry E_VarEntry(Tr_access access, Ty_ty ty) {
    E_enventry env = malloc(sizeof(struct E_enventry_));
    env->kind = E_varEntry;
    env->u.var.ty = ty;
    env->u.var.access = access;
    return env;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result) {
    E_enventry env = malloc(sizeof(struct E_enventry_));
    env->kind = E_funEntry;

    env->u.fun.level = level;
    env->u.fun.label = label;

    env->u.fun.formals = formals;
    env->u.fun.result = result;
    return env;
}

/* type environment */
S_table E_base_tenv(void) {
    S_table t = S_empty();
    S_enter(t, S_Symbol("int"), Ty_Int());
    S_enter(t, S_Symbol("string"), Ty_String());
    return t;
}

/* eventry environment */
S_table E_base_venv(void) {
    S_table t = S_empty();
    S_enter(t,
            S_Symbol("print"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       Ty_TyList(Ty_String(), NULL),
                       Ty_Void()));
    S_enter(t,
            S_Symbol("flush"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       NULL,
                       Ty_Void()));
    S_enter(t,
            S_Symbol("getchar"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       NULL,
                       Ty_String()));
    S_enter(t,
            S_Symbol("ord"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       Ty_TyList(Ty_String(), NULL),
                       Ty_Int()));
    S_enter(t,
            S_Symbol("chr"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       Ty_TyList(Ty_Int(), NULL),
                       Ty_String()));
    S_enter(t,
            S_Symbol("size"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       Ty_TyList(Ty_String(), NULL),
                       Ty_Int()));
    S_enter(t,
            S_Symbol("substring"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       Ty_TyList(Ty_String(),
                                 Ty_TyList(Ty_Int(),
                                           Ty_TyList(Ty_Int(), NULL))),
                       Ty_String()));
    S_enter(t,
            S_Symbol("concat"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       Ty_TyList(Ty_String(),
                                 Ty_TyList(Ty_String(), NULL)),
                       Ty_String()));
    S_enter(t,
            S_Symbol("not"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       Ty_TyList(Ty_Int(), NULL),
                       Ty_Int()));
    S_enter(t,
            S_Symbol("exit"),
            E_FunEntry(global_level,
                       Temp_newlabel(),
                       Ty_TyList(Ty_Int(), NULL),
                       Ty_Void()));
    return t;
}