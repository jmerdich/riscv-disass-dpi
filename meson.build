project('riscv-disass-dpi', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c99'])

dpi_lib = library('riscv-disass-dpi',
                  'src/lib.cpp',
                  install : true)

subdir('tests')