#include "gtest/gtest.h"
#include "test_common.h"

#define ASSERT_DISASS(inst, disass) \
    ASSERT_EQ(rv_disass_str(inst), disass)

TEST(Rv32Basic, Core) {
    ASSERT_DISASS(0x00000093, "addi    ra, zero, 0");
    ASSERT_DISASS(0xFFF00093, "addi    ra, zero, -1");
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}