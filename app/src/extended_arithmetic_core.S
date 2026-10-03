.syntax unified
.cpu cortex-m4
.thumb

/* Soft-float rounding, conversion, and extended arithmetic primitives. */
.section .text.softfloat_round_even,"ax",%progbits
.balign 4
.global round_even_core
.type round_even_core, %function
.thumb_func
round_even_core:
.Lround_27c3c:
    movs.w ip, r0, lsl #1
.Lround_27c40:
    movs.w ip, ip, lsr #24
.Lround_27c44:
    subs.w r3, ip, #127
.Lround_27c48:
    bmi.n .Lround_27c74
.Lround_27c4a:
    rsbs r2, r3, #23
.Lround_27c4e:
    ble.w .Lround_27c8a
.Lround_27c52:
    mov.w ip, #1
.Lround_27c56:
    lsl.w ip, ip, r2
.Lround_27c5a:
    sub.w r3, ip, #1
.Lround_27c5e:
    ands r3, r0
.Lround_27c60:
    subs.w r2, r3, ip, lsr #1
.Lround_27c64:
    bic.w r0, r0, r3
.Lround_27c68:
    it eq
.Lround_27c6a:
    tsteq.w r0, ip
.Lround_27c6e:
    it hi
.Lround_27c70:
    addhi r0, ip
.Lround_27c72:
    bx lr
.Lround_27c74:
    cmp.w r3, #4294967295
.Lround_27c78:
    it cs
.Lround_27c7a:
    movscs.w ip, r0, lsl #8
.Lround_27c7e:
    and.w r0, r0, #2147483648
.Lround_27c82:
    it hi
.Lround_27c84:
    orrhi.w r0, r0, #1065353216
.Lround_27c88:
    bx lr
.Lround_27c8a:
    cmp.w ip, #255
.Lround_27c8e:
    it lt
.Lround_27c90:
    bxlt lr
.Lround_27c92:
    push {r4, lr}
.Lround_27c94:
    bl softfloat_exception_core
.balign 4
.Lround_27c98:
    .word 0xbeb6db40
    /* Selector action zero continues in the round-to-even formatter. */
    .hword 0xf7f8, 0xbf53
/* Exponent 255 alone enters selector: actions 5 (infinity) or 6 (NaN).
 * The action-zero formatter continuation is unreachable here. */
.size round_even_core, .-round_even_core
.section .text.softfloat_runtime_round_to_int,"ax",%progbits
.balign 2
.global runtime_round_to_int
.type runtime_round_to_int, %function
.thumb_func
runtime_round_to_int:
.Lround_23e94:
    push {r4, lr}
.Lround_23e96:
    vpush {d8}
.Lround_23e9a:
    vmov.f32 s17, s0
.Lround_23e9e:
    vmov r0, s17
.Lround_23ea2:
    bl round_even_core
.Lround_23ea6:
    vmov s16, r0
.Lround_23eaa:
    vmov r0, s17
.Lround_23eae:
    vmov.f32 s0, #1.0
.Lround_23eb2:
    vsub.f32 s1, s17, s16
.Lround_23eb6:
    tst.w r0, #2147483648
.Lround_23eba:
    mov.w r0, #4278190080
.Lround_23ebe:
    vmov r1, s1
.Lround_23ec2:
    beq.n .Lround_23eea
.Lround_23ec4:
    cmp.w r1, #3204448256
.Lround_23ec8:
    itt cs
.Lround_23eca:
    vmovcs r1, s1
.Lround_23ece:
    cmpcs.w r0, r1, lsl #1
.Lround_23ed2:
    bcs.n .Lround_23efa
.Lround_23ed4:
    vmov r1, s1
.Lround_23ed8:
    cmp.w r1, #1056964608
.Lround_23edc:
    ble.n .Lround_23f18
.Lround_23ede:
    vmov r1, s1
.Lround_23ee2:
    cmp.w r0, r1, lsl #1
.Lround_23ee6:
    bcc.n .Lround_23f18
.Lround_23ee8:
    b.n .Lround_23f14
.Lround_23eea:
    cmp.w r1, #3204448256
.Lround_23eee:
    bls.n .Lround_23f00
.Lround_23ef0:
    vmov r1, s1
.Lround_23ef4:
    cmp.w r0, r1, lsl #1
.Lround_23ef8:
    bcc.n .Lround_23f00
.Lround_23efa:
    vsub.f32 s16, s16, s0
.Lround_23efe:
    b.n .Lround_23f18
.Lround_23f00:
    vmov r1, s1
.Lround_23f04:
    cmp.w r1, #1056964608
.Lround_23f08:
    blt.n .Lround_23f18
.Lround_23f0a:
    vmov r1, s1
.Lround_23f0e:
    cmp.w r0, r1, lsl #1
.Lround_23f12:
    bcc.n .Lround_23f18
.Lround_23f14:
    vadd.f32 s16, s16, s0
.Lround_23f18:
    vmov r1, s16
.Lround_23f1c:
    cmp.w r1, #1325400064
.Lround_23f20:
    ble.n .Lround_23f2c
.Lround_23f22:
    vmov r1, s16
.Lround_23f26:
    cmp.w r0, r1, lsl #1
.Lround_23f2a:
    bcs.n .Lround_23f40
.Lround_23f2c:
    vmov r1, s16
.Lround_23f30:
    cmp.w r1, #3472883712
.Lround_23f34:
    bls.n .Lround_23f46
.Lround_23f36:
    vmov r1, s16
.Lround_23f3a:
    cmp.w r0, r1, lsl #1
.Lround_23f3e:
    bcc.n .Lround_23f46
.Lround_23f40:
    movs r0, #1
.Lround_23f42:
    bl runtime_errno_set_helper
.Lround_23f46:
    vmov r0, s16
.Lround_23f4a:
    vpop {d8}
.Lround_23f4e:
    ldmia.w sp!, {r4, lr}
.Lround_23f52:
    b.w float_to_int_core
.size runtime_round_to_int, .-runtime_round_to_int
.section .text.softfloat_float_to_int,"ax",%progbits
.balign 2
.global float_to_int_core
.type float_to_int_core, %function
.thumb_func
float_to_int_core:
.Lround_27b2e:
    vmov s0, r0
.Lround_27b32:
    vcvtr.s32.f32 s2, s0
.Lround_27b36:
    vmov r0, s2
.Lround_27b3a:
    bx lr
.size float_to_int_core, .-float_to_int_core

/* Unsigned 64-bit to binary32 entry point and conversion core. */
.section .text.softfloat_uint64_to_float_entry,"ax",%progbits
.balign 4
.global runtime_uint64_to_float
.type runtime_uint64_to_float, %function
.thumb_func
runtime_uint64_to_float:
    mov.w r2, #0x3f800000
    .hword 0xe009
.size runtime_uint64_to_float, .-runtime_uint64_to_float

.section .text.softfloat_uint64_to_float_core,"ax",%progbits
.balign 2
    movs r3, r1
    add.w r2, r2, #0x0f000000
    itee ne
    movne r3, #32
    movseq.w r1, r0
    bxeq lr
    clz ip, r1
    sub.w r3, r3, ip
    lsl.w r1, r1, ip
    lsr.w ip, r0, r3
    add.w r2, r2, r3, lsl #23
    rsb.w r3, r3, #32
    orr.w r1, r1, ip
    lsls.w ip, r0, r3
    orrs.w ip, ip, r1, lsl #25
    adc.w r0, r2, r1, lsr #8
    it ne
    movsne.w ip, #0x04000000
    it cs
    biccs.w r0, r0, #1
    bx lr

/* Binary32 exception selection and binary32-to-binary64 conversion. */
.section .text.softfloat_softfloat_exception,"ax",%progbits
.balign 4
.global softfloat_exception_core
.type softfloat_exception_core, %function
.thumb_func
softfloat_exception_core:
.Lsoft_float_27b9c:
    add.w lr, lr, #2
.Lsoft_float_27ba0:
    bic.w lr, lr, #3
.Lsoft_float_27ba4:
    ldr.w r4, [lr], #4
.Lsoft_float_27ba8:
    tst r4, r4
.Lsoft_float_27baa:
    bmi.n .Lsoft_float_27bb6
.Lsoft_float_27bac:
    mov.w r2, r1, lsl #1
.Lsoft_float_27bb0:
    cmp.w r2, #4278190080
.Lsoft_float_27bb4:
    bhi.n .Lsoft_float_27bda
.Lsoft_float_27bb6:
    mov.w r3, r0, lsl #1
.Lsoft_float_27bba:
    cmp.w r3, #4278190080
.Lsoft_float_27bbe:
    bhi.n .Lsoft_float_27bda
.Lsoft_float_27bc0:
    bne.n .Lsoft_float_27be0
.Lsoft_float_27bc2:
    mov.w ip, r0, lsr #31
.Lsoft_float_27bc6:
    cmp.w r2, #4278190080
.Lsoft_float_27bca:
    add.w ip, ip, ip, lsl #1
.Lsoft_float_27bce:
    add.w ip, ip, #2
.Lsoft_float_27bd2:
    it eq
.Lsoft_float_27bd4:
    adceq.w ip, ip, r1, lsr #31
.Lsoft_float_27bd8:
    b.n .Lsoft_float_27be4
.Lsoft_float_27bda:
    mov.w ip, #8
.Lsoft_float_27bde:
    b.n .Lsoft_float_27be4
.Lsoft_float_27be0:
    mov.w ip, r1, lsr #31
.Lsoft_float_27be4:
    add.w r3, ip, ip, lsl #1
.Lsoft_float_27be8:
    lsr.w r4, r4, r3
.Lsoft_float_27bec:
    and.w r4, r4, #7
.Lsoft_float_27bf0:
    subs.w ip, r4, #4
.Lsoft_float_27bf4:
    bcs.n .Lsoft_float_27c04
.Lsoft_float_27bf6:
    add.w ip, lr, r4, lsl #2
.Lsoft_float_27bfa:
    ldmia.w sp!, {r4, lr}
.Lsoft_float_27bfe:
    orr.w ip, ip, #1
.Lsoft_float_27c02:
    bx ip
.Lsoft_float_27c04:
    tbb [pc, ip]
.Lsoft_float_27c08:
    .byte 7, 8, 2, 2
