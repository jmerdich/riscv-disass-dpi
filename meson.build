project('riscv-disass-dpi', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=gnu99'])

dpi_lib = library('riscv-disass-dpi',
                  'src/lib.c',
                  install : true)

subdir('tests')