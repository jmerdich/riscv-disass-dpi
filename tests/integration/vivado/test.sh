#!/bin/bash

TESTDIR=$(realpath $(dirname -- "$BASH_SOURCE[0]"))
WORKDIR=$TESTDIR/workdir
ROOT=$TESTDIR/../../..

if [ -z "$XILINX_VIVADO" ]
then
    echo "Vivado not set up. Is it installed and did you source settings64.sh?"
    exit 1
fi

mkdir -p $WORKDIR
cd $WORKDIR

xsc --gcc_compile_options=-DVIVADO $ROOT/src/rv_disass.c
xelab -vlog $TESTDIR/../simple_nop_check.sv -sv -i $ROOT/src -sv_lib dpi -R

echo