.Lsoft_float_27c0c:
    mov.w r0, #2147483648
.Lsoft_float_27c10:
    sub.w r0, r0, #4194304
.Lsoft_float_27c14:
    pop {r4, pc}
.Lsoft_float_27c16:
    mov r0, r1
.Lsoft_float_27c18:
    lsls r2, r0, #1
.Lsoft_float_27c1a:
    it ne
.Lsoft_float_27c1c:
    rsbsne r2, r2, #16777216
.Lsoft_float_27c20:
    it hi
.Lsoft_float_27c22:
    andhi.w r0, r0, #2147483648
.Lsoft_float_27c26:
    pop {r4, pc}
.section .text.softfloat_f2d,"ax",%progbits
.balign 4
.global __wrap___aeabi_f2d
.type __wrap___aeabi_f2d, %function
.thumb_func
__wrap___aeabi_f2d:
.Lsoft_float_27ad8:
    lsls r3, r0, #1
.Lsoft_float_27ada:
    mov.w ip, r3, lsr #3
.Lsoft_float_27ade:
    mov.w ip, ip, rrx
.Lsoft_float_27ae2:
    eors r3, r0
.Lsoft_float_27ae4:
    itttt ne
.Lsoft_float_27ae6:
    lslne r0, r0, #29
.Lsoft_float_27ae8:
    addne.w r1, ip, #939524096
.Lsoft_float_27aec:
    tstne.w r3, #2130706432
.Lsoft_float_27af0:
    bxne lr
.Lsoft_float_27af2:
    tst.w ip, #134217728
.Lsoft_float_27af6:
    bne.n .Lsoft_float_27b00
.Lsoft_float_27af8:
    movs r0, #0
.Lsoft_float_27afa:
    and.w r1, ip, #2147483648
.Lsoft_float_27afe:
    bx lr
.Lsoft_float_27b00:
    and.w r3, ip, #2147483648
.Lsoft_float_27b04:
    orr.w r0, r3, r0, lsr #29
.Lsoft_float_27b08:
    push {r4, lr}
.Lsoft_float_27b0a:
    orr.w r0, r0, ip, lsl #3
.Lsoft_float_27b0e:
    bl softfloat_exception_core
.Lsoft_float_27b12:
    nop
.balign 4
.Lsoft_float_27b14:
    .word 0x92000000
.Lsoft_float_27b18:
    mov.w r1, r0
.Lsoft_float_27b1c:
    b.w softdouble_saturate_core
.section .text.softfloat_f2d_exception_result,"ax",%progbits
.balign 2
.Lsoft_float_27b20:
    mov.w r1, #0x80000000
    mov.w r0, #0
    sub.w r1, r1, #0x80000
    bx lr

/* Software binary64 division with a PC-relative reciprocal seed table. */
.section .text.softfloat_ddiv,"ax",%progbits
.balign 4
.global __wrap___aeabi_ddiv
.type __wrap___aeabi_ddiv, %function
.thumb_func
__wrap___aeabi_ddiv:
.Lsoft_divide_27228:
    stmdb sp!, {r6, r7, r8, lr}
.Lsoft_divide_2722c:
    push {r4, r5}
.Lsoft_divide_2722e:
    /* The literal follows the 128-byte reciprocal table. */
    .hword 0xf8df, 0xc2a0
.Lsoft_divide_27232:
    bics.w r4, ip, r1, lsr #4
.Lsoft_divide_27236:
    it ne
.Lsoft_divide_27238:
    bicsne.w r4, ip, r3, lsr #4
.Lsoft_divide_2723c:
    beq.w .Lsoft_divide_27418
.Lsoft_divide_27240:
    eor.w r5, r1, r3
.Lsoft_divide_27244:
    ands.w r4, ip, r1, lsr #4
.Lsoft_divide_27248:
    orr.w r4, r4, r5, lsr #31
.Lsoft_divide_2724c:
    it ne
.Lsoft_divide_2724e:
    andsne.w r5, ip, r3, lsr #4
.Lsoft_divide_27252:
    beq.w .Lsoft_divide_273e4
.Lsoft_divide_27256:
    sub.w r4, r4, r5
.Lsoft_divide_2725a:
    mov.w r5, #2147483648
.Lsoft_divide_2725e:
    orr.w r1, r5, r1, lsl #11
.Lsoft_divide_27262:
    orr.w r3, r5, r3, lsl #11
.Lsoft_divide_27266:
    orr.w r1, r1, r0, lsr #21
.Lsoft_divide_2726a:
    orr.w r3, r3, r2, lsr #21
.Lsoft_divide_2726e:
    add.w r4, r4, #66846720
.Lsoft_divide_27272:
    cmp r1, r3
.Lsoft_divide_27274:
    add.w r4, r4, #131072
.Lsoft_divide_27278:
    mov.w r0, r0, lsl #11
.Lsoft_divide_2727c:
    mov.w r2, r2, lsl #11
.Lsoft_divide_27280:
    it eq
.Lsoft_divide_27282:
    cmpeq r0, r2
.Lsoft_divide_27284:
    beq.w .Lsoft_divide_273c0
.Lsoft_divide_27288:
    mov.w r5, r3, lsr #24
.Lsoft_divide_2728c:
    /* Fixed-address PC-relative base; r5 contributes 0x80..0xff. */
    addw r6, pc, #320
.Lsoft_divide_27290:
    ldrb r6, [r6, r5]
.Lsoft_divide_27292:
    mov.w r5, r3, lsr #16
.Lsoft_divide_27296:
    mul.w r7, r6, r5
.Lsoft_divide_2729a:
    rsb r7, r7, #16777216
.Lsoft_divide_2729e:
    mul.w r7, r6, r7
.Lsoft_divide_272a2:
    mov.w r7, r7, lsr #15
.Lsoft_divide_272a6:
    umull r6, r8, r7, r3
.Lsoft_divide_272aa:
    negs r6, r6
.Lsoft_divide_272ac:
    rsb r8, r8, #65536
.Lsoft_divide_272b0:
    umull ip, lr, r7, r6
.Lsoft_divide_272b4:
    it cc
.Lsoft_divide_272b6:
    subcc.w r8, r8, #1
.Lsoft_divide_272ba:
    mla r6, r7, r8, lr
.Lsoft_divide_272be:
    mov.w lr, #0
.Lsoft_divide_272c2:
    umull r8, ip, r6, r2
.Lsoft_divide_272c6:
    umlal ip, lr, r6, r3
.Lsoft_divide_272ca:
    rsbs ip, ip, #0
.Lsoft_divide_272ce:
    rsb lr, lr, #2147483648
.Lsoft_divide_272d2:
    it cc
.Lsoft_divide_272d4:
    subcc.w lr, lr, #1
.Lsoft_divide_272d8:
    umull r5, r8, r6, ip
.Lsoft_divide_272dc:
    mov.w r7, #0
.Lsoft_divide_272e0:
    mov.w r5, #0
.Lsoft_divide_272e4:
    umlal r8, r7, r6, lr
.Lsoft_divide_272e8:
    umull ip, r6, r1, r8
.Lsoft_divide_272ec:
    umull ip, lr, r0, r7
.Lsoft_divide_272f0:
    adds.w r6, r6, lr
.Lsoft_divide_272f4:
    adc.w r5, r5, #0
.Lsoft_divide_272f8:
    umlal r6, r5, r1, r7
.Lsoft_divide_272fc:
    adds.w lr, r5, #1879048192
.Lsoft_divide_27300:
    ittt vc
.Lsoft_divide_27302:
    subvc.w r4, r4, #65536
.Lsoft_divide_27306:
    movsvc.w r6, r6, lsl #1
.Lsoft_divide_2730a:
    adcvc r5, r5
.Lsoft_divide_2730c:
    adds.w r7, r6, #128
.Lsoft_divide_27310:
    adc.w r5, r5, #0
.Lsoft_divide_27314:
    mov.w r7, r7, lsr #8
.Lsoft_divide_27318:
    mov.w r6, r6, lsl #24
.Lsoft_divide_2731c:
    orr.w r7, r7, r5, lsl #24
.Lsoft_divide_27320:
    sub.w r6, r6, #1862270976
.Lsoft_divide_27324:
    cmp.w r6, #268435456
.Lsoft_divide_27328:
    mov.w r5, r5, lsr #8
.Lsoft_divide_2732c:
    bls.n .Lsoft_divide_2736e
.Lsoft_divide_2732e:
    mov r0, r7
.Lsoft_divide_27330:
    bic.w ip, r4, #1
.Lsoft_divide_27334:
    add.w r1, r5, r4, lsl #31
.Lsoft_divide_27338:
    cmp.w ip, #133169152
.Lsoft_divide_2733c:
    add.w r1, r1, ip, lsl #4
.Lsoft_divide_27340:
    bhi.n .Lsoft_divide_27348
.Lsoft_divide_27342:
    pop {r4, r5, r6, r7}
.Lsoft_divide_27344:
    ldmia.w sp!, {r8, pc}
.Lsoft_divide_27348:
    tst r4, r4
.Lsoft_divide_2734a:
    itt pl
.Lsoft_divide_2734c:
    addpl.w lr, r1, #1048576
.Lsoft_divide_27350:
    teqpl lr, r4, lsl #31
.Lsoft_divide_27354:
    bmi.n .Lsoft_divide_2735c
.Lsoft_divide_27356:
    pop {r4, r5, r6, r7}
.Lsoft_divide_27358:
    ldmia.w sp!, {r8, pc}
.Lsoft_divide_2735c:
    tst r4, r4
.Lsoft_divide_2735e:
    bmi.n .Lsoft_divide_273d2
.Lsoft_divide_27360:
    pop {r4, r5, r6, r7}
.Lsoft_divide_27362:
    sub.w r1, r1, #1610612736
.Lsoft_divide_27366:
    ldmia.w sp!, {r8, lr}
.Lsoft_divide_2736a:
    b.w softdouble_saturate_core
.Lsoft_divide_2736e:
    mov.w r2, r2, lsr #11
.Lsoft_divide_27372:
    orr.w r2, r2, r3, lsl #21
.Lsoft_divide_27376:
    mov.w r3, r3, lsr #11
