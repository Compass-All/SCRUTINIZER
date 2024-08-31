#include <asm_macros.S>


#ifdef __aarch64__
#if __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0)

    .arch armv8.7-a+tme
	.global	tme_test
    .global	fallback
func tme_test
    tstart x16
    cbnz x16, .fail
.loop:
    ldr x5, =0x88017000
    ldr x4, [x5]
    mov x4, #0x123
    str x4, [x5]
    b .loop
    tcommit
    //tcancel #0x1234
    ret
.fail:
    ldr x5, =0x88017000
    ldr x4, [x5]
    ret
endfunc tme_test

func fallback
    //check the gpt result:read again
    ldr x6, =0x88017000
    ldr x4, [x6]
	ret
endfunc fallback

#endif /* __GNUC__ > 8 || (__GNUC__ == 8 && __GNUC_MINOR__ > 0) */
#endif /* __aarch64__ */