
extern char* rv_disass(unsigned int inst);
extern void rv_free(char* str);


// Implementation details
enum InstLayout {
    InstLayout_R,
    InstLayout_R_shamt5,
    InstLayout_R_shamt6,
    InstLayout_I,
    InstLayout_I_fence,
    InstLayout_S,
    InstLayout_B,
    InstLayout_U,
    InstLayout_J,
    InstLayout_None,
};
struct OpInfo {
    const char* name;
    uint32_t    searchVal;
    uint32_t    searchMask;
    InstLayout  layout;
    void*       pseudoInsts;
};
extern const OpInfo UncompressedInsts[];
extern const uint32_t UncompressedInstsSize;