.Lsoft_divide_2737a:
    umull r8, r6, r7, r2
.Lsoft_divide_2737e:
    mov.w r0, r0, lsr #11
.Lsoft_divide_27382:
    orr.w r0, r0, r1, lsl #21
.Lsoft_divide_27386:
    mla r6, r7, r3, r6
.Lsoft_divide_2738a:
    tst.w lr, lr
.Lsoft_divide_2738e:
    mov.w r1, r1, lsr #11
.Lsoft_divide_27392:
    mla r6, r5, r2, r6
.Lsoft_divide_27396:
    it pl
.Lsoft_divide_27398:
    subpl.w r6, r6, r0, lsl #20
.Lsoft_divide_2739c:
    subs.w r6, r6, r0, lsl #20
.Lsoft_divide_273a0:
    mov.w lr, r2, lsr #1
.Lsoft_divide_273a4:
    orr.w lr, lr, r3, lsl #31
.Lsoft_divide_273a8:
    adds.w lr, r8, lr
.Lsoft_divide_273ac:
    adcs.w lr, r6, r3, lsr #1
.Lsoft_divide_273b0:
    bpl.n .Lsoft_divide_2732e
.Lsoft_divide_273b2:
    adds.w r8, r8, r2
.Lsoft_divide_273b6:
    adcs r6, r3
.Lsoft_divide_273b8:
    .hword 0x1c7f
.Lsoft_divide_273ba:
    adcs.w r5, r5, #0
.Lsoft_divide_273be:
    b.n .Lsoft_divide_2732e
.Lsoft_divide_273c0:
    mov.w r5, #1048576
.Lsoft_divide_273c4:
    mov.w r7, #0
.Lsoft_divide_273c8:
    mov.w r6, #0
.Lsoft_divide_273cc:
    mov.w r8, #0
.Lsoft_divide_273d0:
    b.n .Lsoft_divide_2732e
.Lsoft_divide_273d2:
    pop {r4, r5, r6, r7}
.Lsoft_divide_273d4:
    mov.w r0, #0
.Lsoft_divide_273d8:
    add.w r1, r1, #1610612736
.Lsoft_divide_273dc:
    and.w r1, r1, #2147483648
.Lsoft_divide_273e0:
    ldmia.w sp!, {r8, pc}
.Lsoft_divide_273e4:
    tst.w r1, ip, lsl #4
.Lsoft_divide_273e8:
    and.w r5, ip, r3, lsr #4
.Lsoft_divide_273ec:
    beq.n .Lsoft_divide_273f0
.Lsoft_divide_273ee:
    b.n .Lsoft_divide_2740a
.Lsoft_divide_273f0:
    tst.w r3, ip, lsl #4
.Lsoft_divide_273f4:
    beq.w .Lsoft_divide_27442
.Lsoft_divide_273f8:
    mov.w r0, #0
.Lsoft_divide_273fc:
    pop {r4, r5, r6, r7}
.Lsoft_divide_273fe:
    eor.w r1, r1, r3
.Lsoft_divide_27402:
    and.w r1, r1, #2147483648
.Lsoft_divide_27406:
    ldmia.w sp!, {r8, pc}
.Lsoft_divide_2740a:
    pop {r4, r5, r6, r7}
.Lsoft_divide_2740c:
    ldmia.w sp!, {r8, lr}
.Lsoft_divide_27410:
    eor.w r1, r1, r3
.Lsoft_divide_27414:
    b.w softdouble_saturate_core
.Lsoft_divide_27418:
    pop {r4, r5, r6, r7}
.Lsoft_divide_2741a:
    ldmia.w sp!, {r8, lr}
.Lsoft_divide_2741e:
    push {r4, r5, r6, lr}
.Lsoft_divide_27420:
    bl softdouble_exception_core
.balign 4
.Lsoft_divide_27424:
    .word 0x3efc7e09
.Lsoft_divide_27428:
    b.w .Lsoft_divide_2743a
.Lsoft_divide_2742c:
    eor.w r1, r1, r3
.Lsoft_divide_27430:
    mov.w r0, #0
.Lsoft_divide_27434:
    and.w r1, r1, #2147483648
.Lsoft_divide_27438:
    bx lr
.Lsoft_divide_2743a:
    eor.w r1, r1, r3
.Lsoft_divide_2743e:
    b.w softdouble_saturate_core
.Lsoft_divide_27442:
    pop {r4, r5, r6, r7}
.Lsoft_divide_27444:
    ldmia.w sp!, {r8, lr}
.Lsoft_divide_27448:
    mov.w r0, #0
.Lsoft_divide_2744c:
    /* Keep the saturation literal 0x84 bytes beyond the aligned PC. */
    .hword 0x4921
.Lsoft_divide_2744e:
    bx lr
.section .rodata.softfloat_ddiv_literals,"a",%progbits
.balign 4
.Lsoft_divide_exponent_mask:
    .word 0x07ff0000
.Lsoft_divide_nan:
    .word 0x7ff80000

/* Binary64-to-uint32 conversion with saturating exceptional cases. */
.section .text.softfloat_d2uiz,"ax",%progbits
.balign 4
.global runtime_double_to_uint
.global __wrap___aeabi_d2uiz
.type runtime_double_to_uint, %function
.type __wrap___aeabi_d2uiz, %function
.thumb_func
runtime_double_to_uint:
__wrap___aeabi_d2uiz:
    mov.w r3, r1, lsr #20
    sub.w r3, r3, #1024
    rsbs r3, r3, #30
    it ge
    rsbsge ip, r3, #32
    blt.n .Ld2uiz_negative_shift
    mov.w r2, #0x80000000
    orr.w r2, r2, r1, lsl #11
    orrs.w r2, r2, r0, lsr #21
    lsrs.w r0, r2, r3
    bx lr
.Ld2uiz_negative_shift:
    tst r1, r1
    bmi.n .Ld2uiz_zero
    cmp r3, #16
    itt gt
    movgt r0, #0
    bxgt lr
    add.w ip, r3, #992
    cmp.w ip, #0xffffffff
    beq.n .Ld2uiz_exception
.Ld2uiz_saturate:
    mov.w r0, #0xffffffff
    bx lr
.Ld2uiz_zero:
    mov.w r0, #0
    bx lr
.Ld2uiz_exception:
    push {r4, r5, r6, lr}
    bl softdouble_exception_core
    nop
    .word 0x80249249
    movs r0, #0
    bx lr
.size runtime_double_to_uint, .-runtime_double_to_uint
.size __wrap___aeabi_d2uiz, .-__wrap___aeabi_d2uiz

.section .text.softfloat_d2uiz_saturate,"ax",%progbits
.balign 2
    b.n .Ld2uiz_saturate

/* Unsigned uint32-to-binary64 conversion. */
.section .text.softfloat_ui2d,"ax",%progbits
.balign 2
.global __wrap___aeabi_ui2d
.type __wrap___aeabi_ui2d, %function
.thumb_func
__wrap___aeabi_ui2d:
    clz r3, r0
    lsls.w r1, r0, r3
    beq.n .Lui2d_zero
    rsb r3, r3, #29
    add.w r3, r3, #1024
    mov.w r0, r1, lsl #21
    mov.w r2, r3, lsl #20
    add.w r1, r2, r1, lsr #11
    bx lr
.Lui2d_zero:
    mov.w r0, #0
    bx lr
.size __wrap___aeabi_ui2d, .-__wrap___aeabi_ui2d

/* Software binary64 multiplication. */
.section .text.softfloat_dmul,"ax",%progbits
.balign 4
.global __wrap___aeabi_dmul
.type __wrap___aeabi_dmul, %function
.thumb_func
__wrap___aeabi_dmul:
.Lsoft_multiply_27558:
    ldr.w ip, .Lsoft_multiply_276a4
.Lsoft_multiply_2755c:
    push {r4, r5, r6, lr}
.Lsoft_multiply_2755e:
    ands.w lr, ip, r1, lsr #4
.Lsoft_multiply_27562:
    ittt ne
.Lsoft_multiply_27564:
    andsne.w r5, ip, r3, lsr #4
.Lsoft_multiply_27568:
    teqne lr, ip
.Lsoft_multiply_2756c:
    teqne r5, ip
.Lsoft_multiply_27570:
    beq.w .Lsoft_multiply_27652
.Lsoft_multiply_27574:
    eor.w r4, r1, r3
.Lsoft_multiply_27578:
    bic.w r3, r3, ip, lsl #5
.Lsoft_multiply_2757c:
    orr.w lr, lr, r4, lsr #31
.Lsoft_multiply_27580:
    bic.w r1, r1, ip, lsl #5
.Lsoft_multiply_27584:
    orr.w r1, r1, #1048576
.Lsoft_multiply_27588:
    orr.w r3, r3, #1048576
.Lsoft_multiply_2758c:
    add lr, r5
.Lsoft_multiply_2758e:
    umull r4, ip, r1, r2
.Lsoft_multiply_27592:
    umull r6, r5, r0, r3
.Lsoft_multiply_27596:
    sub.w lr, lr, #66846720
.Lsoft_multiply_2759a:
    adds r6, r6, r4
.Lsoft_multiply_2759c:
    adcs.w r5, r5, ip
.Lsoft_multiply_275a0:
    umull r4, ip, r1, r3
.Lsoft_multiply_275a4:
    adc.w r3, ip, #0
.Lsoft_multiply_275a8:
    umull r1, ip, r0, r2
.Lsoft_multiply_275ac:
    adds.w r6, r6, ip
.Lsoft_multiply_275b0:
    adcs r5, r4
.Lsoft_multiply_275b2:
    adcs.w r3, r3, #0
.Lsoft_multiply_275b6:
    tst r1, r1
.Lsoft_multiply_275b8:
    it ne
.Lsoft_multiply_275ba:
    orrne.w r6, r6, #1
.Lsoft_multiply_275be:
    tst.w r3, #512
.Lsoft_multiply_275c2:
    bne.n .Lsoft_multiply_275dc
.Lsoft_multiply_275c4:
    mov.w r1, r3, lsl #12
.Lsoft_multiply_275c8:
    mov.w r0, r5, lsl #12
.Lsoft_multiply_275cc:
    lsls r4, r6, #12
.Lsoft_multiply_275ce:
    orr.w r1, r1, r5, lsr #20
