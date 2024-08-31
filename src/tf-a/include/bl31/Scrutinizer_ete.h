#ifndef SCRUTINIZER_ETE_H
#define SCRUTINIZER_ETE_H

/*******************************************************************************
 * register definitions
 ******************************************************************************/
//size 0xF8000000 : 0xF8000000 +0x2000000 = 0xFA000000.
#define TRBE_BASE_ADDRESS 0xF8000000
#define TRBE_LIMIT_ADDRESS 0xFA000000
#define TRBE_SIZE 0x2000000

#define TRFCR_EL1 S3_0_C1_C2_1
#define TRFCR_EL2 S3_4_C1_C2_1

DEFINE_RENAME_SYSREG_RW_FUNCS(trfcr_el1, TRFCR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(trfcr_el2, TRFCR_EL2)

#endif /* SCRUTINIZER_ETE_H */