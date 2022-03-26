#include "Vsimple_nop_check.h"
#include "verilated.h"

int main(int argc, char** argv, char** env) {
    (void)env;
#if 0
    VerilatedContext* contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    Vsimple_nop_check* top = new Vsimple_nop_check{};
    while (!contextp->gotFinish()) { top->eval(); }
    delete top;
    delete contextp;
#else
    Verilated::commandArgs(argc, argv);
    Vsimple_nop_check* top = new Vsimple_nop_check{};
    while (!Verilated::gotFinish()) { top->eval(); }
    delete top;
#endif
    return 0;
}