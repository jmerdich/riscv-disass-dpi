//  SPDX-FileCopyrightText: 2022 Jake Merdich <jake@merdich.com>
//  SPDX-License-Identifier: Unlicense

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#ifndef DPI_DLLESPEC
#ifdef _WIN32
#define DPI_DLLESPEC __declspec(dllexport)
#else
#define DPI_DLLESPEC
#endif
#endif

// Psuedoinst flags (per InstLayout)
#define PS_I_NOP  (1 << 0)
#define PS_I_MV   (1 << 1)
#define PS_I_NOT  (1 << 2)
#define PS_I_SEXT (1 << 3)
#define PS_I_SEQZ (1 << 4)

#define PS_R_NEG  (1 << 0)
#define PS_R_NEGW (1 << 1)
#define PS_R_SNEZ (1 << 2) /* achoo! */
#define PS_R_SLTZ (1 << 3)
#define PS_R_SGTZ (1 << 4)

#define PS_B_BLEZ  (1 << 0) /* bless you! */
#define PS_B_BGTZ  (1 << 1)
#define PS_B_ANY_Z (1 << 2)

#define SHIFT_OP   0
#define SHIFT_RD   7
#define SHIFT_F3   12
#define SHIFT_I20  12
#define SHIFT_RS1  15
#define SHIFT_RS2  20
#define SHIFT_SHMT 20
#define SHIFT_I12  20
#define SHIFT_SUC  20
#define SHIFT_PRED 24
#define SHIFT_F7   25
#define SHIFT_F6   26
#define SHIFT_FM   28

#define MASK_OP   0x0000007F
#define MASK_RD   0x00000F80
#define MASK_F3   0x00007000
#define MASK_RS1  0x000F8000
#define MASK_RS2  0x01F00000
#define MASK_F7   0xFE000000
#define MASK_F6   0xFC000000
#define MASK_SHMT 0x03F00000
#define MASK_I12  0xFFF00000
#define MASK_I20  0xFFFFF000
#define MASK_SUC  0x00F00000
#define MASK_PRED 0x0F000000
#define MASK_FM   0xF0000000
#define MASK_ALL  0xFFFFFFFF

#define ENC_OP(x) ((__extension__ (x)) << SHIFT_OP)
#define ENC_F3(x) ((__extension__ (x)) << SHIFT_F3)
#define ENC_F7(x) ((__extension__ (x)) << SHIFT_F7)
#define ENC_F6(x) ((__extension__ (x)) << SHIFT_F6)

#define DEC_OP(x)   (((x) & MASK_OP)   >> SHIFT_OP)
#define DEC_F3(x)   (((x) & MASK_F3)   >> SHIFT_F3)
#define DEC_F7(x)   (((x) & MASK_F7)   >> SHIFT_F7)
#define DEC_RD(x)   (((x) & MASK_RD)   >> SHIFT_RD)
#define DEC_RS1(x)  (((x) & MASK_RS1)  >> SHIFT_RS1)
#define DEC_RS2(x)  (((x) & MASK_RS2)  >> SHIFT_RS2)
#define DEC_SHMT(x) (((x) & MASK_SHMT) >> SHIFT_SHMT)
#define DEC_I12(x)  (((x) & MASK_I12)  >> SHIFT_I12)
#define DEC_I20(x)  (((x) & MASK_I20)  >> SHIFT_I20)
#define DEC_PRED(x) (((x) & MASK_PRED)   >> SHIFT_PRED)
#define DEC_SUC(x)  (((x) & MASK_SUC)   >> SHIFT_SUC)
#define DEC_FM(x)   (((x) & MASK_FM)   >> SHIFT_FM)

#define MAKE_SEXT_BITS(inst, bits) (((int32_t)((inst) & 0x80000000)) >> (32 - bits))

typedef struct {
    bool  UsePsuedoInsts; // Emits known pseudo-opcodes instead of raw insts.
    bool  NoAbiNames;     // Always use register numbers rather than names
    bool  NotThreadSafe;  // Disassembled outputs are freed on the next call
                          // (horrible ugly nasty hack because some implementations don't
                          // give you back the same string for free'ing)
} Context;
Context g_context = {
    false,
    false,
#ifdef VIVADO
    true,
#else
    false,
#endif
};


