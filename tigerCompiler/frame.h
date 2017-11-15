#ifndef TIGERCOMPILER_FRAME_H
#define TIGERCOMPILER_FRAME_H

typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;

typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};

F_frame F_newFrame(Temp_label name, U_boolList formals);
F_accessList F_formals(F_frame f); /* not use ? */
F_access F_allocLocal(F_frame f, bool escape);

struct F_access_ {
    enum {inFrame, inReg} kind;
    union {
        int offset;
        Temp_temp reg;
    } u;
};

F_access InFrame(int offset);
F_access InReg(Temp_temp reg); /* not use now */
F_accessList F_AccessList(F_access head, F_accessList tail);

struct F_frame_ {
    Temp_label name;
    F_accessList formals;
    int argv_cnt;
    int locv_cnt;
};

Temp_temp F_Fp(void);
T_exp F_Exp(F_access access, T_exp fp);



#endif //TIGERCOMPILER_FRAME_H
