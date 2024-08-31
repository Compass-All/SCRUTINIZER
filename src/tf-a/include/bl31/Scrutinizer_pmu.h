#ifndef SCRUTINIZER_PMU_H
#define SCRUTINIZER_PMU_H
/*Provides details of the Performance Monitors implementation, including the
number of counters implemented, configures and controls the counters.*/
#define ARMV8_PMCR_MASK 0x3f
#define ARMV8_PMCR_E (1 << 0) /*  Enable all event and cycle counters */
#define ARMV8_PMCR_P (1 << 1) /*  Reset all event counters */
#define ARMV8_PMCR_C (1 << 2) /*  Reset cycle counter  */
#define ARMV8_PMCR_D (1 << 3) /*  cycle counter counts once every 64th cpu cycle, 0 means When enabled, PMCCNTR_EL0 counts every clock cycle. */
#define ARMV8_PMCR_X (1 << 4) /*  Export to ETM: Enable export of events in an IMPLEMENTATION DEFINED PMU event export bus.*/
#define ARMV8_PMCR_DP (1 << 5) /*  Disable CCNT if non-invasive debug: Disable cycle counter when event counting is prohibited.*/
#define ARMV8_PMCR_LC (1 << 6) /*Long cycle counter enable, Arm deprecates use of PMCR_EL0.LC = 0 .*/
#define ARMV8_PMCR_LP (1 << 7) /*Long event counter enable */
/*N, bits [15:11], This value is in the range of 0b00000-0b11111. If the value is 0b00000 then only PMCCNTR_EL0 is implemented. If the value is 0b11111 PMCCNTR_EL0 and 31 event counters are implemented*/
#define ARMV8_PMCR_N_SHIFT 11 /*  Number of counters supported */
#define ARMV8_PMCR_N_MASK 0x1f

/*Enables or disables User mode access to the Performance Monitors.*/
#define ARMV8_PMUSERENR_EN_EL0 (1 << 0) /*  EL0 access enable */
#define ARMV8_PMUSERENR_CR (1 << 2) /*  Cycle counter read enable */
#define ARMV8_PMUSERENR_ER (1 << 3) /*  Event counter read enable */

/* Enables the Cycle Count Register, PMCCNTR_EL0, and any implemented event counters PMEVCNTR<n>. Reading this register shows which counters are enabled.*/
#define ARMV8_PMCNTENSET_EL0_ENABLE (1 << 31) /* PMCCNTR_EL0 enable bit. Enables the cycle counter register. */


#define ARMV8_PMEVTYPER_P              (1 << 31) /* EL1 modes filtering bit */
#define ARMV8_PMEVTYPER_U              (1 << 30) /* EL0 filtering bit */
#define ARMV8_PMEVTYPER_NSK            (1 << 29) /* Non-secure EL1 (kernel) modes filtering bit */
#define ARMV8_PMEVTYPER_NSU            (1 << 28) /* Non-secure User mode filtering bit */
#define ARMV8_PMEVTYPER_NSH            (1 << 27) /* Non-secure Hyp modes filtering bit */
#define ARMV8_PMEVTYPER_M              (1 << 26) /* Secure EL3 filtering bit */
#define ARMV8_PMEVTYPER_MT             (1 << 25) /* Multithreading */
#define ARMV8_PMEVTYPER_SH              (1 << 24) /* Secure EL2 filtering bit */ 
#define ARMV8_PMEVTYPER_RLU             (1 << 21) /* Realm EL0 (unprivileged) filtering bit */
#define ARMV8_PMEVTYPER_RLH              (1 << 20) /* Realm EL2 filtering bit */ 
#define ARMV8_PMEVTYPER_EVTCOUNT_MASK  0x3ff

#define PMU_EVENT_INST_RETIRED 0x08
#define PMU_EVENT_CPU_CYCLE 0x11
#define PMU_EVENT_SECURE  (PMU_EVENT_CPU_CYCLE | ARMV8_PMEVTYPER_NSK | ARMV8_PMEVTYPER_NSU| ARMV8_PMEVTYPER_SH | ARMV8_PMEVTYPER_RLU | ARMV8_PMEVTYPER_M)


#endif /* SCRUTINIZER_PMU_H */