// Begin test interface
typedef enum {
    InstLayout_R,
    InstLayout_R_shamt5,
    InstLayout_R_shamt6,
    InstLayout_I,
    InstLayout_I_jump,
    InstLayout_I_load,
    InstLayout_I_fence,
    InstLayout_I_shift,
    InstLayout_S,
    InstLayout_B,
    InstLayout_U,
    InstLayout_J,
    InstLayout_None,
} InstLayout;

typedef struct {
    const char  name[8];
    uint32_t    searchVal;
    uint32_t    searchMask;
    InstLayout  layout;
    uint32_t    pseudoInstFlags;
} OpInfo;
DPI_DLLESPEC const OpInfo UncompressedInsts[];
DPI_DLLESPEC const uint32_t UncompressedInstsSize;
// End test interface

DPI_DLLESPEC const OpInfo UncompressedInsts[] = {
    // =========================================
    // RV32I Base Instruction Set
    {"lui",                   ENC_OP(0b0110111),           MASK_OP, InstLayout_U, 0},
    {"auipc",                 ENC_OP(0b0010111),           MASK_OP, InstLayout_U, 0},
    {"jal",                   ENC_OP(0b1101111),           MASK_OP, InstLayout_J, 0},
    {"jalr",  ENC_F3(0b000) | ENC_OP(0b1100111), MASK_F3 | MASK_OP, InstLayout_I_jump, 0},
    {"beq",   ENC_F3(0b000) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, PS_B_ANY_Z},
    {"bne",   ENC_F3(0b001) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, PS_B_ANY_Z},
    {"blt",   ENC_F3(0b100) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, PS_B_ANY_Z | PS_B_BGTZ},
    {"bge",   ENC_F3(0b101) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, PS_B_ANY_Z | PS_B_BLEZ},
    {"bltu",  ENC_F3(0b110) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, 0},
    {"bgeu",  ENC_F3(0b111) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, 0},
    {"lb",    ENC_F3(0b000) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, 0},
    {"lh",    ENC_F3(0b001) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, 0},
    {"lw",    ENC_F3(0b010) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, 0},
    {"lbu",   ENC_F3(0b100) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, 0},
    {"lhu",   ENC_F3(0b101) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, 0},
    {"sb",    ENC_F3(0b000) | ENC_OP(0b0100011), MASK_F3 | MASK_OP, InstLayout_S, 0},
    {"sh",    ENC_F3(0b001) | ENC_OP(0b0100011), MASK_F3 | MASK_OP, InstLayout_S, 0},
    {"sw",    ENC_F3(0b010) | ENC_OP(0b0100011), MASK_F3 | MASK_OP, InstLayout_S, 0},
    {"addi",  ENC_F3(0b000) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, PS_I_NOP | PS_I_MV},
    {"slti",  ENC_F3(0b010) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, 0},
    {"sltiu", ENC_F3(0b011) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, PS_I_SEQZ},
    {"xori",  ENC_F3(0b100) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, PS_I_NOT},
    {"ori",   ENC_F3(0b110) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, 0},
    {"andi",  ENC_F3(0b111) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, 0},
    // shift imm insts skipped in favor of 64-bit variants
    {"add",   ENC_F7(0b0000000) | ENC_F3(0b000) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"sub",   ENC_F7(0b0100000) | ENC_F3(0b000) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, PS_R_NEG},
    {"sll",   ENC_F7(0b0000000) | ENC_F3(0b001) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"slt",   ENC_F7(0b0000000) | ENC_F3(0b010) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, PS_R_SLTZ | PS_R_SGTZ},
    {"sltu",  ENC_F7(0b0000000) | ENC_F3(0b011) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, PS_R_SNEZ},
    {"xor",   ENC_F7(0b0000000) | ENC_F3(0b100) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"srl",   ENC_F7(0b0000000) | ENC_F3(0b101) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"sra",   ENC_F7(0b0100000) | ENC_F3(0b101) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"or",    ENC_F7(0b0000000) | ENC_F3(0b110) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"and",   ENC_F7(0b0000000) | ENC_F3(0b111) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"fence",                     ENC_F3(0b000) | ENC_OP(0b0001111),           MASK_F3 | MASK_OP, InstLayout_I_fence, 0},
    {"ecall",                                           0x000000073,                    MASK_ALL, InstLayout_None, 0},
    {"ebreak",                                          0x000100073,                    MASK_ALL, InstLayout_None, 0},
    // =========================================
    // RV64I Base Instruction Set
    {"lwu",                       ENC_F3(0b110) | ENC_OP(0b0000011),           MASK_F3 | MASK_OP, InstLayout_I_load, 0},
    {"ld",                        ENC_F3(0b011) | ENC_OP(0b0000011),           MASK_F3 | MASK_OP, InstLayout_I_load, 0},
    {"sd",                        ENC_F3(0b011) | ENC_OP(0b0100011),           MASK_F3 | MASK_OP, InstLayout_S, 0},
    {"slli",   ENC_F6(0b000000) | ENC_F3(0b001) | ENC_OP(0b0010011), MASK_F6 | MASK_F3 | MASK_OP, InstLayout_I_shift, 0},
    {"srli",   ENC_F6(0b000000) | ENC_F3(0b101) | ENC_OP(0b0010011), MASK_F6 | MASK_F3 | MASK_OP, InstLayout_I_shift, 0},
    {"srai",   ENC_F6(0b010000) | ENC_F3(0b101) | ENC_OP(0b0010011), MASK_F6 | MASK_F3 | MASK_OP, InstLayout_I_shift, 0},
    {"addiw",                     ENC_F3(0b000) | ENC_OP(0b0011011),           MASK_F3 | MASK_OP, InstLayout_I, PS_I_SEXT},
    {"slliw", ENC_F7(0b0000000) | ENC_F3(0b001) | ENC_OP(0b0011011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_I_shift, 0},
    {"srliw", ENC_F7(0b0000000) | ENC_F3(0b101) | ENC_OP(0b0011011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_I_shift, 0},
    {"sraiw", ENC_F7(0b0100000) | ENC_F3(0b101) | ENC_OP(0b0011011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_I_shift, 0},
    {"addw",  ENC_F7(0b0000000) | ENC_F3(0b000) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"subw",  ENC_F7(0b0100000) | ENC_F3(0b000) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, PS_R_NEGW},
    {"sllw",  ENC_F7(0b0000000) | ENC_F3(0b001) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"srlw",  ENC_F7(0b0000000) | ENC_F3(0b101) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
    {"sraw",  ENC_F7(0b0100000) | ENC_F3(0b101) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, 0},
};

DPI_DLLESPEC const uint32_t UncompressedInstsSize = sizeof(UncompressedInsts)/sizeof(UncompressedInsts[0]);

static const char* get_reg_name(uint8_t reg) {
    const char* RegNames[] = {
       "zero",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",  "x9",
        "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19",
        "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29",
        "x30", "x31"
    };

    return RegNames[reg % 32];
}

static const char* get_abi_name(uint8_t reg) {
    if (g_context.NoAbiNames) {
        return get_reg_name(reg);
    }
    const char* RegNames[] = {
        "zero", "ra", "sp", "gp", "tp", "t0",  "t1",  "t2", "s0", "s1",
          "a0", "a1", "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3",
          "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4",
          "t5", "t6"
    };
    // s0 = fp?

    return RegNames[reg % 32];
}

static char* rv_disass_i(unsigned int inst, const OpInfo* info) {
    char output[64] = {0};

    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t imm = DEC_I12(inst);

    // Do sign extension of immediate
    imm |= MAKE_SEXT_BITS(inst, 12);

    if (g_context.UsePsuedoInsts && info->pseudoInstFlags) {
        if ((info->pseudoInstFlags & PS_I_NOP) && 
            (rd == 0) && (rs1 == 0) && (imm == 0)) {
            return strdup("nop");
        }
        if ((info->pseudoInstFlags & PS_I_MV) && 
            (imm == 0)) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %s", "mv",  get_abi_name(rd), get_abi_name(rs1));
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
        if ((info->pseudoInstFlags & PS_I_NOT) && 
            (imm == (uint32_t)-1)) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %s", "not",  get_abi_name(rd), get_abi_name(rs1));
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
        if ((info->pseudoInstFlags & PS_I_SEXT) && 
            (imm == 0)) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %s", "sext.w",  get_abi_name(rd), get_abi_name(rs1));
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
        if ((info->pseudoInstFlags & PS_I_SEQZ) && 
            (imm == 1)) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %s", "seqz",  get_abi_name(rd), get_abi_name(rs1));
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
    }

    int size = snprintf(output, sizeof(output), "%-7s %s, %s, %d", info->name,  get_abi_name(rd), get_abi_name(rs1), (int32_t)imm);
    assert(size > 0 && (size_t)size < sizeof(output));

    char* str = strdup(output);
    return str;
}

static char* rv_disass_i_shift(unsigned int inst, const OpInfo* info) {
    char output[64] = {0};

    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t imm = DEC_SHMT(inst);

    int size = snprintf(output, sizeof(output), "%-7s %s, %s, %d", info->name,  get_abi_name(rd), get_abi_name(rs1), (int32_t)imm);
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

static char* rv_disass_i_jump(unsigned int inst, const OpInfo* info) {
    char output[64] = {0};

    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t imm = DEC_I12(inst);

    // Do sign extension of immediate
    imm |= MAKE_SEXT_BITS(inst, 12);

    int size = 0;
    if (rd == 0 && imm == 0 && rs1 == 1 && g_context.UsePsuedoInsts) {
        return strdup("ret");
    } else if (rd == 0 && imm == 0 && g_context.UsePsuedoInsts) {
        size = snprintf(output, sizeof(output), "%-7s %s", "jr",  get_abi_name(rs1));
    } else if (rd == 0 && g_context.UsePsuedoInsts) {
        size = snprintf(output, sizeof(output), "%-7s %d(%s)", "jr", (int32_t)imm, get_abi_name(rs1));
    } else if (rd == 1 && imm == 0 && g_context.UsePsuedoInsts) {
        size = snprintf(output, sizeof(output), "%-7s %s", info->name,  get_abi_name(rs1));
    } else if (rd == 1 && g_context.UsePsuedoInsts) {
        size = snprintf(output, sizeof(output), "%-7s %d(%s)", info->name, (int32_t)imm, get_abi_name(rs1));
    } else if (imm == 0 && g_context.UsePsuedoInsts) {
        size = snprintf(output, sizeof(output), "%-7s %s, %s", info->name,  get_abi_name(rd), get_abi_name(rs1));
    } else {
        size = snprintf(output, sizeof(output), "%-7s %s, %d(%s)", info->name,  get_abi_name(rd), (int32_t)imm, get_abi_name(rs1));
    }
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

static char* rv_disass_i_load(unsigned int inst, const OpInfo* info) {
    char output[64] = {0};

    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t imm = DEC_I12(inst);

    // Do sign extension of immediate
    imm |= MAKE_SEXT_BITS(inst, 12);

    int size = snprintf(output, sizeof(output), "%-7s %s, %d(%s)", info->name,  get_abi_name(rd), (int32_t)imm, get_abi_name(rs1));
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

static char* rv_disass_i_fence(unsigned int inst, const OpInfo* info) {
    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t fm = DEC_FM(inst);

    if (g_context.UsePsuedoInsts && inst == 0x0ff0000f) {
        return strdup("fence");
    }
    if (inst == 0x8330000f) {
        if (g_context.UsePsuedoInsts) {
            return strdup("fence.tso");
        } else {
            return strdup("fence.tso rw, rw");
        }
    }

    // These are reserved insts.
    if (rd != 0 || rs1 != 0 || fm != 0) {
        return strdup("unknown");
    }
    const char* bitnames = "iorw";

    char predbuf[5] = {0};
    uint32_t pred = DEC_PRED(inst);
    for (int i = 0; i < 4; i++) {
        if (pred & (1 << (3-i))) {
            predbuf[strlen(predbuf)] = bitnames[i];
        }
    }

    char sucbuf[5] = {0};
    uint32_t suc = DEC_SUC(inst);
    for (int i = 0; i < 4; i++) {
        if (suc & (1 << (3-i))) {
            sucbuf[strlen(sucbuf)] = bitnames[i];
        }
    }

    const char* predstr = (predbuf[0] != 0) ? predbuf : "unknown";
    const char* sucstr = (sucbuf[0] != 0) ? sucbuf : "unknown";

    char output[64] = {0};
    int size = snprintf(output, sizeof(output), "%-7s %s, %s", info->name, predstr, sucstr);
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

static char* rv_disass_u(unsigned int inst, const OpInfo* info) {
    char output[64] = {0};

    uint32_t rd = DEC_RD(inst);
    uint32_t imm = DEC_I20(inst);

    int size = snprintf(output, sizeof(output), "%-7s %s, %d", info->name,  get_abi_name(rd), (int32_t)imm);
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

static char* rv_disass_b(unsigned int inst, const OpInfo* info) {
    char output[64] = {0};

    uint32_t rs1 = DEC_RS1(inst);
    uint32_t rs2 = DEC_RS2(inst);

    // Throw these bits at a dartboard and see where they land....
    // Exactly what are they smoking at Berkeley?
    uint32_t imm = 0;
    imm |= (inst & 0x80) << 4;
    imm |= (inst & 0xF00) >> 7;
    imm |= (inst & 0x7E000000) >> 20;
    imm |= MAKE_SEXT_BITS(inst, 13);

    if (g_context.UsePsuedoInsts) {
        if (info->pseudoInstFlags & PS_B_BLEZ && rs1 == 0) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %d", "blez",  get_abi_name(rs2), (int32_t)imm);
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
        if (info->pseudoInstFlags & PS_B_ANY_Z && rs2 == 0) {
            char newinst[8] = {0};
            strcpy(newinst, info->name);
            strcat(newinst, "z");
            int size = snprintf(output, sizeof(output), "%-7s %s, %d", newinst,  get_abi_name(rs1), (int32_t)imm);
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
        if (info->pseudoInstFlags & PS_B_BGTZ && rs1 == 0) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %d", "bgtz",  get_abi_name(rs2), (int32_t)imm);
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
    }

    int size = snprintf(output, sizeof(output), "%-7s %s, %s, %d", info->name,  get_abi_name(rs1), get_abi_name(rs2), (int32_t)imm);
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

static char* rv_disass_r(unsigned int inst, const OpInfo* info) {
    char output[64] = {0};

    uint32_t rd  = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t rs2 = DEC_RS2(inst);

    if (g_context.UsePsuedoInsts && info->pseudoInstFlags)
    {
        if (info->pseudoInstFlags & PS_R_NEG && rs1 == 0) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %s", "neg",  get_abi_name(rd), get_abi_name(rs2));
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
        if (info->pseudoInstFlags & PS_R_NEGW && rs1 == 0) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %s", "negw",  get_abi_name(rd), get_abi_name(rs2));
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
        if (info->pseudoInstFlags & PS_R_SNEZ && rs1 == 0) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %s", "snez",  get_abi_name(rd), get_abi_name(rs2));
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
        if (info->pseudoInstFlags & PS_R_SLTZ && rs2 == 0) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %s", "sltz",  get_abi_name(rd), get_abi_name(rs1));
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }
        if (info->pseudoInstFlags & PS_R_SGTZ && rs1 == 0) {
            int size = snprintf(output, sizeof(output), "%-7s %s, %s", "sgtz",  get_abi_name(rd), get_abi_name(rs2));
            assert(size > 0 && (size_t)size < sizeof(output));
            return strdup(output);
        }

    }

    int size = snprintf(output, sizeof(output), "%-7s %s, %s, %s", info->name,  get_abi_name(rd), get_abi_name(rs1), get_abi_name(rs2));
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

static char* rv_disass_s(unsigned int inst, const OpInfo* info) {
    char output[64] = {0};

    uint32_t imm = MAKE_SEXT_BITS(inst, 12) | (DEC_F7(inst) << 5) | DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t rs2 = DEC_RS2(inst);

    int size = snprintf(output, sizeof(output), "%-7s %s, %d(%s)", info->name,  get_abi_name(rs2), (int32_t)imm, get_abi_name(rs1));
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

static char* rv_disass_j(unsigned int inst, const OpInfo* info) {
    char output[64] = {0};

    uint32_t rd  = DEC_RD(inst);
    uint32_t raw_imm = DEC_I20(inst);

    // The next major rev of riscv should put an lfsr here because clearly this isn't convoluted enough.
    uint32_t imm = 0;
    imm |= (raw_imm & 0xFF) << 12;
    imm |= ((raw_imm >> 8) & 0x1) << 11;
    imm |= ((raw_imm >> 9) & 0x3FF) << 1;
    imm |= ((raw_imm >> 19) & 0x1) << 20;
    imm |= MAKE_SEXT_BITS(inst, 21);

    if (g_context.UsePsuedoInsts && rd == 0) {
        int size = snprintf(output, sizeof(output), "%-7s %d", "j",  (int32_t)imm);
        assert(size > 0 && (size_t)size < sizeof(output));
    } else if (g_context.UsePsuedoInsts && rd == 1) {
        int size = snprintf(output, sizeof(output), "%-7s %d", "jal",  (int32_t)imm);
        assert(size > 0 && (size_t)size < sizeof(output));
    } else {
        int size = snprintf(output, sizeof(output), "%-7s %s, %d", info->name,  get_abi_name(rd), (int32_t)imm);
        assert(size > 0 && (size_t)size < sizeof(output));
    }

    return strdup(output);
}

static char* rv_disass_none(unsigned int _dummy, const OpInfo* info) {
    (void)_dummy;
    return strdup(info->name);
}

static char* rv_disass_impl(unsigned int inst) {
    // Start with a naive search
    // We can choose better algorithms (bsearch, jump table) later.
    for (uint32_t i = 0; i < UncompressedInstsSize; i++) {
        const OpInfo* info = &UncompressedInsts[i];
        if ((inst & info->searchMask) == info->searchVal) {
            switch (info->layout) {
                case InstLayout_B:
                    return rv_disass_b(inst, info);
                    break;
                case InstLayout_I:
                    return rv_disass_i(inst, info);
                    break;
                case InstLayout_I_jump:
                    return rv_disass_i_jump(inst, info);
                    break;
                case InstLayout_I_load:
                    return rv_disass_i_load(inst, info);
                    break;
                case InstLayout_I_shift:
                    return rv_disass_i_shift(inst, info);
                    break;
                case InstLayout_I_fence:
                    return rv_disass_i_fence(inst, info);
                    break;
                case InstLayout_U:
                    return rv_disass_u(inst, info);
                    break;
                case InstLayout_S:
                    return rv_disass_s(inst, info);
                    break;
                case InstLayout_R:
                    return rv_disass_r(inst, info);
                    break;
                case InstLayout_J:
                    return rv_disass_j(inst, info);
                    break;
                case InstLayout_None:
                    return rv_disass_none(inst, info);
                    break;
                default:
                    // not implemented :(
                    break;
            }
            break;
        }
    }
    return strdup("unknown");
}

DPI_DLLESPEC const char* rv_disass(int raw_inst) {
    uint32_t inst = (uint32_t)raw_inst;
    static char* last_char = NULL;
    char* disass = rv_disass_impl(inst);

    if (g_context.NotThreadSafe) {
        if (last_char != NULL) {
            free(last_char);
        }
        last_char = disass;
    }

    return disass;
}

DPI_DLLESPEC void rv_free(char* str) {
    if (!g_context.NotThreadSafe) {
        // If "not thread safe", we'll auto free on next disass.
        free(str);
    }
}

DPI_DLLESPEC void rv_set_option(const char* str, bool enabled) {
    if (strcmp(str, "UsePsuedoInsts") == 0) {
        g_context.UsePsuedoInsts = enabled;
    }
    if (strcmp(str, "NoAbiNames") == 0) {
        g_context.NoAbiNames = enabled;
    }
    if (strcmp(str, "NotThreadSafe") == 0) {
        g_context.NotThreadSafe = enabled;
    }
}

DPI_DLLESPEC void rv_reset_options() {
    memset(&g_context, 0, sizeof(g_context));
#ifdef VIVADO
    g_context.NotThreadSafe = true;
#endif
}
