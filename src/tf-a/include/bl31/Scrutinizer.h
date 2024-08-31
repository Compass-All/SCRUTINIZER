#ifndef SCRUTINIZER_H
#define SCRUTINIZER_H
#include <assert.h>
#include <string.h>

#include <arch.h>
#include <arch_features.h>
#include <arch_helpers.h>
#include <bl31/bl31.h>
#include <bl31/ehf.h>
#include <drivers/arm/gic_common.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <common/interrupt_props.h>
#include <drivers/console.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <lib/pmf/pmf.h>
#include <lib/runtime_instr.h>
#include <lib/spinlock.h>
#include <plat/common/platform.h>
#include <services/std_svc.h>
#include <bl31/interrupt_mgmt.h>


#define TASK_MAX 16

#define DATA_ACCESS_READ_FAULT 0x28
#define DATA_ACCESS_WRITE_FAULT 0x68
#define INSTRUCTION_ACCESS__FAULT 0x100028

//assume xFSC, bits [5:0] is 101000, and check WnR, bit [6] and InD, bit [20]
#define ISS_MASK 0x10007F

#define FVP_PMI				92
#define FVP_PMI_2			93
#define FVP_PMI_3			94
#define FVP_PMI_4			95
#define PMI_FLOOD_NUM 1

#define FVP_PMU			    0x22030000
#define FVP_PMU_2			0x22130000
#define FVP_PMU_3			0x23030000
#define FVP_PMU_4			0x23130000

#define GICD_BASE			0x2f000000
#define GICD_END			0x2f010000


#define	GICD_OFFSET_8(REG, id)	\
	(GICD_##REG##R + (uintptr_t)(id))

#define	GICD_OFFSET_64(REG, id)	\
	(GICD_##REG##R + (((uintptr_t)(id) >> REG##R_SHIFT) << 3))

#define FVP_PMU_GROUP0 0x2f000088
#define FVP_PMU_2_GROUP0 0x2f000089
#define FVP_PMU_3_GROUP0 0x2f00008A
#define FVP_PMU_4_GROUP0 0x2f00008B

#define FVP_PMU_PRIORITY 0x2f00045c
#define FVP_PMU_2_PRIORITY 0x2f00045d
#define FVP_PMU_3_PRIORITY 0x2f00045e
#define FVP_PMU_4_PRIORITY 0x2f00045f

#define FVP_PMU_AFFINITY 0x2f0062e0
#define FVP_PMU_2_AFFINITY 0x2f0062e8
#define FVP_PMU_3_AFFINITY 0x2f0062f0
#define FVP_PMU_4_AFFINITY 0x2f0062f8

#define REGISTER_INFO_LENGTH 0x1000

typedef struct register_info {
   u_register_t x0;
   u_register_t x1;
   u_register_t x2;
   u_register_t x3;
   u_register_t x4;
   u_register_t x5;
   u_register_t x6;
   u_register_t x7;
   u_register_t x8;
   u_register_t x9;
   u_register_t x10;
   u_register_t x11;
   u_register_t x12;
   u_register_t x13;
   u_register_t x14;
   u_register_t x15;
   u_register_t x16;
   u_register_t x17;
   u_register_t x18;
   u_register_t x19;
   u_register_t x20;
   u_register_t x21;
   u_register_t x22;
   u_register_t x23;
   u_register_t x24;
   u_register_t x25;
   u_register_t x26;
   u_register_t x27;
   u_register_t x28;
   u_register_t x29;
   u_register_t x30;
   u_register_t scr_el3;
   u_register_t elr_el3;
   u_register_t esr_el3;
   u_register_t far_el3;
   u_register_t sctlr_el3;
   u_register_t ttbr0_el3;
   u_register_t gpccr_el3;
   u_register_t gptbr_el3;
   u_register_t sctlr_el1;
   u_register_t sp_el1;
   u_register_t esr_el1; 
   u_register_t ttbr0_el1;
   u_register_t ttbr1_el1;
   u_register_t vbar_el1; 
   u_register_t spsr_el1;
   u_register_t hcr_el2;   
   u_register_t sctlr_el2;
   u_register_t sp_el2;
   u_register_t esr_el2;
   u_register_t elr_el2;
   u_register_t ttbr0_el2;
   u_register_t vsttbr_el2;
   u_register_t vttbr_el2;
   u_register_t vbar_el2;
   u_register_t spsr_el2;
} sc_reg_info_t;

#define GENERATE_REG_ASSIGNMENTS(reg_info, temp_target_ctx)  \
    do {                                                \
        reg_info->x0  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X0);   \
        reg_info->x1  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X1);   \
        reg_info->x2  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X2);   \
        reg_info->x3  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X3);   \
        reg_info->x4  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X4);   \
        reg_info->x5  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X5);   \
        reg_info->x6  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X6);   \
        reg_info->x7  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X7);   \
        reg_info->x8  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X8);   \
        reg_info->x9  = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X9);   \
        reg_info->x10 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X10);  \
        reg_info->x11 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X11);  \
        reg_info->x12 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X12);  \
        reg_info->x13 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X13);  \
        reg_info->x14 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X14);  \
        reg_info->x15 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X15);  \
        reg_info->x16 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X16);  \
        reg_info->x17 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X17);  \
        reg_info->x18 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X18);  \
        reg_info->x19 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X19);  \
        reg_info->x20 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X20);  \
        reg_info->x21 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X21);  \
        reg_info->x22 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X22);  \
        reg_info->x23 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X23);  \
        reg_info->x24 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X24);  \
        reg_info->x25 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X25);  \
        reg_info->x26 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X26);  \
        reg_info->x27 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X27);  \
        reg_info->x28 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X28);  \
        reg_info->x29 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_X29);  \
        reg_info->x30 = read_ctx_reg(get_gpregs_ctx(temp_target_ctx), CTX_GPREG_LR);  \
    } while (0)

