
gtest = dependency('gtest')
llvm  = dependency('llvm', # for cross-check tests
                   include_type: 'system',
                   version: '>=10')

test_exe1 = executable('riscv-disass-basic-tests',
               'basic_insts.cpp',
               dependencies:[gtest],
               link_with: [dpi_lib])
test('riscv-disass-basic-tests', test_exe1,
     protocol: 'gtest',
     is_parallel: true)

test_exe2 = executable('riscv-disass-exhaustive-tests',
               'exhaustive.cpp',
               dependencies:[gtest, llvm],
               link_with: [dpi_lib])
test('riscv-disass-exhaustive-tests', test_exe2,
     protocol: 'gtest',
     is_parallel: true)