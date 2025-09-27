
#include <linux/linkage.h>

#define STATE a0
#define DATA a1
#define BLOCKS a2
#define RATE a3
#define TABLE t3
#define COUNTER t4
#define OFFSET t5
#define ROTATION t6

.text
.option arch, +v, +zvkb


// SHA3 Transform for different digest sizes using RVV
// Supports SHA3-224, SHA3-256, SHA3-384, SHA3-512
// a0: pointer to Keccak state (25 * u64)
// a1: pointer to input message DATA
// a2: number of DATAs to process
// a3: digest size in bytes (e.g., 28, 32, 48, 64)

SYM_FUNC_START(sha3_riscv64_transform_zvkb)

load_state:
    vsetivli x0, 5, e64, m4, ta, ma

    mv      t0, STATE
    vle64.v v0,  (t0)          # A[0..4]
    addi    t0,  t0, 5*8
    vle64.v v4,  (t0)          # A[5..9]
    addi    t0,  t0, 5*8
    vle64.v v8,  (t0)          # A[10..14]
    addi    t0,  t0, 5*8
    vle64.v v12, (t0)          # A[15..19]
    addi    t0,  t0, 5*8
    vle64.v v16, (t0)          # A[20..24]

load_input:
    addi BLOCKS, BLOCKS, -1
    li     t0, 28
    beq    RATE, t0, .Lrate224
    li     t0, 32
    beq    RATE, t0, .Lrate256
    li     t0, 48
    beq    RATE, t0, .Lrate384
    li     t0, 64
    beq    RATE, t0, .Lrate512

.Lrate224: // 18 lanes
    vle64.v v20, (DATA)
    vxor.vv v0, v0, v20
    addi    DATA, DATA, 5*8

    vle64.v v20, (DATA)
    vxor.vv v4, v4, v20
    addi    DATA, DATA, 5*8

    vle64.v v20, (DATA)
    vxor.vv v8, v8, v20
    addi    DATA, DATA, 5*8

    vsetivli x0, 3, e64, m4, ta, ma
    vle64.v v20, (DATA)
    vxor.vv v12, v12, v20
    addi    DATA, DATA, 3*8

    vsetivli x0, 5, e64, m4, ta, ma 
    beq x0, x0, keccak_24_rounds 

.Lrate256: // 17 lanes
    vle64.v v20, (DATA)
    vxor.vv v0, v0, v20
    addi    DATA, DATA, 5*8

    vle64.v v20, (DATA)
    vxor.vv v4, v4, v20
    addi    DATA, DATA, 5*8

    vle64.v v20, (DATA)
    vxor.vv v8, v8, v20
    addi    DATA, DATA, 5*8

    vsetivli x0, 2, e64, m4, ta, ma
    vle64.v v20, (DATA)
    vxor.vv v12, v12, v20
    addi    DATA, DATA, 2*8

    vsetivli x0, 5, e64, m4, ta, ma
    beq x0, x0, keccak_24_rounds 


.Lrate384: // 13 lanes
    vle64.v v20, (DATA)
    vxor.vv v0, v0, v20
    addi    DATA, DATA, 5*8

    vle64.v v20, (DATA)
    vxor.vv v4, v4, v20
    addi    DATA, DATA, 5*8

    vsetivli x0, 3, e64, m4, ta, ma
    vle64.v v20, (DATA)
    vxor.vv v8, v8, v20
    addi    DATA, DATA, 3*8

    vsetivli x0, 5, e64, m4, ta, ma
    beq x0, x0, keccak_24_rounds 

.Lrate512: // 7 lanes
    vle64.v v20, (DATA)
    vxor.vv v0,  v0,  v20
    addi    DATA,  DATA, 5*8

    vsetivli x0, 4, e64, m4, ta, ma
    vle64.v v20, (DATA)
    vxor.vv v4,  v4,  v20
    addi    DATA,  DATA, 4*8

    vsetivli x0, 5, e64, m4, ta, ma

keccak_24_rounds:
    la      TABLE, RC64
    la      OFFSET, offset_AtoB
    la      ROTATION, rotation_B
    li      COUNTER, 24
    beq x0, x0, theta 

theta_load:
    vsetivli x0, 5, e64, m4, ta, ma
    mv      t0, STATE
    vle64.v v0,  (t0)
    addi    t0,  t0, 5*8
    vle64.v v4,  (t0)
    addi    t0,  t0, 5*8
    vle64.v v8,  (t0)
    addi    t0,  t0, 5*8
    vle64.v v12, (t0)
    addi    t0,  t0, 5*8
    vle64.v v16, (t0)