.Lsoft_multiply_275d2:
    orr.w r0, r0, r6, lsr #20
.Lsoft_multiply_275d6:
    mvn.w r2, #3
.Lsoft_multiply_275da:
    b.n .Lsoft_multiply_275f2
.Lsoft_multiply_275dc:
    mov.w r1, r3, lsl #11
.Lsoft_multiply_275e0:
    mov.w r0, r5, lsl #11
.Lsoft_multiply_275e4:
    lsls r4, r6, #11
.Lsoft_multiply_275e6:
    orr.w r1, r1, r5, lsr #21
.Lsoft_multiply_275ea:
    orr.w r0, r0, r6, lsr #21
.Lsoft_multiply_275ee:
    mvn.w r2, #2
.Lsoft_multiply_275f2:
    add.w r2, r2, lr, asr #16
.Lsoft_multiply_275f6:
    add.w r5, r1, r2, lsl #20
.Lsoft_multiply_275fa:
    eor.w r1, r5, lr, lsl #31
.Lsoft_multiply_275fe:
    beq.n .Lsoft_multiply_27616
.Lsoft_multiply_27600:
    movs.w ip, r4, lsl #1
.Lsoft_multiply_27604:
    it ne
.Lsoft_multiply_27606:
    bicne.w r4, r4, #2147483648
.Lsoft_multiply_2760a:
    adcs.w r0, r0, #0
.Lsoft_multiply_2760e:
    adc.w r1, r1, #0
.Lsoft_multiply_27612:
    bic.w r0, r0, r4, lsr #31
.Lsoft_multiply_27616:
    movw ip, #2046
.Lsoft_multiply_2761a:
    cmp r2, ip
.Lsoft_multiply_2761c:
    bcs.n .Lsoft_multiply_27620
.Lsoft_multiply_2761e:
    pop {r4, r5, r6, pc}
.Lsoft_multiply_27620:
    cmp r0, r5
.Lsoft_multiply_27622:
    ite ne
.Lsoft_multiply_27624:
    movne.w r4, #1073741824
.Lsoft_multiply_27628:
    moveq.w r4, #3221225472
.Lsoft_multiply_2762c:
    cmp r6, #0
.Lsoft_multiply_2762e:
    it eq
.Lsoft_multiply_27630:
    moveq r4, #0
.Lsoft_multiply_27632:
    cmp.w lr, #67108864
.Lsoft_multiply_27636:
    it ge
.Lsoft_multiply_27638:
    subge.w r1, r1, #1610612736
.Lsoft_multiply_2763c:
    ldmia.w sp!, {r4, r5, r6, lr}
.Lsoft_multiply_27640:
    bge.w softdouble_saturate_core
.Lsoft_multiply_27644:
    mov.w r0, #0
.Lsoft_multiply_27648:
    add.w r1, r1, #1610612736
.Lsoft_multiply_2764c:
    and.w r1, r1, #2147483648
.Lsoft_multiply_27650:
    bx lr
.Lsoft_multiply_27652:
    teq lr, ip
.Lsoft_multiply_27656:
    and.w r5, ip, r3, lsr #4
.Lsoft_multiply_2765a:
    it ne
.Lsoft_multiply_2765c:
    teqne r5, ip
.Lsoft_multiply_27660:
    beq.n .Lsoft_multiply_27670
.Lsoft_multiply_27662:
    mov.w r0, #0
.Lsoft_multiply_27666:
    eor.w lr, r1, r3
.Lsoft_multiply_2766a:
    and.w r1, lr, #2147483648
.Lsoft_multiply_2766e:
    pop {r4, r5, r6, pc}
.Lsoft_multiply_27670:
    bl softdouble_exception_core
.balign 4
.Lsoft_multiply_27674:
    .word 0x3e010089
.Lsoft_multiply_27678:
    b.w .Lsoft_multiply_2768a
.Lsoft_multiply_2767c:
    b.w .Lsoft_multiply_27692
.Lsoft_multiply_27680:
    mov.w ip, r3, lsl #1
.Lsoft_multiply_27684:
    movs.w ip, ip, lsr #21
.Lsoft_multiply_27688:
    beq.n .Lsoft_multiply_2769c
.Lsoft_multiply_2768a:
    eor.w r1, r1, r3
.Lsoft_multiply_2768e:
    b.w softdouble_saturate_core
.Lsoft_multiply_27692:
    mov.w ip, r1, lsl #1
.Lsoft_multiply_27696:
    movs.w ip, ip, lsr #21
.Lsoft_multiply_2769a:
    bne.n .Lsoft_multiply_2768a
.Lsoft_multiply_2769c:
    mov.w r0, #0
.Lsoft_multiply_276a0:
    ldr r1, .Lsoft_multiply_276a8
.Lsoft_multiply_276a2:
    bx lr
.balign 4
.Lsoft_multiply_276a4:
    .word 0x07ff0000
.balign 4
.Lsoft_multiply_276a8:
    .word 0x7ff80000

/* Binary64-to-binary32 conversion with a shared rounding tail. */
.section .text.softfloat_d2f,"ax",%progbits
.balign 4
.global __aeabi_d2f
.type __aeabi_d2f, %function
.thumb_func
__aeabi_d2f:
    bic.w r2, r1, #0x80000000
    sub.w r2, r2, #0x38000000
    cmp.w r2, #0x00100000
    and.w r3, r1, #0x80000000
    it cs
    rsbscs ip, r2, #0x0ff00000
    bls.n .Ld2f_exception_range
    movs.w ip, r0, lsl #4
    orr.w r2, r3, r2, lsl #3
    adc.w r0, r2, r0, lsr #29
    it ne
    bxne lr
    it cs
    biccs.w r0, r0, #1
    bx lr
.Ld2f_exception_range:
    cmp.w r2, #0x00100000
    itt lt
    movlt r0, r3
    bxlt lr
    mov.w ip, r1, lsl #1
    cmn.w ip, #0x00200000
    bcs.n .Ld2f_exception
.Ld2f_special:
    mov r0, r1
    b.w .Ld2f_pack
.Ld2f_exception:
    push {r4, r5, r6, lr}
    bl softdouble_exception_core
    .word 0x89000000
    b.w .Ld2f_special
.size __aeabi_d2f, .-__aeabi_d2f

.section .text.softfloat_d2f_exception_result,"ax",%progbits
.balign 2
    mov.w r0, #0x80000000
    sub.w r0, r0, #0x00400000
    bx lr

.section .text.softfloat_d2f_pack,"ax",%progbits
.balign 2
.Ld2f_pack:
    movs r1, #255
    orr.w r0, r1, r0, lsr #23
    lsls r0, r0, #23
    bx lr

/* Shared software binary64 add, subtract, and reverse-subtract entries. */
.section .text.softfloat_dadd,"ax",%progbits
.balign 4
.global __wrap___aeabi_dadd
.type __wrap___aeabi_dadd, %function
.thumb_func
__wrap___aeabi_dadd:
.Lsoft_arithmetic_270d8:
    push {r4, lr}
.Lsoft_arithmetic_270da:
    teq r1, r3
.Lsoft_arithmetic_270de:
    it mi
.Lsoft_arithmetic_270e0:
    eormi.w r3, r3, #2147483648
.Lsoft_arithmetic_270e4:
    bmi.w .Lsoft_arithmetic_27914
.Lsoft_arithmetic_270e8:
    subs r4, r0, r2
.Lsoft_arithmetic_270ea:
    sbcs.w ip, r1, r3
.Lsoft_arithmetic_270ee:
    bcs.n .Lsoft_arithmetic_270fc
.Lsoft_arithmetic_270f0:
    adds r2, r2, r4
.Lsoft_arithmetic_270f2:
    adc.w r3, r3, ip
.Lsoft_arithmetic_270f6:
    subs r0, r0, r4
.Lsoft_arithmetic_270f8:
    sbc.w r1, r1, ip
.Lsoft_arithmetic_270fc:
    ldr.w lr, .Lsoft_arithmetic_27224
.Lsoft_arithmetic_27100:
    mov.w r4, r1, lsr #20
.Lsoft_arithmetic_27104:
    sub.w ip, r4, r3, lsr #20
.Lsoft_arithmetic_27108:
    tst.w lr, r3, lsl #1
.Lsoft_arithmetic_2710c:
    it ne
.Lsoft_arithmetic_2710e:
    teqne lr, r4, lsl #21
.Lsoft_arithmetic_27112:
    beq.n .Lsoft_arithmetic_271fc
.Lsoft_arithmetic_27114:
    bic.w r3, r3, lr
.Lsoft_arithmetic_27118:
    rsbs lr, ip, #32
.Lsoft_arithmetic_2711c:
    bic.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_27120:
    orr.w r3, r3, #1048576
.Lsoft_arithmetic_27124:
    bcc.n .Lsoft_arithmetic_2718c
.Lsoft_arithmetic_27126:
    lsr.w lr, r2, ip
.Lsoft_arithmetic_2712a:
    adds.w r0, r0, lr
.Lsoft_arithmetic_2712e:
    lsr.w lr, r3, ip
.Lsoft_arithmetic_27132:
    adc.w r1, r1, lr
.Lsoft_arithmetic_27136:
    rsb lr, ip, #32
.Lsoft_arithmetic_2713a:
    lsl.w lr, r3, lr
.Lsoft_arithmetic_2713e:
    adds.w r0, r0, lr
.Lsoft_arithmetic_27142:
    adcs.w r1, r1, #0
.Lsoft_arithmetic_27146:
    cmp.w r1, #1048576
.Lsoft_arithmetic_2714a:
    rsb lr, ip, #32
.Lsoft_arithmetic_2714e:
    bcs.n .Lsoft_arithmetic_271b6
.Lsoft_arithmetic_27150:
    add.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_27154:
    lsls.w r2, r2, lr
.Lsoft_arithmetic_27158:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_2715c:
    it pl
.Lsoft_arithmetic_2715e:
    bxpl lr
.Lsoft_arithmetic_27160:
    .hword 0x1c40
.Lsoft_arithmetic_27162:
    it cc
.Lsoft_arithmetic_27164:
    movscc.w r2, r2, lsl #1
.Lsoft_arithmetic_27168:
    it ne
