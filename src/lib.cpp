//  SPDX-FileCopyrightText: 2022 Jake Merdich <jake@merdich.com>
//  SPDX-License-Identifier: Unlicense

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

enum InstLayout {
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
};

#define SHIFT_OP   0
#define SHIFT_RD   7
#define SHIFT_F3   12
#define SHIFT_I20  12
#define SHIFT_RS1  15
#define SHIFT_RS2  20
#define SHIFT_SHMT 20
#define SHIFT_I12  20
#define SHIFT_F7   25
#define SHIFT_F6   26

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
#define MASK_ALL  0xFFFFFFFF

#define ENC_OP(x) ((x) << SHIFT_OP)
#define ENC_F3(x) ((x) << SHIFT_F3)
#define ENC_F7(x) ((x) << SHIFT_F7)
#define ENC_F6(x) ((x) << SHIFT_F6)

#define DEC_OP(x)   (((x) & MASK_OP)   >> SHIFT_OP)
#define DEC_F3(x)   (((x) & MASK_F3)   >> SHIFT_F3)
#define DEC_F7(x)   (((x) & MASK_F7)   >> SHIFT_F7)
#define DEC_RD(x)   (((x) & MASK_RD)   >> SHIFT_RD)
#define DEC_RS1(x)  (((x) & MASK_RS1)  >> SHIFT_RS1)
#define DEC_RS2(x)  (((x) & MASK_RS2)  >> SHIFT_RS2)
#define DEC_SHMT(x) (((x) & MASK_SHMT) >> SHIFT_SHMT)
#define DEC_I12(x)  (((x) & MASK_I12)  >> SHIFT_I12)
#define DEC_I20(x)  (((x) & MASK_I20)  >> SHIFT_I20)

#define MAKE_SEXT_BITS(inst, bits) (((int32_t)((inst) & 0x80000000)) >> (32 - bits))

struct Context {
    bool  UsePsuedoInsts;
} g_context = {};

struct OpInfo {
    const char* name;
    uint32_t    searchVal;
    uint32_t    searchMask;
    InstLayout  layout;
    void*       pseudoInsts;
};

