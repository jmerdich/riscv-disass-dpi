#include "gtest/gtest.h"
#include "test_common.h"

#include <llvm-c/Disassembler.h>
#include <llvm-c/Target.h>

// Memory leaks are unimportant here.
#define ASSERT_DISASS(inst, disass) \
    ASSERT_STREQ(rv_disass(inst), disass)

TEST(Rv32Basic, Core) {
    ASSERT_DISASS(0x00000093, "addi    ra, zero, 0");
    ASSERT_DISASS(0xFFF00093, "addi    ra, zero, -1");
}

TEST(LiterallyEverything, DISABLED_DontCrash) {
    for (uint32_t inst = 0; ; inst++) {
        char* str = rv_disass(inst);
        rv_free(str);

        if (inst == 0xFFFFFFFF) {
            break;
        }
    }
}

LLVMDisasmContextRef GetLlvmDisassembler() {
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllDisassemblers();

    LLVMDisasmContextRef ref = LLVMCreateDisasm(
        "riscv64", // TripleName
        NULL,
        0,
        NULL,
        NULL
    );
    EXPECT_NE(ref, nullptr);
    return ref;
}

std::string GetDisassFromLlvm(LLVMDisasmContextRef dis, uint32_t inst) {
    char buffer[128] = {};
    LLVMDisasmInstruction(dis, reinterpret_cast<uint8_t*>(&inst), sizeof(inst), 0,
                          buffer, sizeof(buffer));

    return std::string(buffer);
}

// Strips
std::string normalize_ws(std::string input) {
    std::string out;
    out.reserve(input.size());
    bool wasspace = false;
    bool wasstart = true;
    for (char& c : input) {
        if (isspace(c)) {
            wasspace = true;
            continue;
        }
        if (wasspace && !wasstart) {
            out += ' ';
        }
        wasspace = false;
        wasstart = false;
        out += c;
    }
    return out;
}

bool ShouldSkip(const std::string& llvms) {
    std::string blacklist[] = {
        // Any 64 bit insts

        // Unimplemented exts
        "fence.i",
        "csr",

        // I'm dealing with you later
        "unknown",
        "fence ",
    };
    for (const auto& tv : blacklist) {
        if (llvms.find(tv) != std::string::npos) {
            return true;
        }
    }
    return false;
}

TEST(LiterallyEverything, CompareToLlvm) {
    LLVMDisasmContextRef dis = GetLlvmDisassembler();
    rv_set_option("UsePsuedoInsts", true);
    for (uint32_t inst = 0; ; inst++) {
        char* rv_inst_raw = rv_disass(inst);
        std::string rv_inst = normalize_ws(rv_inst_raw);
        std::string llvm_inst = normalize_ws(GetDisassFromLlvm(dis, inst));

        if (llvm_inst == "") {
            llvm_inst = "unknown";
        }

        if (!ShouldSkip(llvm_inst)) {
            ASSERT_EQ(rv_inst, llvm_inst) << "when disassembling " << inst;
        }

        rv_free(rv_inst_raw);
        if ((inst % 0x10000) == 0) {
            printf("...0x%08x\n", inst);
        }

        if (inst == 0xFFFFFFFF) {
            break;
        }
    }
}

TEST(LiterallyEverything, DISABLED_NoOverlap) {
    for (uint32_t inst = 0; ; inst++) {
        int matches = 0;
        for (uint32_t info_idx = 0; info_idx < UncompressedInstsSize; info_idx++) {
            const OpInfo* info = &UncompressedInsts[info_idx];
            if ((inst & info->searchMask) == info->searchVal) {
                matches += 1;
            }
        }

        ASSERT_LT(matches, 2) << "Instruction " << inst << " matches more than one op!";

        if ((inst % 0x100000) == 0) {
            printf("...0x%08x\n", inst);
        }

        if (inst == 0xFFFFFFFF) {
            break;
        }
    }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}