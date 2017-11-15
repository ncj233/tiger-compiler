#ifndef TIGERCOMPILER_TRANSLATE_H
#define TIGERCOMPILER_TRANSLATE_H

typedef struct Tr_access_ *Tr_access;
typedef struct Tr_accessList_ *Tr_accessList;
typedef struct Tr_level_ *Tr_level;

struct Tr_access_ {
    Tr_level level;
    F_access access;
};

struct Tr_accessList_ {
    Tr_access head;
    Tr_accessList tail;
};

struct Tr_level_ {
    Tr_level upper; //out most null
    Temp_label func_name;
    F_frame frame;
    Tr_accessList formals; //may be null
};

Tr_access Tr_Access(Tr_level level, F_access access);
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost(void);
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level); // this function return level->formals
Tr_access Tr_allocLocal(Tr_level level, bool escape);

/* --------------------------------------------------------------------------- */
typedef struct Tr_exp_ *Tr_exp;
typedef struct patchList_ *patchList;

struct patchList_ {
    Temp_label *head;
    patchList tail;
};

struct Cx {
    patchList trues;
    patchList falses;
    T_stm stm;
};

struct Tr_exp_ {
    enum {Tr_ex, Tr_nx, Tr_cx} kind;
    union {T_exp ex; T_stm nx; struct Cx cx; } u;
};

patchList PatchList(Temp_label *head, patchList tail);

Tr_exp Tr_Ex(T_exp ex);
Tr_exp Tr_Nx(T_stm nx);
Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

T_exp unEx(Tr_exp e);
T_stm unNx(Tr_exp e);
struct Cx unCx(Tr_exp e);

void doPatch(patchList tList, Temp_label label);
patchList joinPatch(patchList first, patchList second);

/*---------------------------------------------------------------------------*/
Tr_exp Tr_simpleVar(Tr_access access, Tr_level level);
Tr_exp Tr_subscript(Tr_exp left, Tr_exp index);
Tr_exp Tr_field(Tr_exp left, int offset);

Tr_exp Tr_binOp_int(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_compare_int(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_compare_string(A_oper op, Tr_exp left, Tr_exp right);

Tr_exp Tr_if(Tr_exp cond, Tr_exp then);
Tr_exp Tr_ifElse(Tr_exp cond, Tr_exp then, Tr_exp elsee);

Tr_exp Tr_while(Tr_exp cond, Tr_exp body, Temp_label done);
Tr_exp Tr_for(Tr_access access, Tr_level level, Tr_exp left, Tr_exp right, Tr_exp body, Temp_label done);
Tr_exp Tr_call(Temp_label func, T_expList list);

#endif //TIGERCOMPILER_TRANSLATE_H