extern const OpInfo UncompressedInsts[] = {
    // =========================================
    // RV32I Base Instruction Set
    {"lui",                   ENC_OP(0b0110111),           MASK_OP, InstLayout_U, nullptr},
    {"auipc",                 ENC_OP(0b0010111),           MASK_OP, InstLayout_U, nullptr},
    {"jal",                   ENC_OP(0b1101111),           MASK_OP, InstLayout_J, nullptr},
    {"jalr",  ENC_F3(0b000) | ENC_OP(0b1100111), MASK_F3 | MASK_OP, InstLayout_I_jump, nullptr},
    {"beq",   ENC_F3(0b000) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, nullptr},
    {"bne",   ENC_F3(0b001) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, nullptr},
    {"blt",   ENC_F3(0b100) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, nullptr},
    {"bge",   ENC_F3(0b101) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, nullptr},
    {"bltu",  ENC_F3(0b110) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, nullptr},
    {"bgeu",  ENC_F3(0b111) | ENC_OP(0b1100011), MASK_F3 | MASK_OP, InstLayout_B, nullptr},
    {"lb",    ENC_F3(0b000) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, nullptr},
    {"lh",    ENC_F3(0b001) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, nullptr},
    {"lw",    ENC_F3(0b010) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, nullptr},
    {"lbu",   ENC_F3(0b100) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, nullptr},
    {"lhu",   ENC_F3(0b101) | ENC_OP(0b0000011), MASK_F3 | MASK_OP, InstLayout_I_load, nullptr},
    {"sb",    ENC_F3(0b000) | ENC_OP(0b0100011), MASK_F3 | MASK_OP, InstLayout_S, nullptr},
    {"sh",    ENC_F3(0b001) | ENC_OP(0b0100011), MASK_F3 | MASK_OP, InstLayout_S, nullptr},
    {"sw",    ENC_F3(0b010) | ENC_OP(0b0100011), MASK_F3 | MASK_OP, InstLayout_S, nullptr},
    {"addi",  ENC_F3(0b000) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, nullptr},
    {"slti",  ENC_F3(0b010) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, nullptr},
    {"sltiu", ENC_F3(0b011) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, nullptr},
    {"xori",  ENC_F3(0b100) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, nullptr},
    {"ori",   ENC_F3(0b110) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, nullptr},
    {"andi",  ENC_F3(0b111) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, nullptr},
    // shift imm insts skipped in favor of 64-bit variants
    {"add",   ENC_F7(0b0000000) | ENC_F3(0b000) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"sub",   ENC_F7(0b0100000) | ENC_F3(0b000) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"sll",   ENC_F7(0b0000000) | ENC_F3(0b001) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"slt",   ENC_F7(0b0000000) | ENC_F3(0b010) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"sltu",  ENC_F7(0b0000000) | ENC_F3(0b011) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"xor",   ENC_F7(0b0000000) | ENC_F3(0b100) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"srl",   ENC_F7(0b0000000) | ENC_F3(0b101) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"sra",   ENC_F7(0b0100000) | ENC_F3(0b101) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"or",    ENC_F7(0b0000000) | ENC_F3(0b110) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"and",   ENC_F7(0b0000000) | ENC_F3(0b111) | ENC_OP(0b0110011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"fence",                     ENC_F3(0b000) | ENC_OP(0b0001111),           MASK_F3 | MASK_OP, InstLayout_I_fence, nullptr},
    {"ecall",                                           0x000000073,                    MASK_ALL, InstLayout_None, nullptr},
    {"ebreak",                                          0x001000073,                    MASK_ALL, InstLayout_None, nullptr},
    // =========================================
    // RV64I Base Instruction Set
    {"lwu",                       ENC_F3(0b110) | ENC_OP(0b0000011),           MASK_F3 | MASK_OP, InstLayout_I_load, nullptr},
    {"ld",                        ENC_F3(0b011) | ENC_OP(0b0000011),           MASK_F3 | MASK_OP, InstLayout_I_load, nullptr},
    {"sd",                        ENC_F3(0b011) | ENC_OP(0b0100011),           MASK_F3 | MASK_OP, InstLayout_S, nullptr},
    {"slli",   ENC_F6(0b000000) | ENC_F3(0b001) | ENC_OP(0b0010011), MASK_F6 | MASK_F3 | MASK_OP, InstLayout_I_shift, nullptr},
    {"srli",   ENC_F6(0b000000) | ENC_F3(0b101) | ENC_OP(0b0010011), MASK_F6 | MASK_F3 | MASK_OP, InstLayout_I_shift, nullptr},
    {"srai",   ENC_F6(0b010000) | ENC_F3(0b101) | ENC_OP(0b0010011), MASK_F6 | MASK_F3 | MASK_OP, InstLayout_I_shift, nullptr},
    {"addi",                      ENC_F3(0b000) | ENC_OP(0b0010011),           MASK_F3 | MASK_OP, InstLayout_I, nullptr},
    {"slliw", ENC_F7(0b0000000) | ENC_F3(0b001) | ENC_OP(0b0011011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_I_shift, nullptr},
    {"srliw", ENC_F7(0b0000000) | ENC_F3(0b101) | ENC_OP(0b0011011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_I_shift, nullptr},
    {"sraiw", ENC_F7(0b0100000) | ENC_F3(0b101) | ENC_OP(0b0011011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_I_shift, nullptr},
    {"addw",  ENC_F7(0b0000000) | ENC_F3(0b000) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"subw",  ENC_F7(0b0100000) | ENC_F3(0b000) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"sllw",  ENC_F7(0b0000000) | ENC_F3(0b001) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"srlw",  ENC_F7(0b0000000) | ENC_F3(0b101) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
    {"sraw",  ENC_F7(0b0100000) | ENC_F3(0b101) | ENC_OP(0b0111011), MASK_F7 | MASK_F3 | MASK_OP, InstLayout_R, nullptr},
};

extern const uint32_t UncompressedInstsSize = sizeof(UncompressedInsts)/sizeof(UncompressedInsts[0]);

const char* get_reg_name(uint8_t reg) {
    const char* RegNames[] = {
       "zero",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",  "x9",
        "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19",
        "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29",
        "x30", "x31"
    };

    return RegNames[reg % 32];
}

const char* get_abi_name(uint8_t reg) {
    const char* RegNames[] = {
        "zero", "ra", "sp", "gp", "tp", "t0",  "t1",  "t2", "s0", "s1",
          "a0", "a1", "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3",
          "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4",
          "t5", "t6"
    };
    // s0 = fp?

    return RegNames[reg % 32];
}

char* rv_disass_i(unsigned int inst, const OpInfo* info) {
    char output[64] = {};

    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t imm = DEC_I12(inst);

    // Do sign extension of immediate
    imm |= MAKE_SEXT_BITS(inst, 12);

    int size = snprintf(output, sizeof(output), "%-7s %s, %s, %d", info->name,  get_reg_name(rd), get_reg_name(rs1), (int32_t)imm);
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

char* rv_disass_i_shift(unsigned int inst, const OpInfo* info) {
    char output[64] = {};

    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t imm = DEC_SHMT(inst);

    int size = snprintf(output, sizeof(output), "%-7s %s, %s, %d", info->name,  get_reg_name(rd), get_reg_name(rs1), (int32_t)imm);
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

char* rv_disass_i_jump(unsigned int inst, const OpInfo* info) {
    char output[64] = {};

    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t imm = DEC_I12(inst);

    // Do sign extension of immediate
    imm |= MAKE_SEXT_BITS(inst, 12);

    int size = 0;
    if (imm == 0 && g_context.UsePsuedoInsts) {
        size = snprintf(output, sizeof(output), "%-7s %s, %s", info->name,  get_abi_name(rd), get_abi_name(rs1));

    } else {
        size = snprintf(output, sizeof(output), "%-7s %s, %d(%s)", info->name,  get_abi_name(rd), (int32_t)imm, get_abi_name(rs1));
    }
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

char* rv_disass_i_load(unsigned int inst, const OpInfo* info) {
    char output[64] = {};

    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t imm = DEC_I12(inst);

    // Do sign extension of immediate
    imm |= MAKE_SEXT_BITS(inst, 12);

    int size = snprintf(output, sizeof(output), "%-7s %s, %d(%s)", info->name,  get_abi_name(rd), (int32_t)imm, get_abi_name(rs1));
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

char* rv_disass_i_fence(unsigned int inst, const OpInfo* info) {
    // TODO: what the heck are these arguments?
    return strdup("fence unknown, unknown");
}

char* rv_disass_u(unsigned int inst, const OpInfo* info) {
    char output[64] = {};

    uint32_t rd = DEC_RD(inst);
    uint32_t imm = DEC_I20(inst) << 12;

    int size = snprintf(output, sizeof(output), "%-7s %s, %d", info->name,  get_abi_name(rd), (int32_t)imm);
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

char* rv_disass_r(unsigned int inst, const OpInfo* info) {
    char output[64] = {};

    uint32_t rd  = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t rs2 = DEC_RS2(inst);

    int size = snprintf(output, sizeof(output), "%-7s %s, %s, %s", info->name,  get_abi_name(rd), get_abi_name(rs1), get_abi_name(rs2));
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

char* rv_disass_s(unsigned int inst, const OpInfo* info) {
    char output[64] = {};

    uint32_t imm = MAKE_SEXT_BITS(inst, 12) | (DEC_F7(inst) << 5) | DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t rs2 = DEC_RS2(inst);

    int size = snprintf(output, sizeof(output), "%-7s %s, %d(%s)", info->name,  get_abi_name(rs1), (int32_t)imm, get_abi_name(rs2));
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

char* rv_disass_j(unsigned int inst, const OpInfo* info) {
    char output[64] = {};

    uint32_t rd  = DEC_RD(inst);
    uint32_t raw_imm = DEC_I20(inst);

    // The next major rev of riscv should put an lfsr here because clearly this isn't convoluted enough.
    uint32_t imm = 0;
    imm |= (raw_imm & 0x7F) << 12;
    imm |= ((raw_imm >> 8) & 0x1) << 11;
    imm |= ((raw_imm >> 9) & 0x1FF) << 1;
    imm |= ((raw_imm >> 18) & 0x1) << 20;
    imm |= MAKE_SEXT_BITS(inst, 21);

    int size = snprintf(output, sizeof(output), "%-7s %s, %d", info->name,  get_abi_name(rd), (int32_t)imm);
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
}

char* rv_disass_none(unsigned int, const OpInfo* info) {
    return strdup(info->name);
}

char* rv_disass(unsigned int inst) {
    // Start with a naive search
    // We can choose better algorithms (bsearch, jump table) later.
    for (uint32_t i = 0; i < UncompressedInstsSize; i++) {
        const OpInfo* info = &UncompressedInsts[i];
        if ((inst & info->searchMask) == info->searchVal) {
            switch (info->layout) {
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

void rv_free(char* str) {
    free(str);
}

void rv_set_option(const char* str, bool enabled) {
    if (strcmp(str, "UsePsuedoInsts") == 0) {
        g_context.UsePsuedoInsts = enabled;
    }
}

void rv_reset_options() {
    g_context = {};
}