.Lsoft_arithmetic_2716a:
    bxne lr
.Lsoft_arithmetic_2716c:
    cmp r0, #0
.Lsoft_arithmetic_2716e:
    ite ne
.Lsoft_arithmetic_27170:
    bicne.w r0, r0, #1
.Lsoft_arithmetic_27174:
    adceq.w r1, r1, #0
.Lsoft_arithmetic_27178:
    mov.w r3, r1, lsl #1
.Lsoft_arithmetic_2717c:
    cmn.w r3, #2097152
.Lsoft_arithmetic_27180:
    it cc
.Lsoft_arithmetic_27182:
    bxcc lr
.Lsoft_arithmetic_27184:
    sub.w r1, r1, #1610612736
.Lsoft_arithmetic_27188:
    b.w softdouble_saturate_core
.Lsoft_arithmetic_2718c:
    cmp r2, #1
.Lsoft_arithmetic_2718e:
    adc.w r2, r3, r3
.Lsoft_arithmetic_27192:
    sub.w ip, ip, #32
.Lsoft_arithmetic_27196:
    rsbs lr, ip, #31
.Lsoft_arithmetic_2719a:
    itee cc
.Lsoft_arithmetic_2719c:
    movcc.w lr, #0
.Lsoft_arithmetic_271a0:
    lsrcs.w r3, r3, ip
.Lsoft_arithmetic_271a4:
    addscs.w r0, r0, r3
.Lsoft_arithmetic_271a8:
    adc.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_271ac:
    cmp.w r4, r1, lsr #20
.Lsoft_arithmetic_271b0:
    beq.n .Lsoft_arithmetic_27154
.Lsoft_arithmetic_271b2:
    sub.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_271b6:
    add.w r1, r1, #1048576
.Lsoft_arithmetic_271ba:
    lsrs r1, r1, #1
.Lsoft_arithmetic_271bc:
    movs.w r0, r0, rrx
.Lsoft_arithmetic_271c0:
    add.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_271c4:
    bcc.n .Lsoft_arithmetic_271e4
.Lsoft_arithmetic_271c6:
    adcs.w r0, r0, #0
.Lsoft_arithmetic_271ca:
    it cc
.Lsoft_arithmetic_271cc:
    lslscc.w lr, r2, lr
.Lsoft_arithmetic_271d0:
    bne.n .Lsoft_arithmetic_271e4
.Lsoft_arithmetic_271d2:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_271d6:
    b.n .Lsoft_arithmetic_2716c
.Lsoft_arithmetic_271d8:
    .hword 0x1c40
.Lsoft_arithmetic_271da:
    it cc
.Lsoft_arithmetic_271dc:
    movscc.w r2, r2, lsl #1
.Lsoft_arithmetic_271e0:
    bne.n .Lsoft_arithmetic_27178
.Lsoft_arithmetic_271e2:
    b.n .Lsoft_arithmetic_2716c
.Lsoft_arithmetic_271e4:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_271e8:
    mov.w r3, r1, lsl #1
.Lsoft_arithmetic_271ec:
    cmn.w r3, #2097152
.Lsoft_arithmetic_271f0:
    it cc
.Lsoft_arithmetic_271f2:
    bxcc lr
.Lsoft_arithmetic_271f4:
    sub.w r1, r1, #1610612736
.Lsoft_arithmetic_271f8:
    b.w softdouble_saturate_core
.Lsoft_arithmetic_271fc:
    teq lr, r4, lsl #21
.Lsoft_arithmetic_27200:
    beq.n .Lsoft_arithmetic_27214
.Lsoft_arithmetic_27202:
    tst.w r1, lr, lsr #1
.Lsoft_arithmetic_27206:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_2720a:
    itt eq
.Lsoft_arithmetic_2720c:
    andeq.w r1, r1, #2147483648
.Lsoft_arithmetic_27210:
    moveq r0, #0
.Lsoft_arithmetic_27212:
    bx lr
.Lsoft_arithmetic_27214:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_27218:
    push {r4, r5, r6, lr}
.Lsoft_arithmetic_2721a:
    bl softdouble_exception_core
.Lsoft_arithmetic_2721e:
    nop
.balign 4
.Lsoft_arithmetic_27220:
    .word 0x3ebefb64
.balign 4
.Lsoft_arithmetic_27224:
    .word 0xffe00000
.balign 4
.section .text.softfloat_reverse_subtract,"ax",%progbits
.global softdouble_reverse_subtract_core
.type softdouble_reverse_subtract_core, %function
.thumb_func
softdouble_reverse_subtract_core:
.Lsoft_arithmetic_27754:
    teq r1, r3
.Lsoft_arithmetic_27758:
    push {r4, lr}
.Lsoft_arithmetic_2775a:
    eor.w r1, r1, #2147483648
.Lsoft_arithmetic_2775e:
    bmi.w .Lsoft_arithmetic_270e8
.Lsoft_arithmetic_27762:
    eor.w r3, r3, #2147483648
.Lsoft_arithmetic_27766:
    b.w .Lsoft_arithmetic_27914
.section .text.softfloat_dsub,"ax",%progbits
.balign 4
.global __wrap___aeabi_dsub
.type __wrap___aeabi_dsub, %function
.thumb_func
__wrap___aeabi_dsub:
.Lsoft_arithmetic_27904:
    push {r4, lr}
.Lsoft_arithmetic_27906:
    teq r1, r3
.Lsoft_arithmetic_2790a:
    it mi
.Lsoft_arithmetic_2790c:
    eormi.w r3, r3, #2147483648
.Lsoft_arithmetic_27910:
    bmi.w .Lsoft_arithmetic_270e8
.Lsoft_arithmetic_27914:
    subs r4, r0, r2
.Lsoft_arithmetic_27916:
    sbcs.w ip, r1, r3
.Lsoft_arithmetic_2791a:
    bcs.n .Lsoft_arithmetic_2792c
.Lsoft_arithmetic_2791c:
    adds r2, r2, r4
.Lsoft_arithmetic_2791e:
    eor.w ip, ip, #2147483648
.Lsoft_arithmetic_27922:
    adc.w r3, r3, ip
.Lsoft_arithmetic_27926:
    subs r0, r0, r4
.Lsoft_arithmetic_27928:
    sbc.w r1, r1, ip
.Lsoft_arithmetic_2792c:
    ldr.w lr, .Lsoft_arithmetic_27ad4
.Lsoft_arithmetic_27930:
    mov.w r4, r1, lsr #20
.Lsoft_arithmetic_27934:
    sub.w ip, r4, r3, lsr #20
.Lsoft_arithmetic_27938:
    tst.w lr, r3, lsl #1
.Lsoft_arithmetic_2793c:
    it ne
.Lsoft_arithmetic_2793e:
    teqne lr, r4, lsl #21
.Lsoft_arithmetic_27942:
    beq.w .Lsoft_arithmetic_27aa6
.Lsoft_arithmetic_27946:
    bic.w r3, r3, lr, asr #1
.Lsoft_arithmetic_2794a:
    negs r2, r2
.Lsoft_arithmetic_2794c:
    bic.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_27950:
    rsb r3, r3, lr, asr #1
.Lsoft_arithmetic_27954:
    .hword 0xbf38
.Lsoft_arithmetic_27956:
    .hword 0x1e5b
.Lsoft_arithmetic_27958:
    rsbs lr, ip, #32
.Lsoft_arithmetic_2795c:
    bcc.n .Lsoft_arithmetic_279aa
.Lsoft_arithmetic_2795e:
    lsr.w lr, r2, ip
.Lsoft_arithmetic_27962:
    adds.w r0, r0, lr
.Lsoft_arithmetic_27966:
    asr.w lr, r3, ip
.Lsoft_arithmetic_2796a:
    adc.w r1, r1, lr
.Lsoft_arithmetic_2796e:
    rsb lr, ip, #32
.Lsoft_arithmetic_27972:
    lsl.w lr, r3, lr
.Lsoft_arithmetic_27976:
    adds.w r0, r0, lr
.Lsoft_arithmetic_2797a:
    adcs.w r1, r1, #0
.Lsoft_arithmetic_2797e:
    rsb lr, ip, #32
.Lsoft_arithmetic_27982:
    bmi.n .Lsoft_arithmetic_279d8
.Lsoft_arithmetic_27984:
    lsls.w r2, r2, lr
.Lsoft_arithmetic_27988:
    add.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_2798c:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_27990:
    it pl
.Lsoft_arithmetic_27992:
    bxpl lr
.Lsoft_arithmetic_27994:
    .hword 0x1c40
.Lsoft_arithmetic_27996:
    itt ne
.Lsoft_arithmetic_27998:
    cmpne.w r2, #2147483648
.Lsoft_arithmetic_2799c:
    bxne lr
.Lsoft_arithmetic_2799e:
    cmp r0, #0
.Lsoft_arithmetic_279a0:
    .hword 0xbf0c
.Lsoft_arithmetic_279a2:
    .hword 0x1c49, 0xf020, 0x0001
.Lsoft_arithmetic_279a4:
.Lsoft_arithmetic_279a8:
    bx lr
.Lsoft_arithmetic_279aa:
    adds.w lr, r2, r2
.Lsoft_arithmetic_279ae:
    adc.w r2, r3, r3
.Lsoft_arithmetic_279b2:
    it ne
.Lsoft_arithmetic_279b4:
    movne.w lr, #1
.Lsoft_arithmetic_279b8:
    orr.w r2, lr, r2, lsl #1
.Lsoft_arithmetic_279bc:
    sub.w ip, ip, #32
.Lsoft_arithmetic_279c0:
    rsbs lr, ip, #30
.Lsoft_arithmetic_279c4:
    bls.n .Lsoft_arithmetic_27a70
.Lsoft_arithmetic_279c6:
    asr.w lr, r3, ip
.Lsoft_arithmetic_279ca:
    adds.w r0, r0, lr
.Lsoft_arithmetic_279ce:
    adcs.w r1, r1, #4294967295
.Lsoft_arithmetic_279d2:
    rsb lr, ip, #30
.Lsoft_arithmetic_279d6:
    bpl.n .Lsoft_arithmetic_27984
.Lsoft_arithmetic_279d8:
    add.w lr, lr, #1
