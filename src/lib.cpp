//  SPDX-FileCopyrightText: 2022 Jake Merdich <jake@merdich.com>
//  SPDX-License-Identifier: Unlicense

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

enum InstLayout {
    InstLayout_R,
    InstLayout_I,
    InstLayout_S,
    InstLayout_B,
    InstLayout_U,
    InstLayout_J,
};

#define SHIFT_OP  0
#define SHIFT_RD  7
#define SHIFT_F3  12
#define SHIFT_RS1 15
#define SHIFT_RS2 20
#define SHIFT_I12 20
#define SHIFT_F7  25

#define MASK_OP   0x0000007F
#define MASK_RD   0x00000F80
#define MASK_F3   0x00007000
#define MASK_RS1  0x000F8000
#define MASK_RS2  0x01F00000
#define MASK_F7   0xFE000000
#define MASK_I12  0xFFF00000
#define MASK_ALL  0xFFFFFFFF

#define ENC_OP(x) ((x) << SHIFT_OP)
#define ENC_F3(x) ((x) << SHIFT_F3)
#define ENC_F7(x) ((x) << SHIFT_F7)

#define DEC_OP(x)  (((x) & MASK_OP) >> SHIFT_OP)
#define DEC_F3(x)  (((x) & MASK_F3) >> SHIFT_F3)
#define DEC_F7(x)  (((x) & MASK_F7) >> SHIFT_F7)
#define DEC_RD(x)  (((x) & MASK_RD) >> SHIFT_RD)
#define DEC_RS1(x) (((x) & MASK_RS1) >> SHIFT_RS1)
#define DEC_RS2(x) (((x) & MASK_RS2) >> SHIFT_RS2)
#define DEC_I12(x)  (((x) & MASK_I12) >> SHIFT_I12)


struct OpInfo {
    const char* name;
    uint32_t    searchVal;
    uint32_t    searchMask;
    InstLayout  layout;
    void*       pseudoInsts;
};

const OpInfo UncompressedInsts[] = {
    {"addi", ENC_F3(0b000) | ENC_OP(0b0010011), MASK_F3 | MASK_OP, InstLayout_I, nullptr}
};

const uint32_t UncompressedInstsSize = sizeof(UncompressedInsts)/sizeof(UncompressedInsts[0]);

const char* get_reg_name(uint8_t reg) {
    const char* RegNames[] = {
         "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",  "x9",
        "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19",
        "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29",
        "x30", "x31"
    };

    return RegNames[reg % 32];
}

char* rv_disass_i(unsigned int inst, const OpInfo* info) {
    char output[64] = {};

    uint32_t rd = DEC_RD(inst);
    uint32_t rs1 = DEC_RS1(inst);
    uint32_t imm = DEC_I12(inst);

    // TODO: sign extend immediate

    int size = snprintf(output, sizeof(output), "%-7s %s, %s, %d", info->name,  get_reg_name(rd), get_reg_name(rs1), (int32_t)imm);
    assert(size > 0 && (size_t)size < sizeof(output));

    return strdup(output);
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
