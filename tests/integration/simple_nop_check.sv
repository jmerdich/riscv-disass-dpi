`include "rv_disass.svi"

module simple_nop_check();

string s;
integer i = 32'h00000093;

initial begin
s = rv_disass(i);
$display(s);
$finish();
end


endmodule