theta:
    // c_vector(v20)
    vsetivli x0, 5, e64, m4, ta, ma
    vxor.vv v20, v0, v4
    vxor.vv v24, v8, v12
    vxor.vv v28, v20, v16
    vxor.vv v20, v28, v24
    
    // count d
    vsetivli x0, 1, e64, m4, ta, ma  
    vslidedown.vi v24, v20, 4         // (c4: v24)
    vsetivli x0, 5, e64, m4, ta, ma  
    vslideup.vi v24, v20, 1           // (c40123: v24)
    vslidedown.vi v28, v20, 1         // (c1234:  v28)
    vslideup.vi v28, v20, 4           // (c12340: v28)
    vror.vi v28, v28, 64-1

    vxor.vv v20, v24, v28             // d

    vxor.vv v0, v0, v20
    vxor.vv v4, v4, v20
    vxor.vv v8, v8, v20
    vxor.vv v12, v12, v20
    vxor.vv v16, v16, v20

    // Store back to original state
    mv      t0, STATE
    vse64.v v0,  (t0)
    addi    t0,  t0, 5*8
    vse64.v v4,  (t0)
    addi    t0,  t0, 5*8
    vse64.v v8,  (t0)
    addi    t0,  t0, 5*8
    vse64.v v12, (t0)
    addi    t0,  t0, 5*8
    vse64.v v16, (t0)

rho_pi:
    // first 16
    vsetivli    x0, 16, e16, m2, ta, ma
    vle16.v    v16, (OFFSET)

    vsetivli    x0, 16, e64, m8, ta, ma
    vluxei16.v v0,  (a0), v16
    vle64.v    v8, (ROTATION)

    vrol.vv    v0,  v0, v8
    

    // last 9
    addi       t0, OFFSET, 32
    vsetivli    x0, 9, e16, m2, ta, ma
    vle16.v    v16, (t0)

    vsetivli    x0, 9, e64, m8, ta, ma
    vluxei16.v v8, (a0), v16
    addi     t0, ROTATION, 16*8
    vle64.v    v24, (t0)
    vrol.vv    v8,  v8,  v24
    addi       t0, STATE, 128
    
    // store last 9
    vse64.v    v8,  (t0)

    // store first 16
    vsetivli    x0, 16, e64, m8, ta, ma
    vse64.v    v0,  (STATE)

chi_iota:
    mv        t0, STATE
    li        t1, 5

1:
    vsetivli  x0, 5, e64, m4, ta, ma
    vle64.v   v0, (t0)   // row

    // row_xp1 = {r1,r2,r3,r4,r0}
    vsetivli zero, 4, e64, m4, ta, ma
    vslidedown.vi v4, v0, 1
    // row_xp2 = {r2,r3,r4,r0,r1}
    vsetivli zero, 3, e64, m4, ta, ma
    vslidedown.vi v8, v0, 2
    vsetivli  x0, 5, e64, m4, ta, ma
    vslideup.vi v4, v0, 4
    vslideup.vi v8, v0, 3

    # row ^= (~row_xp1) & row_xp2
    vandn.vv  v12, v8, v4
    vxor.vv   v0,  v0, v12

    # only rowId==0 do iota
    li        t2, 5
    bne       t1, t2, 2f
    ld        t2, (TABLE)
    vsetivli  x0, 1, e64, m4, tu, ma
    vxor.vx   v0,  v0, t2
    addi      TABLE, TABLE, 8
2:
    vsetivli  x0, 5, e64, m4, ta, ma
    vse64.v   v0, (t0)
    addi      t0, t0, 5*8
    addi      t1, t1, -1
    bnez      t1, 1b
    addi    COUNTER, COUNTER, -1
    bnez    COUNTER, theta_load   // jump if 24 round not end
    bnez    BLOCKS, load_state    // have DATA left
    
    mv  a0, BLOCKS
    ret

SYM_FUNC_END(sha3_riscv64_transform_zvkb)

	.section .rodata,"a",@progbits
offset_AtoB:
    .p2align 1
    .2byte 0, 48, 96, 144, 192
    .2byte 24, 72, 80, 128, 176
    .2byte 8, 56, 104, 152, 160
    .2byte 32, 40, 88, 136, 184
    .2byte 16, 64, 112, 120, 168

    .align 3
rotation_B:
    .8byte  0, 44, 43, 21, 14
    .8byte 28, 20,  3, 45, 61
    .8byte  1,  6, 25,  8, 18
    .8byte 27, 36, 10, 15, 56
    .8byte 62, 55, 39, 41,  2

    .p2align 1
idx_Cr:  .2byte 4,0,1,2,3
idx_Cl:  .2byte 1,2,3,4,0
idx_p1:  .2byte 1,2,3,4,0
idx_p2:  .2byte 2,3,4,0,1

	.align 3
RC64:
    .dword 0x0000000000000001
    .dword 0x0000000000008082
    .dword 0x800000000000808A
    .dword 0x8000000080008000
    .dword 0x000000000000808B
    .dword 0x0000000080000001
    .dword 0x8000000080008081
    .dword 0x8000000000008009
    .dword 0x000000000000008A
    .dword 0x0000000000000088
    .dword 0x0000000080008009
    .dword 0x000000008000000A
    .dword 0x000000008000808B
    .dword 0x800000000000008B
    .dword 0x8000000000008089
    .dword 0x8000000000008003
    .dword 0x8000000000008002
    .dword 0x8000000000000080
    .dword 0x000000000000800A
    .dword 0x800000008000000A
    .dword 0x8000000080008081
    .dword 0x8000000000008080
    .dword 0x0000000080000001
    .dword 0x8000000080008008