
extern "C" {
extern char* rv_disass(unsigned int inst);
extern void rv_free(char* str);
void rv_set_option(const char* name, bool enabled);
void rv_reset_options();
// Implementation details
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
struct OpInfo {
    const char  name[8];
    uint32_t    searchVal;
    uint32_t    searchMask;
    InstLayout  layout;
    uint32_t    pseudoInstFlags;
};
extern const OpInfo UncompressedInsts[];
extern const uint32_t UncompressedInstsSize;
}

// Inline wrapper so we don't have to manually free
// C interfaces don't have destructors
std::string rv_disass_str(uint32_t inst) {
    char* cstr = rv_disass(inst);
    std::string out;
    if (cstr) {
        out = cstr;
    }
    rv_free(cstr);
    return out;
}
