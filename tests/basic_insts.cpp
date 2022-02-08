#include "gtest/gtest.h"
#include "test_common.h"

// Memory leaks are unimportant here.
#define ASSERT_DISASS(inst, disass) \
    ASSERT_STREQ(rv_disass(inst), disass)

TEST(Rv32Basic, Core) {
    ASSERT_DISASS(0x00000093, "addi    x1, x0, 0");
    ASSERT_DISASS(0xFFF00093, "addi    x1, x0, -1");
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