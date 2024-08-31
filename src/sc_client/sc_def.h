#ifndef SC_DEF_H
#define SC_DEF_H


#define REGISTER_INFO_LENGTH 0x1000

typedef unsigned long u_register_t;

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

// The secure world PAS region that scrutinizer currently focus on.
#define PLAT_ARM_TRUSTED_DRAM_BASE	0x06000000
#define PLAT_ARM_TRUSTED_DRAM_SIZE	0x02000000	/* 32 MB */


#define AGENT_DATA_ADDRESS 0xFA800000
#define AGENT_DATA_ADDRESS_END 0xFBC00000
#define AGENT_DATA_SIZE 0x1400000

#define TRBE_BASE_ADDRESS 0xF8000000
#define TRBE_LIMIT_ADDRESS 0xFA000000
#define TRBE_SIZE 0x2000000

typedef struct memory_acquisition_info {
    unsigned long addr;
    unsigned long size;
    unsigned long el;
    unsigned long watchpoint_addr;
    unsigned long breakpoint_addr;
    int pa_access;
    unsigned long reg_share_buf;
} sc_mem_info_t;

typedef struct instruction_info {
    unsigned long start_addr;
    unsigned long end_addr;
    unsigned long el;
    unsigned long enable_inst_trace;
    unsigned long ins_share_buf_phys;
} sc_ins_info_t;

typedef struct sc_config_info {
    sc_mem_info_t mem_info;
    sc_ins_info_t ins_info;
    sc_reg_info_t reg_info;
} sc_ctl_info_t;

#define DEV_PATH "/dev/SC"
#define SC_AUTH_TEST _IOW('m', 1, unsigned int)
#define SC_REGISTER _IOW('m', 2, unsigned int)
#define SC_MEMORY_DUMP	_IOW('m', 3, unsigned int)
#define SC_FULL_MEMORY_DUMP	_IOW('m', 4, unsigned int)
#define SC_REGISTER_SAVE	_IOW('m', 5, unsigned int)
#define SC_ETE_ON	_IOW('m', 6, unsigned int)
#define SC_ETE_OFF	_IOW('m', 7, unsigned int)
#define SC_SET_WATCHPOINT	_IOW('m', 8, unsigned int)
#define SC_SET_BREAKPOINT	_IOW('m', 9, unsigned int)

#endif