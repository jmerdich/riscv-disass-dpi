project('riscv-disass-dpi', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14'])

dpi_lib = library('riscv-disass-dpi',
                  'src/lib.cpp',
                  install : true)

subdir('tests')