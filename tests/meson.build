
gtest = dependency('gtest')

test_exe1 = executable('riscv-disass-basic-tests',
               'basic_insts.cpp',
               dependencies:[gtest],
               link_with: [dpi_lib])
test('riscv-disass-basic-tests', test_exe1,
     protocol: 'gtest',
     is_parallel: true)