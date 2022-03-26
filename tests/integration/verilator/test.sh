#!/bin/bash

TESTDIR=$(realpath $(dirname -- "$BASH_SOURCE[0]"))
WORKDIR=$TESTDIR/workdir
ROOT=$TESTDIR/../../..

DESIGN_NAME=simple_nop_check

#if [ -z "$XILINX_VIVADO" ]
#then
#    echo "Vivado not set up. Is it installed and did you source settings64.sh?"
#    exit 1
#fi

mkdir -p $WORKDIR
cd $WORKDIR

verilator -Wall --cc --exe $ROOT/src/rv_disass.c $TESTDIR/main.cpp +incdir+$ROOT/src $TESTDIR/../$DESIGN_NAME.sv
make -C $WORKDIR/obj_dir -f V$DESIGN_NAME.mk
obj_dir/V$DESIGN_NAME

echo