#define GENERATE_EL1_REG_ASSIGNMENTS(reg_info, temp_target_ctx)  \
    do {                                                        \
        reg_info->sctlr_el1 = read_ctx_reg(get_el1_sysregs_ctx(temp_target_ctx), CTX_SCTLR_EL1); \
        reg_info->sp_el1    = read_ctx_reg(get_el1_sysregs_ctx(temp_target_ctx), CTX_SP_EL1);    \
        reg_info->esr_el1   = read_ctx_reg(get_el1_sysregs_ctx(temp_target_ctx), CTX_ESR_EL1);   \
        reg_info->ttbr0_el1 = read_ctx_reg(get_el1_sysregs_ctx(temp_target_ctx), CTX_TTBR0_EL1); \
        reg_info->ttbr1_el1 = read_ctx_reg(get_el1_sysregs_ctx(temp_target_ctx), CTX_TTBR1_EL1); \
        reg_info->vbar_el1  = read_ctx_reg(get_el1_sysregs_ctx(temp_target_ctx), CTX_VBAR_EL1);  \
        reg_info->spsr_el1  = read_ctx_reg(get_el1_sysregs_ctx(temp_target_ctx), CTX_SPSR_EL1);  \
    } while (0)

#define GENERATE_EL2_REG_ASSIGNMENTS(reg_info, temp_target_ctx)  \
    do {                                                        \
        reg_info->hcr_el2    = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_HCR_EL2);    \
        reg_info->sctlr_el2  = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_SCTLR_EL2);  \
        reg_info->sp_el2     = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_SP_EL2);     \
        reg_info->esr_el2    = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_ESR_EL2);    \
        reg_info->elr_el2    = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_ELR_EL2);    \
        reg_info->ttbr0_el2  = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_TTBR0_EL2);  \
        reg_info->vsttbr_el2 = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_VSTTBR_EL2); \
        reg_info->vttbr_el2  = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_VTTBR_EL2);  \
        reg_info->vbar_el2   = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_VBAR_EL2);   \
        reg_info->spsr_el2   = read_ctx_reg(get_el2_sysregs_ctx(temp_target_ctx), CTX_SPSR_EL2);   \
    } while (0)

#define GENERATE_EL3_REG_ASSIGNMENTS(reg_info)           \
    do {                                                 \
        reg_info->scr_el3  = read_scr_el3();             \
        reg_info->elr_el3  = read_elr_el3();             \
        reg_info->esr_el3  = read_esr_el3();             \
        reg_info->far_el3  = read_far_el3();             \
        reg_info->sctlr_el3 = read_sctlr_el3();          \
        reg_info->ttbr0_el3 = read_ttbr0_el3();          \
        reg_info->gpccr_el3 = read_gpccr_el3();          \
        reg_info->gptbr_el3 = read_gptbr_el3();          \
    } while (0)

typedef struct {
	int type; //1 watchpoint; 2 breakpoint
	bool inited;
	uint64_t addr; // setting address
    uint64_t aligned_addr; // the address for GPT page
    uint64_t mem_addr;
    uint64_t mem_size;
    uint64_t mem_el;
    uint64_t reg_buf;
    uint64_t pa_access;
} debug_task_t;


