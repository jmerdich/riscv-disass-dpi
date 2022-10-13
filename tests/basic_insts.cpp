#include "gtest/gtest.h"
#include "test_common.h"

#define ASSERT_DISASS(inst, disass) \
    ASSERT_EQ(rv_disass_str(inst), disass)

TEST(Rv32Basic, Core) {
    rv_reset_options();
    ASSERT_DISASS(0x00000093, "addi    ra, zero, 0");
    ASSERT_DISASS(0xFFF00093, "addi    ra, zero, -1");
}


TEST(Rv32Basic, Fence) {
    rv_reset_options();
    ASSERT_DISASS(0x8f00000f, "unknown"); // TSO only valid with rw,rw
    ASSERT_DISASS(0xcf00000f, "unknown"); // unknown FM field
    ASSERT_DISASS(0x0f01000f, "unknown"); // unknown rs1 field
    ASSERT_DISASS(0x0f00040f, "unknown"); // unknown rd field
    ASSERT_DISASS(0x0840000f, "fence   i, o");
    ASSERT_DISASS(0x0c40000f, "fence   io, o");
    ASSERT_DISASS(0x0cf0000f, "fence   io, iorw");
    ASSERT_DISASS(0x0c00000f, "fence   io, unknown"); // 0 = unknown
    ASSERT_DISASS(0x0ff0000f, "fence   iorw, iorw"); // equivalent to iorw,iorw
    ASSERT_DISASS(0x8330000f, "fence.tso rw, rw");
    rv_set_option("UsePseudoInsts", true);
    ASSERT_DISASS(0x0ff0000f, "fence"); // equivalent to iorw,iorw
    ASSERT_DISASS(0x8330000f, "fence.tso");
}

TEST(Rv32Basic, Special) {
    rv_reset_options();
    // ASSERT_DISASS(0x10500073, "wfi");

}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}