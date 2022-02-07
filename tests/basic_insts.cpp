#include "gtest/gtest.h"
#include "test_common.h"

// Memory leaks are unimportant here.
#define ASSERT_DISASS(inst, disass) \
    ASSERT_STREQ(rv_disass(inst), disass)

TEST(Rv32Basic, Core) {
    ASSERT_DISASS(0x00000093, "addi x1, x0, 0");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}