/**
 * Gets the Instruction Length bit for the synchronous exception
 */
#define GET_ESR_IL(esr) ((esr) & (1 << 25))
#define GET_NEXT_PC_INC(esr) (GET_ESR_IL(esr) ? 4 : 2)

#define READ_SYSREG(dst, sysreg) asm("mrs %0, " #sysreg \
                                     : "=r"(dst));

#define WRITE_SYSREG(src, sysreg) \
    {                             \
        asm("isb");               \
        asm("msr " #sysreg ", %0" \
            :                     \
            : "r"(src));          \
    }

#define READ_REG(dst, reg) asm("mov %0, " #reg \
                                     : "=r"(dst));

#define WRITE_REG(src, reg) \
    {                             \
        asm("mov " #reg ", %0" \
            :                     \
            : "r"(src));          \
    }

int SCRUTINIZER_INIT();

void enable_PMU(int debug_type);

void scrutinizer_interrupt_handler();

int handle_gpf_trap(uint64_t esr_el3, cpu_context_t *ctx);

size_t search_empty_task(debug_task_t* tasks);

size_t search_debug_task(debug_task_t* tasks, uint64_t target_addr, int type);

u_register_t sc_watchpoint(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4, u_register_t arg5, u_register_t arg6);

u_register_t sc_breakpoint(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4, u_register_t arg5, u_register_t arg6);

u_register_t sc_introspection(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t flags);

u_register_t sc_register_info(u_register_t arg1, u_register_t arg2, u_register_t arg3);

u_register_t sc_pa_introspection(u_register_t arg1, u_register_t target_el, u_register_t size, u_register_t optimize, u_register_t pa_access_model);

u_register_t sc_ete_trace(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4);

u_register_t sc_disable_ete_trace(u_register_t arg1, u_register_t arg2, u_register_t arg3);

u_register_t sc_stepping_model(u_register_t arg1, u_register_t arg2, u_register_t arg3);

u_register_t sc_disable_stepping_model(u_register_t arg1, u_register_t arg2, u_register_t arg3);

u_register_t sc_legal_auth(u_register_t arg1, u_register_t arg2, u_register_t arg3);

u_register_t sc_introspection_exit(u_register_t arg1, u_register_t arg2, u_register_t arg3);


#define AGENT_BASE_ADDRESS 0xFA000000
#define AGENT_END_ADDRESS 0xFBC00000

#define AGENT_CODE_ADDRESS 0xFA000000
#define AGENT_EXCEPTION_VECTOR_TABLE_ADDRESS 0xFA001000
#define AGENT_STACK_ADDRESS 0xFA002000
#define AGENT_STACK_ADDRESS_END 0xFA009000
#define AGENT_PT_ADDRESS 0xFA009000

#define PG_PHYS_ADDR 0xFA009000
#define PG_PHYS_END_ADDR 0xFA800000

#define EL2_PGD_PHYS_ADDR 0xFA009000
#define EL2_PMD_PHYS_ADDR 0xFA00A000
#define EL2_PAGE_TABLE_SPACE_SIZE 0x1F7000

#define EL1_S1_PGD_PHYS_ADDR 0xFA200000
#define EL1_S1_PMD_PHYS_ADDR 0xFA201000
#define EL1_S1_PAGE_TABLE_SPACE_SIZE 0x300000

#define EL1_S2_PGD_PHYS_ADDR 0xFA500000
#define EL1_S2_PMD_PHYS_ADDR 0xFA501000
#define EL1_S2_PUD_PHYS_ADDR 0xFA502000
#define EL1_S2_PAGE_TABLE_SPACE_SIZE 0x300000

#define AGENT_DATA_ADDRESS 0xFA800000
#define AGENT_DATA_ADDRESS_END 0xFBC00000
#define AGENT_DATA_SIZE 0x1400000

#define GPT_AG_ADDRESS 0xFBC00000
#define GPT_AG_END_ADDRESS 0xFC000000


#define ARM_L0_GPT_AG_ADDR_BASE		ARM_L1_GPT_AG_ADDR_BASE + ARM_L1_GPT_AG_SIZE
#define ARM_L0_GPT_AG_SIZE          ARM_L0_GPT_SIZE
#define ARM_L1_GPT_AG_ADDR_BASE	    GPT_AG_ADDRESS
#define ARM_L1_GPT_AG_SIZE  ARM_L1_GPT_SIZE

#endif /* SCRUTINIZER_H */