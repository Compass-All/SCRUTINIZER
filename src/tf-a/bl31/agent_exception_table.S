#include <asmdefs.h>
#include <asm_macros.S>
.text
.global agent_vector_table
.globl agent_vector_end
#define SC_INTROSPECTION_EXIT 0x80000FF7
.balign 2048

agent_vector_table:
//Current EL with SP_EL0
    b .
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .

//Current EL with SP_EL1
.balign 0x80
    b agent_exit_sync
.balign 0x80
    b agent_exit_irq
.balign 0x80
    b agent_exit_fiq
.balign 0x80
    b agent_exit_error

//Lower EL with SP_EL1 from aarch64 exception
.balign 0x80
    b agent_exit_sync
.balign 0x80
    b agent_exit_irq
.balign 0x80
    b agent_exit_fiq
.balign 0x80
    b agent_exit_error

//Lower EL with SP_EL1 from aarch32 exception
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .

.align	6

//restoration
agent_exit_sync:
    mov_imm	x0, SC_INTROSPECTION_EXIT
    smc     #0

agent_exit_irq:
    mov_imm	x0, SC_INTROSPECTION_EXIT
    smc     #0

agent_exit_fiq:
    mov_imm	x0, SC_INTROSPECTION_EXIT
    smc     #0

agent_exit_error:
    mov_imm	x0, SC_INTROSPECTION_EXIT
    smc     #0

agent_vector_end: