/*
 * Copyright (c) 2018-2022, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/cpus/errata_report.h>
#include <lib/cpus/wa_cve_2017_5715.h>
#include <lib/cpus/wa_cve_2018_3639.h>
#include <lib/cpus/wa_cve_2022_23960.h>
#include <lib/smccc.h>
#include <services/arm_arch_svc.h>
#include <smccc_helpers.h>
#include <plat/common/platform.h>
#include <lib/gpt_rme/gpt_rme.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <bl31/Scrutinizer.h>

unsigned int plat_is_my_cpu_primary(void);

static int32_t smccc_version(void)
{
	return MAKE_SMCCC_VERSION(SMCCC_MAJOR_VERSION, SMCCC_MINOR_VERSION);
}

static int32_t smccc_arch_features(u_register_t arg1)
{
	switch (arg1) {
	case SMCCC_VERSION:
	case SMCCC_ARCH_FEATURES:
		return SMC_ARCH_CALL_SUCCESS;
	case SMCCC_ARCH_SOC_ID:
		return plat_is_smccc_feature_available(arg1);
#if WORKAROUND_CVE_2017_5715
	case SMCCC_ARCH_WORKAROUND_1:
		if (check_wa_cve_2017_5715() == ERRATA_NOT_APPLIES)
			return 1;
		return 0; /* ERRATA_APPLIES || ERRATA_MISSING */
#endif

#if WORKAROUND_CVE_2018_3639
	case SMCCC_ARCH_WORKAROUND_2: {
#if DYNAMIC_WORKAROUND_CVE_2018_3639
		unsigned long long ssbs;

		/*
		 * Firmware doesn't have to carry out dynamic workaround if the
		 * PE implements architectural Speculation Store Bypass Safe
		 * (SSBS) feature.
		 */
		ssbs = (read_id_aa64pfr1_el1() >> ID_AA64PFR1_EL1_SSBS_SHIFT) &
			ID_AA64PFR1_EL1_SSBS_MASK;

		/*
		 * If architectural SSBS is available on this PE, no firmware
		 * mitigation via SMCCC_ARCH_WORKAROUND_2 is required.
		 */
		if (ssbs != SSBS_UNAVAILABLE)
			return 1;

		/*
		 * On a platform where at least one CPU requires
		 * dynamic mitigation but others are either unaffected
		 * or permanently mitigated, report the latter as not
		 * needing dynamic mitigation.
		 */
		if (wa_cve_2018_3639_get_disable_ptr() == NULL)
			return 1;
		/*
		 * If we get here, this CPU requires dynamic mitigation
		 * so report it as such.
		 */
		return 0;
#else
		/* Either the CPUs are unaffected or permanently mitigated */
		return SMC_ARCH_CALL_NOT_REQUIRED;
#endif
	}
#endif

#if (WORKAROUND_CVE_2022_23960 || WORKAROUND_CVE_2017_5715)
	case SMCCC_ARCH_WORKAROUND_3:
		/*
		 * SMCCC_ARCH_WORKAROUND_3 should also take into account
		 * CVE-2017-5715 since this SMC can be used instead of
		 * SMCCC_ARCH_WORKAROUND_1.
		 */
		if ((check_smccc_arch_wa3_applies() == ERRATA_NOT_APPLIES) &&
		    (check_wa_cve_2017_5715() == ERRATA_NOT_APPLIES)) {
			return 1;
		}
		return 0; /* ERRATA_APPLIES || ERRATA_MISSING */
#endif

	/* Fallthrough */

	default:
		return SMC_UNK;
	}
}

/* return soc revision or soc version on success otherwise
 * return invalid parameter */
static int32_t smccc_arch_id(u_register_t arg1)
{
	if (arg1 == SMCCC_GET_SOC_REVISION) {
		return plat_get_soc_revision();
	}
	if (arg1 == SMCCC_GET_SOC_VERSION) {
		return plat_get_soc_version();
	}
	return SMC_ARCH_CALL_INVAL_PARAM;
}


static u_register_t sc_debug_point(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4, u_register_t arg5, u_register_t arg6)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_watchpoint(arg1, arg2, arg3, arg4, arg5, arg6);
}

static u_register_t sc_debug_point_2(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4, u_register_t arg5, u_register_t arg6)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_breakpoint(arg1, arg2, arg3, arg4, arg5, arg6);
}

static u_register_t sc_introspection_point(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t flags)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_introspection(arg1, arg2, arg3, flags);
}

static u_register_t sc_register_get(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_register_info(arg1, arg2, arg3);
}

static u_register_t sc_pa_introspection_point(u_register_t arg1, u_register_t arg2, u_register_t arg3, u_register_t arg4,  u_register_t arg5)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_pa_introspection(arg1, arg2, arg3, arg4, arg5);
}

static u_register_t sc_trace(u_register_t arg1, u_register_t arg2, u_register_t arg3,  u_register_t arg4)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_ete_trace(arg1, arg2, arg3, arg4);
}

static u_register_t sc_disable_trace(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_disable_ete_trace(arg1, arg2, arg3);
}

static u_register_t sc_stepping(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_stepping_model(arg1, arg2, arg3);
}

static u_register_t sc_disable_stepping(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_disable_stepping_model(arg1, arg2, arg3);
}

static u_register_t sc_auth(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_legal_auth(arg1, arg2, arg3);
}

static u_register_t sc_introspection_finish(u_register_t arg1, u_register_t arg2, u_register_t arg3)
{
	if (plat_is_my_cpu_primary() == 1U)
	{
		// return 0;
	}
	return sc_introspection_exit(arg1, arg2, arg3);
}




/*
 * Top-level Arm Architectural Service SMC handler.
 */
static uintptr_t arm_arch_svc_smc_handler(uint32_t smc_fid,
	u_register_t x1,
	u_register_t x2,
	u_register_t x3,
	u_register_t x4,
	void *cookie,
	void *handle,
	u_register_t flags)
{
	u_register_t x5;
	u_register_t x6;
	switch (smc_fid) {
	case SMCCC_VERSION:
		SMC_RET1(handle, smccc_version());
	case SMCCC_ARCH_FEATURES:
		SMC_RET1(handle, smccc_arch_features(x1));
	case SMCCC_ARCH_SOC_ID:
		SMC_RET1(handle, smccc_arch_id(x1));
	case SC_WATCHPOINT:
		x5 = SMC_GET_GP(handle, CTX_GPREG_X5);
		x6 = SMC_GET_GP(handle, CTX_GPREG_X6);
		SMC_RET1(handle, sc_debug_point(x1, x2, x3, x4, x5, x6));
	case SC_BREAKPOINT:
		x5 = SMC_GET_GP(handle, CTX_GPREG_X5);
		x6 = SMC_GET_GP(handle, CTX_GPREG_X6);
		SMC_RET1(handle, sc_debug_point_2(x1, x2, x3, x4, x5, x6));
	case SC_INTROSPECTION:
		SMC_RET1(handle, sc_introspection_point(x1, x2, x3,flags));
	case SC_REGISTER_INFO:
		SMC_RET1(handle, sc_register_get(x1, x2, x3));
	case SC_PA_INTROSPECTION:
		x5 = SMC_GET_GP(handle, CTX_GPREG_X5);
		SMC_RET1(handle, sc_pa_introspection_point(x1, x2, x3, x4, x5));
	case SC_ETE_TRACE:
		SMC_RET1(handle, sc_trace(x1, x2, x3, x4));
	case SC_DISABLE_TRACE:
		SMC_RET1(handle, sc_disable_trace(x1, x2, x3));
	case SC_STEPPING:
		SMC_RET1(handle, sc_stepping(x1, x2, x3));
	case SC_DISABLE_STEPPING:
		SMC_RET1(handle, sc_disable_stepping(x1, x2, x3));
	case SC_AUTH:
		SMC_RET1(handle, sc_auth(x1, x2, x3));
	case SC_INTROSPECTION_COMPLETE:
		SMC_RET1(handle, sc_introspection_finish(x1, x2, x3));		
#if WORKAROUND_CVE_2017_5715
	case SMCCC_ARCH_WORKAROUND_1:
		/*
		 * The workaround has already been applied on affected PEs
		 * during entry to EL3. On unaffected PEs, this function
		 * has no effect.
		 */
		SMC_RET0(handle);
#endif
#if WORKAROUND_CVE_2018_3639
	case SMCCC_ARCH_WORKAROUND_2:
		/*
		 * The workaround has already been applied on affected PEs
		 * requiring dynamic mitigation during entry to EL3.
		 * On unaffected or statically mitigated PEs, this function
		 * has no effect.
		 */
		SMC_RET0(handle);
#endif
#if (WORKAROUND_CVE_2022_23960 || WORKAROUND_CVE_2017_5715)
	case SMCCC_ARCH_WORKAROUND_3:
		/*
		 * The workaround has already been applied on affected PEs
		 * during entry to EL3. On unaffected PEs, this function
		 * has no effect.
		 */
		SMC_RET0(handle);
#endif
	default:
		WARN("Unimplemented Arm Architecture Service Call: 0x%x \n",
			smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(
		arm_arch_svc,
		OEN_ARM_START,
		OEN_ARM_END,
		SMC_TYPE_FAST,
		NULL,
		arm_arch_svc_smc_handler
);