.Lsoft_arithmetic_279dc:
    lsls.w lr, r2, lr
.Lsoft_arithmetic_279e0:
    adcs r0, r0
.Lsoft_arithmetic_279e2:
    adc.w r1, r1, r1
.Lsoft_arithmetic_279e6:
    add.w ip, r1, r4, lsl #21
.Lsoft_arithmetic_279ea:
    movs.w ip, ip, lsr #21
.Lsoft_arithmetic_279ee:
    bls.n .Lsoft_arithmetic_27a12
.Lsoft_arithmetic_279f0:
    adds.w r0, r0, lr, lsr #31
.Lsoft_arithmetic_279f4:
    add.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_279f8:
    it cc
.Lsoft_arithmetic_279fa:
    cmpcc.w lr, #2147483648
.Lsoft_arithmetic_279fe:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_27a02:
    it ne
.Lsoft_arithmetic_27a04:
    bxne lr
.Lsoft_arithmetic_27a06:
    cmp r0, #0
.Lsoft_arithmetic_27a08:
    .hword 0xbf0c
.Lsoft_arithmetic_27a0a:
    .hword 0x1c49, 0xf020, 0x0001
.Lsoft_arithmetic_27a0c:
.Lsoft_arithmetic_27a10:
    bx lr
.Lsoft_arithmetic_27a12:
    bcs.n .Lsoft_arithmetic_27a76
.Lsoft_arithmetic_27a14:
    mov.w ip, r4, lsr #11
.Lsoft_arithmetic_27a18:
    adds.w r1, r1, #2097152
.Lsoft_arithmetic_27a1c:
    bic.w r4, r4, #2048
.Lsoft_arithmetic_27a20:
    beq.n .Lsoft_arithmetic_27a5a
.Lsoft_arithmetic_27a22:
    clz r2, r1
.Lsoft_arithmetic_27a26:
    subs r2, #11
.Lsoft_arithmetic_27a28:
    subs r4, r4, r2
.Lsoft_arithmetic_27a2a:
    .hword 0x1ea4
.Lsoft_arithmetic_27a2c:
    rsbs r3, r2, #32
.Lsoft_arithmetic_27a30:
    lsls r1, r2
.Lsoft_arithmetic_27a32:
    lsrs.w r3, r0, r3
.Lsoft_arithmetic_27a36:
    orrs r1, r3
.Lsoft_arithmetic_27a38:
    lsls r0, r2
.Lsoft_arithmetic_27a3a:
    add.w r1, r1, ip, lsl #31
.Lsoft_arithmetic_27a3e:
    cmp r4, #0
.Lsoft_arithmetic_27a40:
    add.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_27a44:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_27a48:
    it ge
.Lsoft_arithmetic_27a4a:
    bxge lr
.Lsoft_arithmetic_27a4c:
    add.w r1, r1, #1610612736
.Lsoft_arithmetic_27a50:
    mov.w r0, #0
.Lsoft_arithmetic_27a54:
    and.w r1, r1, #2147483648
.Lsoft_arithmetic_27a58:
    bx lr
.Lsoft_arithmetic_27a5a:
    clz r3, r0
.Lsoft_arithmetic_27a5e:
    lsls.w r1, r0, r3
.Lsoft_arithmetic_27a62:
    bne.n .Lsoft_arithmetic_27a66
.Lsoft_arithmetic_27a64:
    pop {r4, pc}
.Lsoft_arithmetic_27a66:
    subs r4, r4, r3
.Lsoft_arithmetic_27a68:
    subs r4, #23
.Lsoft_arithmetic_27a6a:
    lsls r0, r1, #21
.Lsoft_arithmetic_27a6c:
    lsrs r1, r1, #11
.Lsoft_arithmetic_27a6e:
    b.n .Lsoft_arithmetic_27a3a
.Lsoft_arithmetic_27a70:
    add.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_27a74:
    pop {r4, pc}
.Lsoft_arithmetic_27a76:
    asrs r1, r1, #1
.Lsoft_arithmetic_27a78:
    add.w r1, r1, r4, lsl #20
.Lsoft_arithmetic_27a7c:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_27a80:
    mov.w r0, r0, rrx
.Lsoft_arithmetic_27a84:
    b.n .Lsoft_arithmetic_27a8c
.Lsoft_arithmetic_27a86:
    mov.w r1, #0
.Lsoft_arithmetic_27a8a:
    bx lr
.Lsoft_arithmetic_27a8c:
    lsls r2, r1, #1
.Lsoft_arithmetic_27a8e:
    it eq
.Lsoft_arithmetic_27a90:
    tsteq r0, r0
.Lsoft_arithmetic_27a92:
    beq.n .Lsoft_arithmetic_27a86
.Lsoft_arithmetic_27a94:
    cmp.w r2, #2097152
.Lsoft_arithmetic_27a98:
    it cs
.Lsoft_arithmetic_27a9a:
    bxcs lr
.Lsoft_arithmetic_27a9c:
    and.w r1, r1, #2147483648
.Lsoft_arithmetic_27aa0:
    mov.w r0, #0
.Lsoft_arithmetic_27aa4:
    bx lr
.Lsoft_arithmetic_27aa6:
    teq lr, r4, lsl #21
.Lsoft_arithmetic_27aaa:
    beq.n .Lsoft_arithmetic_27abc
.Lsoft_arithmetic_27aac:
    tst.w r1, lr, lsr #1
.Lsoft_arithmetic_27ab0:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_27ab4:
    itt eq
.Lsoft_arithmetic_27ab6:
    moveq r1, #0
.Lsoft_arithmetic_27ab8:
    moveq r0, #0
.Lsoft_arithmetic_27aba:
    bx lr
.Lsoft_arithmetic_27abc:
    ldmia.w sp!, {r4, lr}
.Lsoft_arithmetic_27ac0:
    push {r4, r5, r6, lr}
.Lsoft_arithmetic_27ac2:
    bl softdouble_exception_core
.Lsoft_arithmetic_27ac6:
    nop
.balign 4
.Lsoft_arithmetic_27ac8:
    .word 0x3ef6df40
.Lsoft_arithmetic_27acc:
    eor.w r1, r3, #2147483648
.Lsoft_arithmetic_27ad0:
    mov r0, r2
.Lsoft_arithmetic_27ad2:
    bx lr
.balign 4
.Lsoft_arithmetic_27ad4:
    .word 0xffe00000

/* Shared software binary64 exception selector.
 * LR locates a typed selector word and four caller continuation slots.
 * Caller has saved r4/r5/r6/LR; this helper consumes that frame.
 */
.section .text.softfloat_softdouble_exception,"ax",%progbits
.balign 4
.global softdouble_exception_core
.type softdouble_exception_core, %function
.thumb_func
softdouble_exception_core:
.Lsoft_exception_276ac:
    add.w lr, lr, #2
.Lsoft_exception_276b0:
    bic.w lr, lr, #3
.Lsoft_exception_276b4:
    ldr.w r6, [lr], #4
.Lsoft_exception_276b8:
    tst r6, r6
.Lsoft_exception_276ba:
    bmi.n .Lsoft_exception_276c8
.Lsoft_exception_276bc:
    cmp r2, #1
.Lsoft_exception_276be:
    adc.w r5, r3, r3
.Lsoft_exception_276c2:
    cmn.w r5, #2097152
.Lsoft_exception_276c6:
    bhi.n .Lsoft_exception_276ee
.Lsoft_exception_276c8:
    cmp r0, #1
.Lsoft_exception_276ca:
    adc.w ip, r1, r1
.Lsoft_exception_276ce:
    cmn.w ip, #2097152
.Lsoft_exception_276d2:
    bhi.n .Lsoft_exception_276ee
.Lsoft_exception_276d4:
    bne.n .Lsoft_exception_276f4
.Lsoft_exception_276d6:
    mov.w ip, r1, lsr #31
.Lsoft_exception_276da:
    cmn.w r5, #2097152
.Lsoft_exception_276de:
    add.w ip, ip, ip, lsl #1
.Lsoft_exception_276e2:
    add.w ip, ip, #2
.Lsoft_exception_276e6:
    it eq
.Lsoft_exception_276e8:
    adceq.w ip, ip, r3, lsr #31
.Lsoft_exception_276ec:
    b.n .Lsoft_exception_276f8
.Lsoft_exception_276ee:
    mov.w ip, #8
.Lsoft_exception_276f2:
    b.n .Lsoft_exception_276f8
.Lsoft_exception_276f4:
    mov.w ip, r3, lsr #31
.Lsoft_exception_276f8:
    add.w r5, ip, ip, lsl #1
.Lsoft_exception_276fc:
    lsr.w r6, r6, r5
.Lsoft_exception_27700:
    and.w r6, r6, #7
.Lsoft_exception_27704:
    subs.w ip, r6, #4
.Lsoft_exception_27708:
    bcs.n .Lsoft_exception_27718
.Lsoft_exception_2770a:
    add.w ip, lr, r6, lsl #2
.Lsoft_exception_2770e:
    ldmia.w sp!, {r4, r5, r6, lr}
.Lsoft_exception_27712:
    orr.w ip, ip, #1
.Lsoft_exception_27716:
    bx ip
.Lsoft_exception_27718:
    tbb [pc, ip]
.Lsoft_exception_2771c:
    .byte 9, 11, 2, 2
.Lsoft_exception_27720:
    mov.w r1, #2147483648
.Lsoft_exception_27724:
    mov.w r0, #0
.Lsoft_exception_27728:
    sub.w r1, r1, #524288
.Lsoft_exception_2772c:
    pop {r4, r5, r6, pc}
.Lsoft_exception_2772e:
    mov r1, r3
.Lsoft_exception_27730:
    mov r0, r2
.Lsoft_exception_27732:
    cmp r0, #1
.Lsoft_exception_27734:
    adcs.w r4, r1, r1
.Lsoft_exception_27738:
    it ne
.Lsoft_exception_2773a:
    rsbsne r4, r4, #2097152
.Lsoft_exception_2773e:
    itt hi
.Lsoft_exception_27740:
    movhi r0, #0
.Lsoft_exception_27742:
    andhi.w r1, r1, #2147483648
.Lsoft_exception_27746:
    pop {r4, r5, r6, pc}
