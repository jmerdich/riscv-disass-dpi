#include "gtest/gtest.h"
#include "test_common.h"

#include <atomic>
#include <deque>
#include <thread>

#include <llvm-c/Disassembler.h>
#include <llvm-c/Target.h>

constexpr uint64_t FullRangeStart = 0;
constexpr uint64_t FullRangeEnd   = 0x100000000;

class ExhaustiveThreadPool {
public:
    ExhaustiveThreadPool(uint64_t start, uint64_t end /* exclusive */, uint64_t numThreads=0)
     : m_min(start),
       m_max(end),
       m_numThreads(numThreads),
       m_chunksStarted(0),
       m_chunksDone(0),
       m_valid(true)
    {
        if (m_numThreads == 0) 
        {
            m_numThreads = std::thread::hardware_concurrency();
        }

        m_chunkSize = 8192; // Better heuristic probably needed. This makes sense for per-inst tests.
        m_numChunks = (m_max + m_chunkSize - 1 - m_min)  / m_chunkSize; // Divide, rounding up.
    }

    template <typename F>
    void run(F fn) {
        assert(m_chunksStarted == 0);
        for (uint32_t i = 0; i < m_numThreads; i++) {
            m_activeThreads.push_back(std::thread(&ExhaustiveThreadPool::worker<F>, this, fn));
        }

        float percentDone = 0.0;
        while (m_valid && m_chunksDone < m_numChunks) {
            float newPercentDone = static_cast<float>(m_chunksDone) * 100 / m_numChunks;
            if (newPercentDone - percentDone > 1.0) {
                printf("...%2.1f%%\n", newPercentDone);
                percentDone = newPercentDone; // Only store printed percent, that's what user cares about.
            }
        }

        for (auto& activeThread : m_activeThreads) {
            activeThread.join();
        }
    }
private:
    template <typename F>
    void worker(F fn) {
        while (m_valid) {
            uint64_t chunkIdx = m_chunksStarted.fetch_add(1);
            if (chunkIdx >= m_numChunks)
            {
                return;
            }
            uint64_t chunkMin = m_min + (chunkIdx * m_chunkSize);
            uint64_t chunkMax = std::min(m_min + ((chunkIdx + 1) * m_chunkSize), m_max);
            for (uint64_t trial = chunkMin; trial < chunkMax; trial++)
            {
                EXPECT_NO_THROW({
                    fn(trial);
                });
                if (testing::Test::HasFailure()) {
                    // In the future, we might make output more thread friendly but not today.
                    m_valid = false;
                }
                if (m_valid == false) {
                    return;
                }
            }
            m_chunksDone++;
        }
    }


    uint64_t m_min;
    uint64_t m_max;
    uint64_t m_numChunks;
    uint64_t m_numThreads;
    uint64_t m_chunkSize;
    std::deque<std::thread> m_activeThreads;
    std::atomic_uint64_t m_chunksStarted;
    std::atomic_uint64_t m_chunksDone;
    std::atomic_bool m_valid;
};


TEST(LiterallyEverything, DISABLED_DontCrash)
{
    ExhaustiveThreadPool threads(FullRangeStart, FullRangeEnd);
    threads.run([] (uint64_t inst) {
        char* str = rv_disass(inst);
        rv_free(str);
    });
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
    char buffer[128] = {0};
    LLVMDisasmInstruction(dis, reinterpret_cast<uint8_t*>(&inst), sizeof(inst), 0,
                          buffer, sizeof(buffer));

    return std::string(buffer);
}

// Strips preceding, trailing, and interior whitespace of 2+ chars
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
        "unimp", // It's literally an inst that is defined to not be implemented
        "uret",
        "sret",
        "dret",
        "mret",
        "wfi",
        "rdinstret",
        "rdcycle",
        "rdtime",
        "frrm",
        "fsrm",
        "frflags",
        "fsflags",

        // I'm dealing with you later
        "sfence",
    };
    for (const auto& tv : blacklist) {
        if (llvms.find(tv) != std::string::npos) {
            return true;
        }
    }
    return false;
}

uint32_t get_start_point() {
    uint32_t start = 0;
    if (getenv("START_INST")) {
        start = atoi(getenv("START_INST"));
    }
    return start;
}

TEST(LiterallyEverything, CompareToLlvm) {
    LLVMDisasmContextRef dis = GetLlvmDisassembler();
    rv_set_option("UsePseudoInsts", true);
    ExhaustiveThreadPool threads(get_start_point(), FullRangeEnd);
    threads.run([&](uint64_t inst) {
        std::string rv_inst = normalize_ws(rv_disass_str(inst));
        std::string llvm_inst = normalize_ws(GetDisassFromLlvm(dis, inst));

        if (llvm_inst == "") {
            llvm_inst = "unknown";
        }

        if (!ShouldSkip(llvm_inst) && rv_inst != llvm_inst) {
            ASSERT_EQ(rv_inst, llvm_inst) << "when disassembling " << inst;
        }
    });
}

TEST(LiterallyEverything, DISABLED_NoOverlap) {
    ExhaustiveThreadPool threads(FullRangeStart, FullRangeEnd);
    threads.run([&](uint64_t inst) {
        int matches = 0;
        for (uint32_t info_idx = 0; info_idx < UncompressedInstsSize; info_idx++) {
            const OpInfo* info = &UncompressedInsts[info_idx];
            if ((inst & info->searchMask) == info->searchVal) {
                matches += 1;
            }
        }

        ASSERT_LT(matches, 2) << "Instruction " << inst << " matches more than one op!";
    });
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}