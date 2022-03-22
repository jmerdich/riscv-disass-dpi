project('riscv-disass-dpi', 'c',
  version : '0.1',
  default_options : ['warning_level=2', 'c_std=gnu99'])

dpi_lib = library('riscv-disass-dpi',
                  'src/rv_disass.h',
                  'src/rv_disass.c',
                  install : true)

subdir('tests')