.size softdouble_exception_core, .-softdouble_exception_core
.global softdouble_saturate_core
.type softdouble_saturate_core, %function
.thumb_func
softdouble_saturate_core:
.Lsoft_exception_27748:
    lsrs r0, r1, #32
.Lsoft_exception_2774a:
    sub.w r1, r0, #2097152
.Lsoft_exception_2774e:
    mov.w r1, r1, rrx
.Lsoft_exception_27752:
    bx lr
.size softdouble_saturate_core, .-softdouble_saturate_core

/* Pointer-input wrappers for the extended arithmetic cores. */
.section .text.softfloat_extended_divide_entry,"ax",%progbits
.balign 2
.global extended_divide_entry
.type extended_divide_entry, %function
.thumb_func
extended_divide_entry:
    stmdb sp!, {r7, r10, r11, lr}
    mov r10, r3
    mov r11, r2
    push {r4, r5, r6}
    ldmia.w r1, {r3, r4, r5}
    ldmia r0, {r0, r1, r2}
    bics.w ip, r1, r0, lsl #1
    it mi
    bicsmi.w ip, r4, r3, lsl #1
    bpl .Ldivide_entry_return
    bl extended_divide_core
    bl extended_round_core
.Ldivide_entry_return:
    pop {r4, r5, r6, r7}
    ldmia.w sp!, {r10, r11, pc}
.size extended_divide_entry, .-extended_divide_entry

.section .text.softfloat_extended_multiply_entry,"ax",%progbits
.global extended_multiply_entry
.type extended_multiply_entry, %function
.thumb_func
extended_multiply_entry:
    stmdb sp!, {r7, r10, r11, lr}
    mov r10, r3
    mov r11, r2
    push {r4, r5, r6}
    ldmia.w r1, {r3, r4, r5}
    ldmia r0, {r0, r1, r2}
    tst.w r0, #0x40000000
    it eq
    tsteq.w r3, #0x40000000
    bne .Lmultiply_entry_return
    bl extended_multiply_core
    bl extended_round_core
.Lmultiply_entry_return:
    pop {r4, r5, r6, r7}
    ldmia.w sp!, {r10, r11, pc}
.size extended_multiply_entry, .-extended_multiply_entry

/* Extended-precision division core.
 * Inputs r0:r1:r2 and r3:r4:r5 are extended triples.
 * Outputs r0 sign, r3 exponent, r1:r2 quotient, r6 guard/sticky.
 * Non-AAPCS core; only the C wrapper below may call it.
 */
