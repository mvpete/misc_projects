#ifndef __op_codes_h__
#define __op_codes_h__

enum class op_codes
{
    op_br = 0,
    op_add,
    op_ld,
    op_st,
    op_jsr,
    op_and,
    op_ldr,
    op_str,
    op_rti,
    op_not,
    op_ldi,
    op_sti,
    op_jmp,
    op_res,
    op_lea,
    op_trap
};

#endif 
