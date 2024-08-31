#include <bl31/Scrutinizer.h>
#include <bl31/Scrutinizer_pmu.h>
#include <bl31/Scrutinizer_ete.h>
#include <bl31/Scrutinizer_auth.h>
#include <arch_helpers.h>
#include <bl31/sync_handle.h>
#include <context.h>
#include <plat/arm/common/arm_pas_def.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <platform_def.h>
#include <string.h>
#include <lib/el3_runtime/context_mgmt.h>

#include <lib/gpt_rme/gpt_rme.h>
#include <drivers/arm/gicv3.h>
#include <drivers/arm/arm_gicv3_common.h>

debug_task_t wat_tasks[TASK_MAX];
static spinlock_t gpt_lock2;
uint64_t legal_flag = 0;
uint64_t wat_addr_for_pmi = 0;
uint64_t brk_addr_for_pmi = 0;
uint64_t stepping_addr_for_pmi;
uint64_t pmi_ins_addr = 0;
int flood_flag = 0;
int debug_type_for_pmi = 0;
int TRBE_buffer_flag =0;
cpu_context_t agent_cpu_ctx;
cpu_context_t *target_ctx;
int has_agent_el2_local_mappings = 0;
int has_agent_s1_local_mappings = 0;
int has_agent_s2_local_mappings = 0;
u_register_t src_hrc_el2;
u_register_t src_mdcr_el3;
u_register_t src_cptr_el3;
int enable_PA_access_optimization = 0;
struct mem_buffer crypto_mem_buffer;
int smc_from_flag = 0;
uint64_t ins_share_buf = 0;

extern void __attribute__ ((visibility ("hidden"))) agent_entry(void);
extern void __attribute__ ((visibility ("hidden"))) agent_entry_end(void);

extern void __attribute__ ((visibility ("hidden"))) agent_vector_table(void);
extern void __attribute__ ((visibility ("hidden"))) agent_vector_end(void);

int SCRUTINIZER_INIT()
{
    NOTICE("SCRUTINIZER INIT\n");

    int rc __unused;

    rc = mmap_add_dynamic_region((unsigned long long)AGENT_BASE_ADDRESS,
				     AGENT_BASE_ADDRESS,
				     ARM_AGENT_SIZE,
				     MT_MEMORY | MT_RW | MT_SECURE);
	if (rc != 0) {
		panic();
	}
    fast_memset((void*)AGENT_BASE_ADDRESS, 0, ARM_AGENT_SIZE);
    memcpy((void*)AGENT_BASE_ADDRESS, &agent_entry, &agent_entry_end - &agent_entry);
    memcpy((void*)AGENT_EXCEPTION_VECTOR_TABLE_ADDRESS, &agent_vector_table, &agent_vector_end - &agent_vector_table);

    rc = mmap_add_dynamic_region((unsigned long long)GPT_AG_ADDRESS,
				     GPT_AG_ADDRESS,
				     ARM_GPT_AG_SIZE,
				     MT_MEMORY | MT_RW | MT_ROOT);
	if (rc != 0) {
		panic();
	}
    memset((void *)GPT_AG_ADDRESS, 0, ARM_GPT_AG_SIZE);

    rc = mmap_add_dynamic_region((unsigned long long)TRBE_BASE_ADDRESS,
				     TRBE_BASE_ADDRESS,
				     TRBE_SIZE,
				     MT_MEMORY | MT_RW | MT_ROOT);
	if (rc != 0) {
		panic();
	}
    fast_memset((void *)TRBE_BASE_ADDRESS, 0, TRBE_SIZE);

    gpt_ag_build();
    gpt_trans_multi_pages(0, AGENT_BASE_ADDRESS, AGENT_END_ADDRESS - AGENT_BASE_ADDRESS, GPT_GPI_NO_ACCESS);
    return 0;
}


uint64_t get_phys_from_tz_virt(uint64_t virt, int el)
{	
	uint64_t par, pa;
	if(el == 2){
         ats1e2r(virt);}
    else{
        AT(ats1e1r, virt);
    }
    isb();
	par = read_par_el1();
	/* If the translation resulted in fault, return failure */
	if ((par & PAR_F_MASK) != 0)
		return 0;
	/* Extract Physical Address from PAR */
	pa = (par & (PAR_ADDR_MASK << PAR_ADDR_SHIFT));
	//Note that the par only output the address bits[47:12] or [51:12], so we add the later 12 bits to restore the correct pa
	pa = pa + (virt & 0xFFF);
	NOTICE("AT translation the pa is 0x%lx\n", pa);
	return pa;
}