.section .text.softfloat_extended_divide_core,"ax",%progbits
.balign 4
.global extended_divide_core
.type extended_divide_core, %function
.thumb_func
extended_divide_core:
    bic.w ip, r0, #4278190080
    eor.w r0, r0, r3
    bic.w r7, r3, #4278190080
    sub.w r3, ip, r7
    add.w r3, r3, #16128
    and.w r0, r0, #2147483648
    add.w r3, r3, #255
    stmdb sp!, {r7, r8, fp, lr}
    mov.w fp, r5, lsr #16
    push {r0, r3}
    mov.w r3, r4, lsr #16
    bic.w lr, r5, fp, lsl #16
    bic.w r8, r4, r3, lsl #16
    adr.w r6, .Ldivision_reciprocal - 128
    add.w r7, r6, r3, lsr #8
    lsrs r1, r1, #1
    ldrb r6, [r7, #0]
    movs.w r2, r2, rrx
    mla r7, r3, r6, r6
    rsb r7, r7, #8388608
    mul.w r6, r7, r6
    mov.w r7, r4, lsr #13
    mov.w r6, r6, lsr #19
    add.w r6, r6, #2
    mla ip, r7, r6, r6
    rsb ip, ip, #536870912
    mov.w r7, ip, lsr #16
    bic.w ip, ip, r7, lsl #16
    mul.w r4, r7, r6
    mul.w r5, ip, r6
    add.w r6, r4, r5, lsr #16
    mov.w r6, r6, lsr #6
    ite cc
    movcc r0, #0
    movcs.w r0, #2147483648
    mov.w r7, r1, lsr #15
    mul.w ip, r6, r7
    mov.w ip, ip, lsr #16
    mul.w r7, ip, fp
    subs r2, r2, r7
    mul.w r7, r3, ip
    sbc.w r1, r1, r7
    mul.w r7, ip, lr
    subs.w r0, r0, r7, lsl #16
    sbcs.w r2, r2, r7, lsr #16
    mul.w r7, ip, r8
    ite cc
    subcc.w r2, r2, r7, lsl #16
    subscs.w r2, r2, r7, lsl #16
    sbc.w r1, r1, r7, lsr #16
    mov.w r4, ip, lsl #16
    mov.w r7, r1, lsr #2
    mul.w ip, r6, r7
    mov.w ip, ip, lsr #16
    mul.w r7, ip, fp
    subs.w r0, r0, r7, lsl #19
    sbcs.w r2, r2, r7, lsr #13
    mul.w r7, r3, ip
    ite cc
    subcc.w r2, r2, r7, lsl #19
    subscs.w r2, r2, r7, lsl #19
    sbc.w r1, r1, r7, lsr #13
    mul.w r7, ip, lr
    subs.w r0, r0, r7, lsl #3
    sbcs.w r2, r2, r7, lsr #29
    mul.w r7, ip, r8
    ite cc
    subcc.w r2, r2, r7, lsl #3
    subscs.w r2, r2, r7, lsl #3
    sbc.w r1, r1, r7, lsr #29
    ldr r7, [sp, #12]
    mov.w r1, r1, lsl #26
    orr.w r1, r1, r2, lsr #6
    add.w r4, r4, ip, lsl #3
    mov.w r7, r1, lsr #15
    mov.w r2, r2, lsl #26
    mul.w ip, r6, r7
    orr.w r2, r2, r0, lsr #6
    mov.w r0, r0, lsl #26
    mov.w ip, ip, lsr #16
    mul.w r7, ip, fp
    subs r2, r2, r7
    mul.w r7, r3, ip
    sbc.w r1, r1, r7
    mul.w r7, ip, lr
    subs.w r0, r0, r7, lsl #16
    sbcs.w r2, r2, r7, lsr #16
    mul.w r7, ip, r8
    ite cc
    subcc.w r2, r2, r7, lsl #16
    subscs.w r2, r2, r7, lsl #16
    sbc.w r1, r1, r7, lsr #16
    mov.w r5, ip, lsl #22
    mov.w r7, r1, lsr #2
    add.w r4, r4, ip, lsr #10
    mul.w ip, r6, r7
    mov.w ip, ip, lsr #16
    mul.w r7, ip, fp
    subs.w r0, r0, r7, lsl #19
    sbcs.w r2, r2, r7, lsr #13
    mul.w r7, r3, ip
    ite cc
    subcc.w r2, r2, r7, lsl #19
    subscs.w r2, r2, r7, lsl #19
    sbc.w r1, r1, r7, lsr #13
    mul.w r7, ip, lr
    subs.w r0, r0, r7, lsl #3
    sbcs.w r2, r2, r7, lsr #29
    mul.w r7, ip, r8
    ite cc
    subcc.w r2, r2, r7, lsl #3
    subscs.w r2, r2, r7, lsl #3
    sbc.w r1, r1, r7, lsr #29
    adds.w r5, r5, ip, lsl #9
    mov.w r1, r1, lsl #26
    orr.w r1, r1, r2, lsr #6
    mov.w r2, r2, lsl #26
    mov.w r7, r1, lsr #15
    orr.w r2, r2, r0, lsr #6
    mul.w ip, r6, r7
    adc.w r4, r4, #0
    mov.w r0, r0, lsl #26
    mov.w ip, ip, lsr #16
    mul.w r7, ip, fp
    subs r2, r2, r7
    mul.w r7, r3, ip
    sbc.w r1, r1, r7
    mul.w r7, ip, lr
    subs.w r0, r0, r7, lsl #16
    sbcs.w r2, r2, r7, lsr #16
    mul.w r7, ip, r8
    ite cc
    subcc.w r2, r2, r7, lsl #16
    subscs.w r2, r2, r7, lsl #16
    sbc.w r1, r1, r7, lsr #16
    adds.w r5, r5, ip, lsr #4
    mov.w r1, r1, lsl #14
    orr.w r1, r1, r2, lsr #18
    mov.w r2, r2, lsl #14
    orr.w r2, r2, r0, lsr #18
    mov.w r0, r0, lsl #14
    mov.w r6, ip, lsl #28
    adc.w r4, r4, #0
    orr.w r3, r8, r3, lsl #16
    orr.w r8, lr, fp, lsl #16
    mov.w lr, #0
    subs.w ip, r2, r8
    sbcs.w r7, r1, r3
    itt cs
    movcs r2, ip
    movcs r1, r7
    adc.w lr, lr, lr
    mov.w fp, #0
    adds r0, r0, r0
    adcs r2, r2
    adcs r1, r1
    adc.w fp, fp, fp
    subs.w ip, r2, r8
    sbcs.w r7, r1, r3
    sbcs.w fp, fp, #0
    itt cs
    movcs r2, ip
    movcs r1, r7
    adc.w lr, lr, lr
    mov.w fp, #0
    adds r0, r0, r0
    adcs r2, r2
    adcs r1, r1
    adc.w fp, fp, fp
    subs.w ip, r2, r8
    sbcs.w r7, r1, r3
    sbcs.w fp, fp, #0
    itt cs
    movcs r2, ip
    movcs r1, r7
    adc.w lr, lr, lr
    orrs.w r7, r1, r2
    it ne
    orrne.w r6, r6, #1
    adds.w r6, r6, lr, lsl #28
    adcs.w r2, r5, #0
    adcs.w r1, r4, #0
    bpl.n .Ldivision_21592
    ldmia.w sp!, {r0, r3, r7, r8}
    ldmia.w sp!, {fp, pc}
.Ldivision_21592:
    ldmia.w sp!, {r0, r3, r7, r8}
    ldmia.w sp!, {fp, lr}
    adds r6, r6, r6
    adcs r2, r2
    adc.w r1, r1, r1
    sub.w r3, r3, #1
    bx lr
.size extended_divide_core, . - extended_divide_core
.balign 4
.Ldivision_reciprocal:
    .byte 128, 128, 127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 118, 117, 116, 115
    .byte 114, 113, 113, 112, 111, 110, 110, 109, 108, 108, 107, 106, 106, 105, 104, 104
    .byte 103, 102, 102, 101, 100, 100, 99, 99, 98, 97, 97, 96, 96, 95, 95, 94
    .byte 94, 93, 93, 92, 92, 91, 91, 90, 90, 89, 89, 88, 88, 87, 87, 86
    .byte 86, 85, 85, 85, 84, 84, 83, 83, 82, 82, 82, 81, 81, 80, 80, 80
    .byte 79, 79, 79, 78, 78, 77, 77, 77, 76, 76, 76, 75, 75, 75, 74, 74
    .byte 74, 73, 73, 73, 72, 72, 72, 71, 71, 71, 71, 70, 70, 70, 69, 69
    .byte 69, 68, 68, 68, 68, 67, 67, 67, 67, 66, 66, 66, 66, 65, 65, 65

/* Extended-precision multiply core using the same triple/guard register ABI. */
.section .text.softfloat_extended_multiply_core,"ax",%progbits
.balign 4
.global extended_multiply_core
.type extended_multiply_core, %function
.thumb_func
extended_multiply_core:
    bic.w r7, r0, #4278190080
    eor.w r0, r0, r3
    bic.w ip, r3, #4278190080
    add.w r3, r7, ip
    sub.w r3, r3, #16128
    and.w r0, r0, #2147483648
    sub.w r3, r3, #254
    teq r2, #0
    beq.w .Lmultiply_218e2
    teq r5, #0
    beq.w .Lmultiply_21872
    stmdb sp!, {r0, r8, fp, lr}
    mov.w r0, r1, lsr #16
    mov.w r6, r4, lsr #16
    bic.w r8, r1, r0, lsl #16
    mul.w ip, r0, r6
    bic.w r7, r4, r6, lsl #16
    mul.w r6, r8, r6
    mul.w r8, r7, r8
    mul.w r7, r0, r7
    adds.w r8, r8, r6, lsl #16
    adc.w ip, ip, r6, lsr #16
    mov.w fp, r2, lsr #16
    adds.w r8, r8, r7, lsl #16
    mov.w r6, r5, lsr #16
    adc.w r0, ip, r7, lsr #16
    bic.w lr, r2, fp, lsl #16
    mul.w ip, fp, r6
    bic.w r7, r5, r6, lsl #16
    mul.w r6, lr, r6
    mul.w lr, r7, lr
    mul.w r7, fp, r7
    adds.w lr, lr, r6, lsl #16
    adc.w ip, ip, r6, lsr #16
    mov.w r6, #0
    adds.w lr, lr, r7, lsl #16
    adc.w fp, ip, r7, lsr #16
    adds.w r8, r8, fp
    adc.w r0, r0, #0
    adds.w fp, r8, lr
    adcs.w r8, r8, r0
    adc.w r0, r0, #0
    subs r7, r1, r2
    mov.w r1, #0
    itt cc
    mvncc r1, r1
    subcc r6, r4, r5
    ite ne
    subsne.w ip, r5, r4
    moveq r1, #0
    itt cc
    mvncc r1, r1
    subcc r6, r6, r7
    mov.w r4, r7, lsr #16
    orr.w lr, lr, lr, lsl #2
    bic.w r5, r7, r4, lsl #16
    mov.w r7, ip, lsr #16
    mla r2, r4, r7, r6
    bic.w ip, ip, r7, lsl #16
    mul.w r7, r5, r7
    mul.w r6, ip, r5
    mul.w ip, r4, ip
    adds.w r6, r6, r7, lsl #16
    adc.w r2, r2, r7, lsr #16
    adds.w r6, r6, ip, lsl #16
    adc.w r2, r2, ip, lsr #16
    adds.w r6, fp, r6
    adcs.w r2, r8, r2
    adcs r1, r0
    orr.w r6, r6, lr, lsr #2
    bpl.n .Lmultiply_21862
    ldmia.w sp!, {r0, r8, fp, pc}
.Lmultiply_21862:
    adds r6, r6, r6
    adcs r2, r2
    adc.w r1, r1, r1
    sub.w r3, r3, #1
    ldmia.w sp!, {r0, r8, fp, pc}
.Lmultiply_21872:
    mov.w r5, r4, lsr #16
    mov.w r7, r1, lsr #16
    bic.w r6, r4, r5, lsl #16
    bic.w ip, r1, r7, lsl #16
    mul.w r4, r5, r7
    mul.w r7, r6, r7
    mul.w r1, ip, r6
    mul.w ip, r5, ip
    adds.w r1, r1, r7, lsl #16
    adc.w r4, r4, r7, lsr #16
    mov.w r7, r2, lsr #16
    adds.w r1, r1, ip, lsl #16
    adc.w r4, r4, ip, lsr #16
    bic.w ip, r2, r7, lsl #16
    mul.w r2, r5, r7
    mul.w r7, r6, r7
    mul.w r6, ip, r6
    mul.w ip, r5, ip
    adds.w r6, r6, r7, lsl #16
    adc.w r2, r2, r7, lsr #16
    adds.w r6, r6, ip, lsl #16
    adc.w r5, r2, ip, lsr #16
    adds r2, r5, r1
    adcs.w r1, r4, #0
    it mi
    bxmi lr
    adds r6, r6, r6
    adcs r2, r2
    adc.w r1, r1, r1
    sub.w r3, r3, #1
    bx lr
.Lmultiply_218e2:
    teq r5, #0
    beq.n .Lmultiply_21958
    mov.w r2, r1, lsr #16
    mov.w r7, r4, lsr #16
    bic.w r6, r1, r2, lsl #16
    bic.w ip, r4, r7, lsl #16
    mul.w r1, r2, r7
    mul.w r7, r6, r7
    mul.w r4, ip, r6
    mul.w ip, r2, ip
    adds.w r4, r4, r7, lsl #16
    adc.w r1, r1, r7, lsr #16
    mov.w r7, r5, lsr #16
    adds.w r4, r4, ip, lsl #16
    adc.w r1, r1, ip, lsr #16
    bic.w ip, r5, r7, lsl #16
    mul.w r5, r2, r7
    mul.w r7, r6, r7
    mul.w r6, ip, r6
    mul.w ip, r2, ip
    adds.w r6, r6, r7, lsl #16
    adc.w r5, r5, r7, lsr #16
    adds.w r6, r6, ip, lsl #16
    adc.w r2, r5, ip, lsr #16
    adds r2, r2, r4
    adcs.w r1, r1, #0
    it mi
    bxmi lr
    adds r6, r6, r6
    adcs r2, r2
    adc.w r1, r1, r1
    sub.w r3, r3, #1
    bx lr
.Lmultiply_21958:
    mov.w r5, r4, lsr #16
    mov.w r7, r1, lsr #16
    bic.w r6, r4, r5, lsl #16
    bic.w ip, r1, r7, lsl #16
    mul.w r1, r5, r7
    mul.w r7, r6, r7
    mul.w r2, ip, r6
    mov.w r6, #0
    mul.w ip, r5, ip
    adds.w r2, r2, r7, lsl #16
    adc.w r1, r1, r7, lsr #16
    adds.w r2, r2, ip, lsl #16
    adcs.w r1, r1, ip, lsr #16
    it mi
    bxmi lr
    adds r2, r2, r2
    adc.w r1, r1, r1
    sub.w r3, r3, #1
    bx lr
.size extended_multiply_core, . - extended_multiply_core

/* Extended-precision rounding core: r10 mode, r11 rounding, r6 guard. */
.section .text.softfloat_extended_round_core,"ax",%progbits
.balign 4
.global extended_round_core
.type extended_round_core, %function
.thumb_func
extended_round_core:
    cmp r3, #0
    mov ip, r3
    bmi.n .Lround_21690
.Lround_2162e:
    lsls r7, r6, #1
    beq.n .Lround_2168a
.Lround_21632:
    push {r2, r3, r4, lr}
    mov.w r7, r7, rrx
    push {r0, r1}
    mov.w r0, #16
    mov.w r1, #16
    cmp.w sl, #0
    beq.n .Lround_21656
    cmp.w ip, #0
    itt le
    orrle.w r0, r0, #8
    orrle.w r1, r0, #8
.Lround_21656:
    mov.w r0, #0
    pop {r0, r1, r2, r3}
    ldmia.w sp!, {r4, lr}
    add.w fp, fp, #1
    cmp.w fp, #1
    it eq
    movseq.w r7, r7, lsr #32
    ittt cs
    addscs.w r2, r2, #1
    addscs.w r1, r1, #1
    movcs.w r1, #2147483648
    adc.w r3, r3, #0
.Lround_21680:
    and.w r0, r0, #2147483648
    orr.w r0, r3, r0
    bx lr
.Lround_2168a:
    bcc.n .Lround_21680
    lsrs r7, r2, #1
    b.n .Lround_21632
.Lround_21690:
    orr.w r6, r6, r6, lsl #16
    cmn.w r3, #64
    mov.w r6, r6, lsr #16
    ble.n .Lround_216de
    cmn.w r3, #32
    itttt le
    orrle r6, r2
    movle r2, r1
    movle r1, #0
    addle r3, #32
    negs r3, r3
    beq.n .Lround_2162e
    rsb r7, r3, #32
    orr.w r6, r6, r6, lsl #16
    lsl.w r7, r2, r7
    mov.w r6, r6, lsr #16
    orr.w r6, r6, r7
    rsb r7, r3, #32
    lsr.w r2, r2, r3
    lsl.w r7, r1, r7
    lsr.w r1, r1, r3
    mov.w r3, #0
    orr.w r2, r2, r7
    b.n .Lround_2162e
.Lround_216de:
    orr.w r6, r6, r2
    orr.w r6, r6, r6, lsl #16
    mov.w r6, r6, lsr #16
    orr.w r6, r6, r1
    itt lt
    orrlt.w r6, r6, r6, lsl #16
    lsrlt r6, r6, #16
    mov.w r3, #0
    mov.w r2, #0
    mov.w r1, #0
    b.n .Lround_2162e
.size extended_round_core, . - extended_round_core