u_register_t sc_legal_auth(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{	
    spin_lock(&gpt_lock2);
    // NOTICE("sig:0x%lx\n", arg1);
    if(check_auth(arg1))
    {
        NOTICE("Auth pass\n");
        legal_flag = 1;
    }   
    else
        NOTICE("Fail to pass auth\n");
	spin_unlock(&gpt_lock2);

	return 0;

}

u_register_t sc_pa_introspection(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4, u_register_t arg5)
{	
	
    if(legal_flag ==0)
    {
       NOTICE("invalid user\n");
        return 0 ;
    }
    legal_flag = 0;
    spin_lock(&gpt_lock2);
    int rc __unused;
    uint64_t pa_addr = arg1;
    uint64_t size = arg2;
    if(size > AGENT_DATA_SIZE)
        size = AGENT_DATA_SIZE;
    rc = mmap_add_dynamic_region((unsigned long long)PLAT_ARM_TRUSTED_DRAM_BASE,
                PLAT_ARM_TRUSTED_DRAM_BASE,
                PLAT_ARM_TRUSTED_DRAM_SIZE,
                    MT_MEMORY | MT_RW | MT_SECURE);
    if (rc != 0) {
        panic();
    }

    rc = mmap_add_dynamic_region((unsigned long long)ARM_PAS_2_BASE,
                    ARM_PAS_2_BASE,
                    ARM_PAS_2_SIZE - ARM_TRBE_SIZE - ARM_AGENT_SIZE - ARM_GPT_AG_SIZE,
                    MT_MEMORY | MT_RW | MT_NS);
    if (rc != 0) {
        ERROR("Error while mapping KERNEL_DRAM (%d).\n", rc);
        panic();
    }

    rc = mmap_add_dynamic_region((unsigned long long)ARM_PAS_4_BASE,
                    ARM_PAS_4_BASE,
                    ARM_PAS_4_SIZE,
                    MT_MEMORY | MT_RW | MT_NS);
    if (rc != 0) {
        ERROR("Error while mapping KERNEL_1_DRAM (%d).\n", rc);
        panic();
    }

    gpt_switch(1);
    gpt_trans_multi_pages(0, AGENT_DATA_ADDRESS, AGENT_DATA_SIZE, GPT_GPI_NO_ACCESS);
    fast_memset((void *)AGENT_DATA_ADDRESS, 0, AGENT_DATA_SIZE);
    fast_memcpy((void*)AGENT_DATA_ADDRESS, (void*)pa_addr, size);
    crypto_mem_buffer.virt_base = (void*)AGENT_DATA_ADDRESS;
    crypto_mem_buffer.size = AGENT_DATA_SIZE;
    crypto_mem_buffer.flag = 0x18;
    crypto_mem_buffer.hash = 0;
    protect_str_buffer(&crypto_mem_buffer);
    gpt_switch(0);
    gpt_trans_multi_pages(0, AGENT_DATA_ADDRESS, AGENT_DATA_SIZE, GPT_GPI_NS);

    rc = mmap_remove_dynamic_region(PLAT_ARM_TRUSTED_DRAM_BASE, PLAT_ARM_TRUSTED_DRAM_SIZE);
    if (rc != 0) {
            panic();
    }
    rc = mmap_remove_dynamic_region(ARM_PAS_2_BASE, ARM_PAS_2_SIZE - ARM_TRBE_SIZE - ARM_AGENT_SIZE - ARM_GPT_AG_SIZE);
    if (rc != 0) {
            panic();
    }
    rc = mmap_remove_dynamic_region(ARM_PAS_4_BASE, ARM_PAS_4_SIZE);
    if (rc != 0) {
            panic();
    }
	spin_unlock(&gpt_lock2);
	NOTICE("Finish sc_pa_introspection\n");
	return 0;
}


void allocate_s1_local_mappings()
{
    //0xFA000000 - 0xFA800000 X-only; 0xFA800000 - 0xFBC00000 R/W
    unsigned long long  *pud_addr_desc = (unsigned long long *)(EL1_S1_PMD_PHYS_ADDR);
    pud_addr_desc+= (0xe80/8);
    unsigned long long desc_val = AGENT_BASE_ADDRESS;
    int i = 0;
    int size = (AGENT_DATA_ADDRESS-AGENT_BASE_ADDRESS)/0x200000;
    for (i = 0; i < size; i++){
            *pud_addr_desc = desc_val | 0xF8D;
            pud_addr_desc++;
            desc_val+= 0x200000;
    }
    desc_val = AGENT_DATA_ADDRESS;
    size = (AGENT_DATA_ADDRESS_END - AGENT_DATA_ADDRESS)/0x200000 ;
    for (i =0; i < size; i++){
        *pud_addr_desc = desc_val | 0x60000000000F0D;
        pud_addr_desc++;
        desc_val+= 0x200000;
    }
}

void allocate_s2_local_mappings()
{
    unsigned long long  *pud_addr_desc = (unsigned long long *)(EL1_S2_PUD_PHYS_ADDR);
    pud_addr_desc+= (0xe80/8);
    unsigned long long desc_val = AGENT_BASE_ADDRESS;
    int i = 0;
    int size = (AGENT_DATA_ADDRESS-AGENT_BASE_ADDRESS)/0x200000;
    for (i = 0; i < size; i++){
            *pud_addr_desc = desc_val | 0x1800000000007FD;
            pud_addr_desc++;
            desc_val+= 0x200000;
    }
    desc_val = AGENT_DATA_ADDRESS;
    size = (AGENT_DATA_ADDRESS_END - AGENT_DATA_ADDRESS)/0x200000 ;
    for (i =0; i < size; i++){
        *pud_addr_desc = desc_val | 0x1800000000007FD;
        pud_addr_desc++;
        desc_val+= 0x200000;
    }
}

void allocate_el2_local_mappings()
{
   //0xFA000000 - 0xFA800000 X-only; 0xFA800000 - 0xFBC00000 R/W
   unsigned long long  *pmd_addr_desc = (unsigned long long *)EL2_PMD_PHYS_ADDR;
   pmd_addr_desc += (0x18/8);
   *pmd_addr_desc = (EL2_PMD_PHYS_ADDR + 0x1000) | 0x3;
   unsigned long long  *pud_addr_desc = (unsigned long long *)(EL2_PMD_PHYS_ADDR + 0x1000);
   pud_addr_desc+= (0xe80/8);
   unsigned long long desc_val = AGENT_BASE_ADDRESS;
   int i = 0;
   int size = (AGENT_DATA_ADDRESS-AGENT_BASE_ADDRESS)/0x200000;
   for (i = 0; i < size; i++){
        *pud_addr_desc = desc_val | 0x184000000000785;
        pud_addr_desc++;
        desc_val+= 0x200000;
   }
    desc_val = AGENT_DATA_ADDRESS;
    size = (AGENT_DATA_ADDRESS_END - AGENT_DATA_ADDRESS)/0x200000 ;
    for (i =0; i < size; i++){
        *pud_addr_desc = desc_val | 0x01C0000000000705;
        pud_addr_desc++;
        desc_val+= 0x200000;
   }
}

uint64_t copy_page_table(int model, uint64_t *agent_ttbr0_el2, uint64_t *agent_ttbr0_el1, uint64_t *agent_vsttbr_el2, 
    uint64_t src_ttbr0_el2, uint64_t src_ttbr0_el1, uint64_t src_vsttbr_el2)
{
    unsigned long long  *addr_desc;
    unsigned long long  *addr_src;
    unsigned long long  *empty_addr_desc;
	unsigned long long long_desc;
    uint64_t agent_va_offset=0;
    int rc __unused;
    int has_find_empty_entry = 0;
    //el2
    if(model == 0)
    {
        memset((void*)EL2_PGD_PHYS_ADDR, 0, 0x1000);
        src_ttbr0_el2 &= (~0x1); 
        NOTICE("Src TTBR0_EL2: %lx\n", src_ttbr0_el2);
        rc = mmap_add_dynamic_region((unsigned long long)src_ttbr0_el2, src_ttbr0_el2, 0x1000, MT_MEMORY | MT_RW | MT_SECURE);
        if (rc != 0) {
            panic();
        }
        addr_desc = (unsigned long long *)EL2_PGD_PHYS_ADDR;
        addr_src = (unsigned long long *)src_ttbr0_el2;
        //4-level; 1 entry = 0x8000000000 
        //copy L0 page table
        for(int i = 0; i < 512; i++){		
            if(*addr_src!=0){
                    long_desc = *addr_src;
                    long_desc |= 0x60000000000000;//remove target's X permissions
                    *addr_desc = long_desc;
                    addr_desc++;
                    addr_src++;
                }
            //integrate agent's local mappings to empty L0 table entry.
            else if (has_find_empty_entry == 0)
            {
                empty_addr_desc = addr_desc;
                has_find_empty_entry = 1;
                *empty_addr_desc = EL2_PMD_PHYS_ADDR | 0x3;
                addr_desc++;
                addr_src++;
                agent_va_offset = i * 0x8000000000;
            }
        }
        //pre-allocated agent local mappings once
        if(has_agent_el2_local_mappings == 0)
        {
            allocate_el2_local_mappings();
            has_agent_el2_local_mappings = 1;
        }
        //finish grafting mappings
        *agent_ttbr0_el2 = EL2_PGD_PHYS_ADDR;

        rc = mmap_remove_dynamic_region(src_ttbr0_el2, 0x1000);
        if (rc != 0) {
            panic();
        }
    }
    //el1&0
    else if (model == 1)
    {
        memset((void*)EL1_S1_PGD_PHYS_ADDR, 0, 0x1000);
        memset((void*)EL1_S2_PGD_PHYS_ADDR, 0, 0x2000);
        src_ttbr0_el1 &= (~0x1); 
        NOTICE("Src TTBR0_EL1: %lx\n", src_ttbr0_el1);
        rc = mmap_add_dynamic_region((unsigned long long)src_ttbr0_el1, src_ttbr0_el1, 0x1000, MT_MEMORY | MT_RW | MT_SECURE);
         if (rc != 0) {
            panic();
        }
        src_vsttbr_el2 &= (~0x1); 
        NOTICE("Src VSTTBR_EL2: %lx\n", src_vsttbr_el2);
        rc = mmap_add_dynamic_region((unsigned long long)src_vsttbr_el2, src_vsttbr_el2, 0x1000, MT_MEMORY | MT_RW | MT_SECURE);
        if (rc != 0) {
            panic();
        }

        //stage1
        addr_desc = (unsigned long long *)EL1_S1_PGD_PHYS_ADDR;
        addr_src = (unsigned long long *)src_ttbr0_el1;
        //3-level; 1 entry = 0x40000000
        empty_addr_desc = addr_desc;
        for(int i = 0; i < 512; i++){	
          if(*addr_src!=0){
                long_desc = *addr_src;
                long_desc |= 0x60000000000000;//remove target's X permissions
                *addr_desc = long_desc;
                addr_desc++;
                addr_src++;
            }
        }
        empty_addr_desc += (0x18/8);
        *empty_addr_desc = EL1_S1_PMD_PHYS_ADDR | 0x3;
        //pre-allocated agent local mappings once
        if(has_agent_s1_local_mappings == 0)
        {
            allocate_s1_local_mappings();
            has_agent_s1_local_mappings = 1;
        }

        //stage 2
        has_find_empty_entry = 0;
        addr_desc = (unsigned long long *)EL1_S2_PGD_PHYS_ADDR;
        addr_src = (unsigned long long *)src_vsttbr_el2;
        for(int i = 0; i < 512; i++){		
            if(*addr_src!=0){
                long_desc = *addr_src;
                *addr_desc = long_desc;
                addr_desc++;
                addr_src++;
            }
        }
        addr_desc = (unsigned long long *)EL1_S2_PGD_PHYS_ADDR;
        addr_src = (unsigned long long *)src_vsttbr_el2;
        *addr_desc = EL1_S2_PMD_PHYS_ADDR | 0x3;
        addr_desc = (unsigned long long *) EL1_S2_PMD_PHYS_ADDR;
        uint64_t pmd_desc = (*addr_src) & ~0x3;
        addr_src = (unsigned long long *)pmd_desc;
        rc = mmap_add_dynamic_region((unsigned long long)pmd_desc, pmd_desc, 0x1000, MT_MEMORY | MT_RW | MT_SECURE);
         if (rc != 0) {
            panic();
        }
        for(int i = 0; i < 512; i++){		
            if(*addr_src!=0){
                long_desc = *addr_src;
                *addr_desc = long_desc;
                addr_desc++;
                addr_src++;
            }
        }
        empty_addr_desc = (unsigned long long *) EL1_S2_PMD_PHYS_ADDR;
        empty_addr_desc += (0x18/8);
        *empty_addr_desc = EL1_S2_PUD_PHYS_ADDR | 0x3;
        //pre-allocated agent local mappings once
        if(has_agent_s2_local_mappings == 0)
        {
            allocate_s2_local_mappings();
            has_agent_s2_local_mappings = 1;
        }

        //finish grafting mappings
        *agent_ttbr0_el1 = EL1_S1_PGD_PHYS_ADDR;
        *agent_vsttbr_el2 = EL1_S2_PGD_PHYS_ADDR;

        rc = mmap_remove_dynamic_region(src_ttbr0_el1, 0x1000);
        if (rc != 0) {
            panic();
        }
        rc = mmap_remove_dynamic_region(src_vsttbr_el2, 0x1000);
        if (rc != 0) {
            panic();
        }
        rc = mmap_remove_dynamic_region(pmd_desc, 0x1000);
        if (rc != 0) {
            panic();
        }
    }

    return agent_va_offset;
   
}

u_register_t sc_introspection(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t flags)
{	
	
    if(legal_flag ==0)
    {
        return 0 ;
    }
    legal_flag = 0;
    spin_lock(&gpt_lock2);
    gpt_trans_multi_pages(0, AGENT_DATA_ADDRESS, AGENT_DATA_ADDRESS_END - AGENT_DATA_ADDRESS, GPT_GPI_NO_ACCESS);
    gpt_switch(1);
    uint64_t agent_va_offset = 0;
    u_register_t src_address = arg1;
    u_register_t size = arg2;
    u_register_t target_el = arg3;
    u_register_t ret = 0;

    if(target_el != 2 && target_el != 1&& target_el != 0)
    {
        panic();
    }
    else
         NOTICE("target exception level %lu\n",target_el);
    
    if(flags == 0x3378)
    {
        cm_el1_sysregs_context_save(SECURE);
        cm_el2_sysregs_context_save(SECURE);
        smc_from_flag = 2;
    }
    else
    {
        uint32_t src_sec_state;
        /* Determine which security state this SMC originated from */
        src_sec_state = caller_sec_state(flags);
        if(src_sec_state == SMC_FROM_NON_SECURE)
        {
            cm_el1_sysregs_context_save(NON_SECURE);
            cm_el2_sysregs_context_save(NON_SECURE);
            smc_from_flag = 1;
        }   
        
        else if (src_sec_state == SMC_FROM_SECURE)
        {
            cm_el1_sysregs_context_save(SECURE);
            cm_el2_sysregs_context_save(SECURE);
            smc_from_flag = 2;
        }
    }
   
	target_ctx  = cm_get_context(SECURE);
    agent_cpu_ctx = *target_ctx;
    cm_set_context(&agent_cpu_ctx, SECURE);

    write_ctx_reg(get_gpregs_ctx(&agent_cpu_ctx), CTX_GPREG_X1, src_address);
    write_ctx_reg(get_gpregs_ctx(&agent_cpu_ctx), CTX_GPREG_X2, size);
    write_ctx_reg(get_gpregs_ctx(&agent_cpu_ctx), CTX_GPREG_X3, target_el);
    fast_memset((void *)AGENT_STACK_ADDRESS, 0, AGENT_STACK_ADDRESS_END - AGENT_STACK_ADDRESS);
    fast_memset((void *)AGENT_DATA_ADDRESS, 0, AGENT_DATA_ADDRESS_END - AGENT_DATA_ADDRESS);

    if(target_el == 2)
    {
        uint64_t agent_ttbr0_el2 = 0;
        uint64_t src_ttbr0_el2 = read_ctx_reg(get_el2_sysregs_ctx(&agent_cpu_ctx), CTX_TTBR0_EL2);
        agent_va_offset = copy_page_table(0, &agent_ttbr0_el2, NULL, NULL,src_ttbr0_el2,0,0);
        if(agent_ttbr0_el2 == 0)
        {
            NOTICE("copy page table fault\n");
            panic();
        }
        else
        {
            write_ctx_reg(get_el2_sysregs_ctx(&agent_cpu_ctx), CTX_TTBR0_EL2, agent_ttbr0_el2 & (~0x1));
            write_ctx_reg(get_el2_sysregs_ctx(&agent_cpu_ctx), CTX_SP_EL2, (AGENT_STACK_ADDRESS+agent_va_offset));
            u_register_t agent_spsr_el3 = SPSR_64(MODE_EL2, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
            write_ctx_reg(get_el2_sysregs_ctx(&agent_cpu_ctx), CTX_VBAR_EL2, AGENT_EXCEPTION_VECTOR_TABLE_ADDRESS+agent_va_offset);
            u_register_t dest_address = AGENT_DATA_ADDRESS + agent_va_offset;
            write_ctx_reg(get_gpregs_ctx(&agent_cpu_ctx), CTX_GPREG_X0, dest_address); 
            cm_set_elr_spsr_el3(SECURE, (uintptr_t)(AGENT_BASE_ADDRESS + agent_va_offset), agent_spsr_el3);
            ret = dest_address;
        }
    }
    else if (target_el == 0 || target_el == 1)
    {
        uint64_t agent_ttbr0_el1 = 0;
        uint64_t agent_vsttbr_el2 = 0;
        uint64_t src_ttbr0_el1 = read_ctx_reg(get_el1_sysregs_ctx(&agent_cpu_ctx), CTX_TTBR0_EL1);
        if(src_ttbr0_el1 == 0x0)
            src_ttbr0_el1 = 0x6321000;
        uint64_t src_vsttbr_el2 = read_ctx_reg(get_el2_sysregs_ctx(&agent_cpu_ctx), CTX_VSTTBR_EL2);

        agent_va_offset = copy_page_table(1, NULL, &agent_ttbr0_el1, &agent_vsttbr_el2, 0, src_ttbr0_el1, src_vsttbr_el2);
        
        if(agent_ttbr0_el1 == 0 || agent_vsttbr_el2 == 0 )
        {
            NOTICE("copy page table fault\n");
            panic();
        }
        else
        {
            write_ctx_reg(get_el1_sysregs_ctx(&agent_cpu_ctx), CTX_TTBR0_EL1, agent_ttbr0_el1 & (~0x1));
            write_ctx_reg(get_el2_sysregs_ctx(&agent_cpu_ctx), CTX_VSTTBR_EL2, agent_vsttbr_el2 & (~0x1));
            write_ctx_reg(get_el1_sysregs_ctx(&agent_cpu_ctx), CTX_VBAR_EL1, AGENT_EXCEPTION_VECTOR_TABLE_ADDRESS);
            u_register_t hrc_el2 = read_ctx_reg(get_el2_sysregs_ctx(&agent_cpu_ctx), CTX_HCR_EL2);
            write_ctx_reg(get_el2_sysregs_ctx(&agent_cpu_ctx), CTX_HCR_EL2, hrc_el2 & (~0x80018));
            write_ctx_reg(get_el1_sysregs_ctx(&agent_cpu_ctx), CTX_SP_EL1, (AGENT_STACK_ADDRESS+agent_va_offset));
            u_register_t agent_spsr_el3 = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
            u_register_t dest_address = AGENT_DATA_ADDRESS + agent_va_offset;
            write_ctx_reg(get_gpregs_ctx(&agent_cpu_ctx), CTX_GPREG_X0, dest_address); 
            cm_set_elr_spsr_el3(SECURE, (uintptr_t)(AGENT_BASE_ADDRESS + agent_va_offset), agent_spsr_el3);
            ret = dest_address;
        }
    }

    cm_el1_sysregs_context_restore(SECURE);
	cm_el2_sysregs_context_restore(SECURE);
    cm_set_next_eret_context(SECURE);
	spin_unlock(&gpt_lock2);
	NOTICE("start sc_agent introspection\n");
	return ret;
}

u_register_t sc_introspection_exit(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{

    u_register_t x0 = 0;
    memset(&agent_cpu_ctx, 0, sizeof(cpu_context_t));
    if(smc_from_flag == 1)
    {
        cpu_context_t *ori_ctx = cm_get_context(NON_SECURE);
        cm_set_context(ori_ctx, NON_SECURE);
        cm_el1_sysregs_context_restore(NON_SECURE);
	    cm_el2_sysregs_context_restore(NON_SECURE);
        cm_set_next_eret_context(NON_SECURE);
        x0 = read_ctx_reg(get_gpregs_ctx(ori_ctx), CTX_GPREG_X0);
        cm_set_context(target_ctx, SECURE);
        smc_from_flag = 0;
    }
    else if (smc_from_flag == 2)
    {
        cm_set_context(target_ctx, SECURE);
        cm_el1_sysregs_context_restore(SECURE);
        cm_el2_sysregs_context_restore(SECURE);
        cm_set_next_eret_context(SECURE);
        x0 = read_ctx_reg(get_gpregs_ctx(target_ctx), CTX_GPREG_X0);
        smc_from_flag = 0;
    }

    //encryption
    crypto_mem_buffer.virt_base = (void*)AGENT_DATA_ADDRESS;
    crypto_mem_buffer.size = (AGENT_DATA_ADDRESS_END-AGENT_DATA_ADDRESS);
    crypto_mem_buffer.flag = 0x18;
    crypto_mem_buffer.hash = 0;
    protect_str_buffer(&crypto_mem_buffer);
    NOTICE("sc_agent introspection exit\n");
    gpt_switch(0);
    gpt_trans_multi_pages(0, AGENT_DATA_ADDRESS, AGENT_DATA_ADDRESS_END - AGENT_DATA_ADDRESS, GPT_GPI_NS);
    return x0;
}

void populate_general_registers(sc_reg_info_t *reg_info,  cpu_context_t *temp_target_ctx) {
    GENERATE_REG_ASSIGNMENTS(reg_info, temp_target_ctx);
}

void populate_sys_registers(sc_reg_info_t *reg_info, cpu_context_t *temp_target_ctx) {
    GENERATE_EL1_REG_ASSIGNMENTS(reg_info, temp_target_ctx);
    GENERATE_EL2_REG_ASSIGNMENTS(reg_info, temp_target_ctx);
}

void populate_el3_registers(sc_reg_info_t *reg_info) {
    GENERATE_EL3_REG_ASSIGNMENTS(reg_info);
}

u_register_t sc_register_info(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{	
	
    if(legal_flag ==0)
    {
       NOTICE("invalid user\n");
        return 0 ;
    }
    legal_flag = 0;

    spin_lock(&gpt_lock2);
    int rc __unused;
    unsigned long s_phys = arg1;
    
    sc_reg_info_t *reg_info;

    gpt_trans_multi_pages(0, s_phys, REGISTER_INFO_LENGTH, GPT_GPI_ROOT);

    rc = mmap_add_dynamic_region((unsigned long long)arg1,
                    arg1,
                    REGISTER_INFO_LENGTH,
                    MT_MEMORY | MT_RW | MT_ROOT);
    if (rc != 0) {
        panic();
    }
    memset((void*)s_phys, 0, REGISTER_INFO_LENGTH);
    reg_info =  (sc_reg_info_t *) s_phys;
    cpu_context_t *temp_target_ctx;
    temp_target_ctx  = cm_get_context(SECURE);
    populate_general_registers(reg_info, temp_target_ctx);
    populate_sys_registers(reg_info, temp_target_ctx);
    populate_el3_registers(reg_info);
    
    //encryption
    crypto_mem_buffer.virt_base = (void*)s_phys;
    crypto_mem_buffer.size = REGISTER_INFO_LENGTH;
    crypto_mem_buffer.flag = 0x18;
    crypto_mem_buffer.hash = 0;
    protect_str_buffer(&crypto_mem_buffer);

    rc = mmap_remove_dynamic_region(arg1, REGISTER_INFO_LENGTH);
    if (rc != 0) {
        panic();
    }
    gpt_trans_multi_pages(0, s_phys, REGISTER_INFO_LENGTH, GPT_GPI_NS);
    NOTICE("sc_register_info exit\n");
    spin_unlock(&gpt_lock2);
    return 0;
}

u_register_t dump_mem_reg_in_trap(debug_task_t *task)
{
   
    u_register_t x0;
    legal_flag = 1;
    sc_register_info(task->reg_buf, 0, 0);

    legal_flag = 1;
    if(task->pa_access == 0)
    {
        x0 = sc_introspection(task->mem_addr, task->mem_size, task->mem_el, 0x3378);
    }
    else if (task->pa_access)
    {
        x0 = sc_pa_introspection(task->mem_addr, task->mem_size, 0, 0, 0);
    }
    memset(task, 0, sizeof(debug_task_t));
    return x0;
}

size_t search_empty_task(debug_task_t* tasks){
	int i;
	for(i = 0; i < TASK_MAX; i++){
		if(!tasks[i].inited){
			return i;
		}
	}
	return 0xff;
}

size_t search_debug_task(debug_task_t* tasks, uint64_t target_addr, int debug_type)
{
	if(tasks == NULL)
        return 0xff;
    int i;
	
    for(i = 0; i < TASK_MAX; i++)
    {
        if(tasks[i].inited && tasks[i].addr == target_addr && tasks[i].type == debug_type)
        {
            return i;
        }
    }
	
	return 0xff;
}

static inline uint64_t armv8pmu_pmcr_read(void)
{
    uint64_t val = 0;
    asm volatile("mrs %0, pmcr_el0"
                 : "=r"(val));
    return val;
}
static inline void armv8pmu_pmcr_write(uint64_t val)
{
    val &= ARMV8_PMCR_MASK;
    isb();
    asm volatile("msr pmcr_el0, %0"
                 :
                 : "r"((uint64_t)val));
}

void end_of_interrupt(unsigned int id)
{
    dsbishst();
    write_icc_eoir1_el1(id);
    dsbishst();
    write_icc_eoir0_el1(id);
}

void protect_trap_hardware_config()
{
    //MDCR_EL3.TPM, bit [6] trap PMU related registers to EL3 
    u_register_t mdcr_el3 = read_mdcr_el3();
    src_mdcr_el3 = mdcr_el3;
    write_mdcr_el3(mdcr_el3 | 0x40);
    gpt_trans_multi_pages(0, (uint64_t)FVP_PMU, 0x10000, GPT_GPI_ROOT);
    gpt_trans_multi_pages(0, (uint64_t)FVP_PMU_2, 0x10000, GPT_GPI_ROOT);
    gpt_trans_multi_pages(0, (uint64_t)FVP_PMU_3, 0x10000, GPT_GPI_ROOT);
    gpt_trans_multi_pages(0, (uint64_t)FVP_PMU_4, 0x10000, GPT_GPI_ROOT);
    gpt_trans_multi_pages(0, (uint64_t)GICD_BASE, (GICD_END-GICD_BASE), GPT_GPI_ROOT);
}

void open_trap_hardware_config()
{
    u_register_t mdcr_el3 = read_mdcr_el3();
    write_mdcr_el3(mdcr_el3 & ~(0x40));
    gpt_trans_multi_pages(0, (uint64_t)FVP_PMU, 0x10000, GPT_GPI_ANY);
    gpt_trans_multi_pages(0, (uint64_t)FVP_PMU_2, 0x10000, GPT_GPI_ANY);
    gpt_trans_multi_pages(0, (uint64_t)FVP_PMU_3, 0x10000, GPT_GPI_ANY);
    gpt_trans_multi_pages(0, (uint64_t)FVP_PMU_4, 0x10000, GPT_GPI_ANY);
    gpt_trans_multi_pages(0, (uint64_t)GICD_BASE, (GICD_END-GICD_BASE), GPT_GPI_ANY);
}

void disable_PMU()
{
	uint64_t reg;
    asm volatile("msr pmcntenset_el0, %0"
                 :
                 : "r"(0x0));
    armv8pmu_pmcr_write(armv8pmu_pmcr_read() & ~ARMV8_PMCR_E);
	asm volatile("msr pmevtyper0_el0, %0" : : "r"((uint64_t)(0x0)));
    asm volatile("msr pmintenset_el1, %0" : : "r"((uint64_t)(0x0)));

    asm volatile("msr pmuserenr_el0, %0"
                 :
                 : "r"((uint64_t)0));
	cpu_context_t* sec_ctx = cm_get_context(SECURE);
	el3_state_t *state = get_el3state_ctx(sec_ctx);
	write_ctx_reg(state, CTX_ENABLE_PMI, 0x44);

    write_hcr_el2(src_hrc_el2);

    reg = read_mdcr_el3();
    reg &= ~(MDCR_SPME_BIT);
    write_mdcr_el3(reg);

    reg = read_mdcr_el2();
    reg |= MDCR_EL2_HPMD;
    reg &= ~(MDCR_EL2_HPME_BIT); 
    write_mdcr_el2(reg);

    open_trap_hardware_config();

}

void enable_PMU(int debug_type)
{
	NOTICE("[SC]: enable pmu\n");
	uint64_t reg;
    asm volatile("msr pmuserenr_el0, %0"
                 :
                 : "r"((uint64_t)ARMV8_PMUSERENR_EN_EL0 | ARMV8_PMUSERENR_ER | ARMV8_PMUSERENR_CR));

    armv8pmu_pmcr_write(ARMV8_PMCR_P | ARMV8_PMCR_C);


    asm volatile("msr pmintenset_el1, %0"
                 :
                 : "r"((uint64_t)(0x1)));
   


    armv8pmu_pmcr_write(armv8pmu_pmcr_read() | ARMV8_PMCR_E | ARMV8_PMCR_LC);
	reg = PMU_EVENT_SECURE;
	asm volatile("msr pmevtyper0_el0, %0" : : "r" (reg));

    reg = 0xFFFFFFFFFFFFFFFF;
    asm volatile("msr pmevcntr0_el0, %0" : : "r" (reg));

	cpu_context_t* sec_ctx = cm_get_context(SECURE);
	el3_state_t *state = get_el3state_ctx(sec_ctx);
	write_ctx_reg(state, CTX_ENABLE_PMI, 0x12);

    plat_ic_set_interrupt_type(FVP_PMI, INTR_TYPE_EL3);
    plat_ic_set_interrupt_type(FVP_PMI_2, INTR_TYPE_EL3);
    plat_ic_set_interrupt_type(FVP_PMI_3, INTR_TYPE_EL3);
    plat_ic_set_interrupt_type(FVP_PMI_4, INTR_TYPE_EL3);

    plat_ic_set_interrupt_priority(FVP_PMI, GIC_HIGHEST_SEC_PRIORITY);
    plat_ic_set_interrupt_priority(FVP_PMI_2, GIC_HIGHEST_SEC_PRIORITY);
    plat_ic_set_interrupt_priority(FVP_PMI_3, GIC_HIGHEST_SEC_PRIORITY);
    plat_ic_set_interrupt_priority(FVP_PMI_4, GIC_HIGHEST_SEC_PRIORITY);

    interrupt_affinity(FVP_PMI);
    interrupt_affinity(FVP_PMI_2);
    interrupt_affinity(FVP_PMI_3);
    interrupt_affinity(FVP_PMI_4);
   
    u_register_t hrc_el2 = read_hcr_el2();
    src_hrc_el2 = hrc_el2;
    write_hcr_el2(hrc_el2 & (~0x80018));

    protect_trap_hardware_config();


    debug_type_for_pmi = debug_type;
}

void scrutinizer_interrupt_handler()
{
    NOTICE("[SC]: interrupt handler\n");
    uint32_t intr_raw;
    unsigned int intr;
    int i = 0;
    
    uint64_t reg;
    el3_state_t *state;
    cpu_context_t *src_ctx;
    src_ctx  = cm_get_context(SECURE);
    state = get_el3state_ctx(src_ctx);
    reg = read_ctx_reg(state, CTX_SCR_EL3);
    reg &= ~(SCR_FIQ_BIT);
    write_ctx_reg(state, CTX_SCR_EL3, reg);

	uint64_t current_pmi_ins_addr = read_elr_el3();
    if(pmi_ins_addr == current_pmi_ins_addr)
        flood_flag++;
    else
        pmi_ins_addr = current_pmi_ins_addr;
    
    intr_raw = plat_ic_acknowledge_interrupt();
    intr = plat_ic_get_interrupt_id(intr_raw);
    NOTICE("interrupt address is 0x%lx, ID %d\n", pmi_ins_addr, intr);

  	if (intr == FVP_PMI || intr == FVP_PMI_2 || intr == FVP_PMI_3 || intr == FVP_PMI_4) 
    {
        NOTICE("[SC]: It's PMI.\n");
        disable_PMU();
        plat_ic_set_interrupt_type(intr, INTR_TYPE_NS);
        plat_ic_end_of_interrupt(intr);

        if(debug_type_for_pmi == 1)
        {
            gpt_trans_multi_pages(0, wat_addr_for_pmi, 0x1000, GPT_GPI_NO_ACCESS);
            debug_type_for_pmi = 0;
            flood_flag = 0;
        }
        else if (debug_type_for_pmi == 2)
        {
            if(flood_flag >= PMI_FLOOD_NUM)
            {
                uint64_t aligned_brk_addr_for_pmi;
                aligned_brk_addr_for_pmi = (brk_addr_for_pmi & (~(0xFFF)));
                for(i = 0; i < TASK_MAX; i++)
                {
                    if(wat_tasks[i].inited && wat_tasks[i].aligned_addr == aligned_brk_addr_for_pmi && wat_tasks[i].type == 2)
                    {  
                        NOTICE("flood ins addr: 0x%lx handle the brk: 0x%lx\n", pmi_ins_addr, wat_tasks[i].addr);
                        dump_mem_reg_in_trap(&(wat_tasks[i]));
                    }
                }
                debug_type_for_pmi = 0;
                flood_flag = 0;
                brk_addr_for_pmi = 0;
                return;
            }

            int index = search_debug_task(wat_tasks, pmi_ins_addr, 2);
            if (index == 0xff)
            {
                uint64_t aligned_pmi_ins_addr;
                aligned_pmi_ins_addr = (pmi_ins_addr & (~(0xFFF)));
                for(i = 0; i < TASK_MAX; i++)
                {
                    if(wat_tasks[i].inited && wat_tasks[i].aligned_addr == aligned_pmi_ins_addr && wat_tasks[i].type == 2)
                    {
                        enable_PMU(2);
                        return; 
                    }
                }
                NOTICE("outside the brk gpt page, use GPF instaed of PMI\n");
                gpt_trans_multi_pages(0, brk_addr_for_pmi, 0x1000, GPT_GPI_NO_ACCESS);
            }
            else
            {
                NOTICE("Hit brk:0x%lx via pmi\n", pmi_ins_addr);
                dump_mem_reg_in_trap(&(wat_tasks[index]));
                debug_type_for_pmi = 0;
                flood_flag = 0;
            }      
        
        }
        else if (debug_type_for_pmi == 3)
        {
            uint64_t pa_pmi_ins_addr = get_phys_from_tz_virt(pmi_ins_addr, 1);
            if(stepping_addr_for_pmi == pa_pmi_ins_addr)
                NOTICE("Hit brk via stepping:0x%lx\n", pmi_ins_addr);
            else
                enable_PMU(3);
        }
        return;
    }
    else
    {
        if(flood_flag >= PMI_FLOOD_NUM)
        {
            if(debug_type_for_pmi == 1)
            {
                NOTICE("flood ins addr: 0x%lx handle the wrk: 0x%lx\n", pmi_ins_addr, wat_addr_for_pmi);
                disable_PMU();
                int index = search_debug_task(wat_tasks, wat_addr_for_pmi, 1); 
                dump_mem_reg_in_trap(&(wat_tasks[index]));
                debug_type_for_pmi = 0;
                flood_flag = 0;
                wat_addr_for_pmi = 0;
            }
            else if (debug_type_for_pmi == 2)
            {
                disable_PMU();
                uint64_t aligned_brk_addr_for_pmi;
                aligned_brk_addr_for_pmi = (brk_addr_for_pmi & (~(0xFFF)));
                for(i = 0; i < TASK_MAX; i++)
                {
                    if(wat_tasks[i].inited && wat_tasks[i].aligned_addr == aligned_brk_addr_for_pmi && wat_tasks[i].type == 2)
                    {  
                        NOTICE("flood ins addr: 0x%lx handle the brk: 0x%lx\n", pmi_ins_addr, wat_tasks[i].addr);
                        dump_mem_reg_in_trap(&(wat_tasks[i]));
                    }
                }
                debug_type_for_pmi = 0;
                flood_flag = 0;
                brk_addr_for_pmi = 0;
            }
        }
    }
    plat_ic_end_of_interrupt(intr);
    return;
}

int data_access_trap(uint64_t esr_el3, cpu_context_t *ctx, int op)
{
    uint64_t fault_addr = read_far_el3();
    uint64_t ins_addr = read_elr_el3();
    uint64_t esr = read_esr_el3();
    NOTICE("[SC_DATA_GPF]: fault_addr:0x%lx\n", fault_addr);
    NOTICE("[SC_DATA_GPF]: ins addr is 0x%lx\n", ins_addr);
    int i = 0;
    uint64_t aligned_data_addr;
    aligned_data_addr = (fault_addr & (~(0xFFF)));

    if(fault_addr >= GICD_BASE && fault_addr <= GICD_END)
    {
        if(fault_addr == FVP_PMU_GROUP0 || fault_addr == FVP_PMU_PRIORITY || fault_addr == FVP_PMU_AFFINITY ||
                fault_addr == FVP_PMU_2_GROUP0 || fault_addr==FVP_PMU_2_PRIORITY || fault_addr==FVP_PMU_2_AFFINITY ||    
                    fault_addr ==FVP_PMU_3_GROUP0 || fault_addr==FVP_PMU_3_PRIORITY || fault_addr==FVP_PMU_3_AFFINITY ||  
                         fault_addr ==FVP_PMU_4_GROUP0 || fault_addr==FVP_PMU_4_PRIORITY || fault_addr==FVP_PMU_4_AFFINITY)
        {}
        else
        {
            if(op == 0)
            {
                u_register_t reg;
                uint64_t* de = (uint64_t*)fault_addr;
                reg = *de;
                write_ctx_reg(get_gpregs_ctx(ctx),CTX_GPREG_X0, reg);
            }
            else if(op == 1)
            {
                u_register_t reg = read_ctx_reg(get_gpregs_ctx(ctx),CTX_GPREG_X1);
                uint64_t* de = (uint64_t*)fault_addr;
                *de = reg;
            }
        }
        ins_addr+= GET_NEXT_PC_INC(esr);  
        write_elr_el3(ins_addr);
        return TRAP_RET_REPEAT;
    }
    else if ((fault_addr >= FVP_PMU && fault_addr <= FVP_PMU + 0x10000) || 
                (fault_addr >= FVP_PMU_2 && fault_addr <= FVP_PMU_2 + 0x10000) ||
                    (fault_addr >= FVP_PMU_3 && fault_addr <= FVP_PMU_3 + 0x10000) ||
                         (fault_addr >= FVP_PMU_4 && fault_addr <= FVP_PMU_4 + 0x10000))
    {
        ins_addr+= GET_NEXT_PC_INC(esr);  
        write_elr_el3(ins_addr);
        return TRAP_RET_REPEAT;
    }

    int index = search_debug_task(wat_tasks, fault_addr, 1);
    if (index == 0xff)
    {
        for(i = 0; i < TASK_MAX; i++)
        {
            if(wat_tasks[i].inited && wat_tasks[i].aligned_addr == aligned_data_addr && wat_tasks[i].type == 1)
            {
                NOTICE("no wrk gpf hit, enable pmi for continue and reset\n"); 
                enable_PMU(1);
                wat_addr_for_pmi = wat_tasks[i].addr;
                gpt_trans_multi_pages(0, fault_addr, 0x1000, GPT_GPI_SECURE); 
                return TRAP_RET_REPEAT; 
            }
        }
    }
    else
    {
        if(op == 0)
        {
            NOTICE("Addr: 0x%lx READ hit the wrk via GPF:0x%lx\n", ins_addr, fault_addr);
            gpt_trans_multi_pages(0, fault_addr, 0x1000, GPT_GPI_SECURE);  
            dump_mem_reg_in_trap(&(wat_tasks[index]));
        }
        else if(op == 1)
        {
            NOTICE("Addr: 0x%lx WRITE hit the wrk via GPF:0x%lx\n", ins_addr, fault_addr);
            gpt_trans_multi_pages(0, fault_addr, 0x1000, GPT_GPI_SECURE);  
            dump_mem_reg_in_trap(&(wat_tasks[index]));
        }
        
        return TRAP_RET_REPEAT;  
    }
    
    return TRAP_RET_UNHANDLED;
}

int instruction_access_trap(uint64_t esr_el3, cpu_context_t *ctx)
{
    uint64_t ins_addr = read_elr_el3();
    NOTICE("[SC_INS_GPF]:0x%lx\n", ins_addr);
    if (debug_type_for_pmi == 2)
    {
        int index = search_debug_task(wat_tasks, ins_addr, 2);
        if (index == 0xff)
        {
            enable_PMU(2);
            brk_addr_for_pmi = ins_addr;
            gpt_trans_multi_pages(0, ins_addr, 0x1000, GPT_GPI_SECURE);
            return TRAP_RET_REPEAT;
        }
        else
        {
            NOTICE("Hit the breakpoint via GPF:0x%lx\n", ins_addr);
            gpt_trans_multi_pages(0, ins_addr, 0x1000, GPT_GPI_SECURE);
            dump_mem_reg_in_trap(&(wat_tasks[index]));
            return TRAP_RET_REPEAT;  
        }
    }

    NOTICE("UNHANDLED GPF\n");
    return TRAP_RET_UNHANDLED;
}

int handle_gpf_trap(uint64_t esr_el3, cpu_context_t *ctx)
{   
    NOTICE("SC GPF handler\n");

	switch (esr_el3 & ISS_MASK) {
        case DATA_ACCESS_READ_FAULT:
            return data_access_trap(esr_el3, ctx, 0);
        case DATA_ACCESS_WRITE_FAULT:
            return data_access_trap(esr_el3, ctx, 1);
        case INSTRUCTION_ACCESS__FAULT:
            return instruction_access_trap(esr_el3, ctx);
	default:
		return TRAP_RET_UNHANDLED;
	}
}

u_register_t sc_watchpoint(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4, u_register_t arg5, u_register_t arg6)
{	
    if(legal_flag ==0)
    {
       NOTICE("invalid user\n");
        return 0 ;
    }
    legal_flag = 0;
    spin_lock(&gpt_lock2);
    uint64_t pa_addr = arg4;
	size_t task_id = search_empty_task(wat_tasks);
	wat_tasks[task_id].type = 1;
	wat_tasks[task_id].addr = pa_addr;
	wat_tasks[task_id].inited = true;
	wat_tasks[task_id].aligned_addr = (pa_addr & (~(0xFFF)));
    wat_tasks[task_id].mem_addr = arg1;
    wat_tasks[task_id].mem_size = arg2;
    wat_tasks[task_id].mem_el = arg3;
    wat_tasks[task_id].reg_buf = arg5;
    wat_tasks[task_id].pa_access = arg6;
	gpt_trans_multi_pages(0, pa_addr, 0x1000, GPT_GPI_NO_ACCESS);
	spin_unlock(&gpt_lock2);
	NOTICE("set sc_watchpoint 0x%lx, task_id %lu\n", pa_addr, task_id);
	return 0;
}

u_register_t sc_breakpoint(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4, u_register_t arg5, u_register_t arg6)
{	
    if(legal_flag ==0)
    {
       NOTICE("invalid user\n");
        return 0 ;
    }
    legal_flag = 0;
    spin_lock(&gpt_lock2);
    uint64_t pa_addr = arg4;
	size_t task_id = search_empty_task(wat_tasks);
	wat_tasks[task_id].type = 2;
	wat_tasks[task_id].addr = pa_addr;
	wat_tasks[task_id].inited = true;
	wat_tasks[task_id].aligned_addr = (pa_addr & (~(0xFFF)));
    wat_tasks[task_id].mem_addr = arg1;
    wat_tasks[task_id].mem_size = arg2;
    wat_tasks[task_id].mem_el = arg3;
    wat_tasks[task_id].reg_buf = arg5;
    wat_tasks[task_id].pa_access = arg6;
	gpt_trans_multi_pages(0, pa_addr, 0x1000, GPT_GPI_NO_ACCESS);
	spin_unlock(&gpt_lock2);
    NOTICE("set sc_breakpoint 0x%lx, task_id %lu\n", pa_addr, task_id);
    debug_type_for_pmi = 2;
	return 0;
}



static inline void tsb_csync(void)
{
	/*
	 * The assembler does not yet understand the tsb csync mnemonic
	 * so use the equivalent hint instruction.
	 */
	__asm__ volatile("hint #18");
}

u_register_t sc_ete_trace(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4)
{	

    if(legal_flag ==0)
    {
       NOTICE("invalid user\n");
        return 0 ;
    }
    legal_flag = 0;
    uint64_t reg;
    ins_share_buf = arg4;
    
    // Set up TRBE
    reg = read_mdcr_el3();
    reg &= 0xFFFFFFFFF8FFFFFF;  // NSTB=0b0, NSTBE == 0b0, Trace buffer is secure. Accesses to Trace Buffer control registers at EL2 and EL1 generate Trap exceptions to EL3.
    reg |= MDCR_STE_BIT;    // STE=1       Secure Trace enable
    write_mdcr_el3(reg);

    reg = read_mdcr_el2();
    reg &= 0xFFFFFFFFFCFFFFFF; // E2TB=0b00     Trace Buffer owning Exception level is EL2
    write_mdcr_el2(reg);

    reg = TRBE_BASE_ADDRESS;
    asm volatile("msr TRBBASER_EL1, %0" : : "r" (reg));

    reg = TRBE_LIMIT_ADDRESS; // Trace Buffer Limit pointer address
    reg |= (3 << 3); // TM=0b11       Ignore trigger
    reg |= (3 << 1); // FM=0b11       Circular Buffer mode
    //reg |= (1 << 5);                 //nVM = 0, The trace buffer pointers are virtual addresses; nVM =1, physical address
    asm volatile("msr TRBLIMITR_EL1, %0" : : "r" (reg));

    reg = TRBE_BASE_ADDRESS;
    asm volatile("msr TRBPTR_EL1, %0" : : "r" (reg)); // Trace Buffer current write pointer virtual address

    // Set up ETE
    reg = 0;
    asm volatile("msr OSLAR_EL1, %0" : : "r" (reg)); // OSLK=0        Clear the OS lock

    reg = read_cptr_el3();
    reg &= ~(1<<20);
    write_cptr_el3(reg);

    reg |= (1 << 12); // RS=1          Return stack enabled
    reg |= (1 << 7); // VMID=1        Virtual context identifier tracing enabled
    reg |= (1 << 6);  // CID=1         Context identifier tracing enabled
    reg |= 0x1;
    asm volatile("msr TRCCONFIGR, %0" : : "r" (reg));

    reg = 0xC;
    asm volatile("msr TRCSYNCPR, %0" : : "r" (reg)); // PERIOD=0xC    Enable trace protocol synchronization every 4096 bytes of trace

    reg = 0x2;
    asm volatile("msr TRCTRACEIDR, %0" : : "r" (reg)); // TRACEID=0x2   Set trace ID to 0x2

    reg = 0x0;
    reg |= 0x7780201; // Trace unit generate instruction trace for SEL0-SEL2
    asm volatile("msr TRCVICTLR, %0" : : "r" (reg)); 

    reg = 0x16;
    asm volatile("msr TRCCCCTLR, %0" : : "r" (reg)); // THRESHOLD=0x16    Sets the threshold value for instruction trace cycle counting to 0x16

    //ETE range configs
    reg = 0x0;
    asm volatile("msr TRCEVENTCTL0R, %0" : : "r" (reg));  // Disable all event tracing
    asm volatile("msr TRCEVENTCTL1R, %0" : : "r" (reg));  // Disable all event tracing
    asm volatile("msr TRCSTALLCTLR, %0" : : "r" (reg));  // Disable stalling, if implemented
    asm volatile("msr TRCTSCTLR, %0" : : "r" (reg));     // Disable the timestamp event
    asm volatile("msr TRCVIIECTLR, %0" : : "r" (reg)); // No address range filtering for logic started
    asm volatile("msr TRCVISSCTLR, %0" : : "r" (reg));  // No start or stop points for ViewInst
    asm volatile("msr TRCBBCTLR, %0" : : "r" (reg));   // Deactivate branch broadcasting for all address ranges

    // Program only if TRCIDR4.NUMPC, number of PE Comparator Inputs that are available for tracing, > 0
    //asm volatile("msr TRCVIPCSSCTLR, %0" : : "r" (reg));  // No PE Comparator Input m is not selected as a trace stop resource.
    
    reg = 0x0;
    asm volatile("msr TRCRSR, %0" : : "r" (reg));  // Set the trace resource status to 0

    reg = read_trfcr_el1();
    reg |= 0x3;
    write_trfcr_el1(reg);  // E0TRE=1, E1TRE=1    EL0 and EL1 tracing enabled
    reg = read_trfcr_el2();
    reg |= 0x3;
    write_trfcr_el2(reg);    // E0HTRE=1, E2TRE=1   EL0 and EL2 tracing enabled
    isb();

    if(TRBE_buffer_flag == 0)
    {
        fast_memset((void *)TRBE_BASE_ADDRESS, 0, TRBE_SIZE);
        gpt_trans_multi_pages(0, (uint64_t)TRBE_BASE_ADDRESS, TRBE_SIZE, GPT_GPI_SECURE);
        TRBE_buffer_flag = 1;
    }

	cpu_context_t* sec_ctx = cm_get_context(SECURE);
	el3_state_t *state = get_el3state_ctx(sec_ctx);
	write_ctx_reg(state, CTX_ENABLE_TRACE, 0x60);
    reg = read_ctx_reg(state, CTX_CPTR_EL3);
    reg &= ~(1<<20);
    write_ctx_reg(state, CTX_CPTR_EL3, reg);
	NOTICE("ete_on\n");
	return 0;

}


u_register_t sc_disable_ete_trace(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{	
    if(legal_flag ==0)
    {
       NOTICE("invalid user\n");
        return 0 ;
    }
    legal_flag = 0;
    uint64_t reg;

    reg = read_trfcr_el1();// Save the current programming of TRFCR_EL1.
    reg &= ~(0x3);
    write_trfcr_el1(reg);                     // Clear the values of TRFCR_EL1.ExTRE. to put the PE in to a prohibited region
    isb();  // Synchronize the entry to the prohibited region

   __asm__ volatile("hint #18");  // Ensure that all trace has reached the trace buffer and address translations have taken place
    dsbnsh();

    reg = 0x0;
    asm volatile("msr TRCPRGCTLR, %0" : : "r" (reg));  // EN=0          Disable ETE

    // Wait for TRCSTATR.IDLE==1 and TRCSTATR.PMSTABLE==1
    isb();
   
    do
    {
        asm volatile("mrs %0, TRCSTATR": "=r"(reg));
    } while (!(reg&0x1) || !(reg&0x2));
    
    
	cpu_context_t* sec_ctx = cm_get_context(SECURE);
	el3_state_t *state = get_el3state_ctx(sec_ctx);
	write_ctx_reg(state, CTX_ENABLE_TRACE, 0x0);

    if(TRBE_buffer_flag == 1)
    {
        gpt_trans_multi_pages(0, (uint64_t)TRBE_BASE_ADDRESS, TRBE_SIZE, GPT_GPI_ROOT);
        //encryption
        crypto_mem_buffer.virt_base = (void*)TRBE_BASE_ADDRESS;
        crypto_mem_buffer.size = TRBE_SIZE;
        crypto_mem_buffer.flag = 0x18;
        crypto_mem_buffer.hash = 0;
        protect_str_buffer(&crypto_mem_buffer);

        //copy to share buf
        if(ins_share_buf)
        {
            mmap_add_dynamic_region(ins_share_buf,
				     ins_share_buf,
				     TRBE_SIZE,
				     MT_MEMORY | MT_RW | MT_NS);
            fast_memset((void*)ins_share_buf, 0, TRBE_SIZE);
            fast_memcpy((void*)ins_share_buf, (void*)TRBE_BASE_ADDRESS, TRBE_SIZE);
            mmap_remove_dynamic_region(ins_share_buf, TRBE_SIZE);
        }
        TRBE_buffer_flag = 0;
    }

    reg = read_ctx_reg(state, CTX_CPTR_EL3);
    reg &= ~(1<<20);
    write_ctx_reg(state, CTX_CPTR_EL3, reg);
    
    NOTICE("ete_off\n");
	return 0;

}

u_register_t sc_stepping_model(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{	
    if(legal_flag ==0)
    {
       NOTICE("invalid user\n");
        return 0 ;
    }
    spin_lock(&gpt_lock2);
	uint64_t pa_addr = get_phys_from_tz_virt(arg1, arg2);
    stepping_addr_for_pmi = pa_addr;
    debug_type_for_pmi = 3;
    enable_PMU(3);
	spin_unlock(&gpt_lock2);
	NOTICE("test stepping brk in VA: 0x%lx, PA: 0x%lx\n", arg1, pa_addr);
	return 0;
}

u_register_t sc_disable_stepping_model(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{	
    if(legal_flag ==0)
    {
       NOTICE("invalid user\n");
        return 0 ;
    }
    spin_lock(&gpt_lock2);
    NOTICE("stop stepping\n");
    disable_PMU();
	spin_unlock(&gpt_lock2);
	return 0;
}