module simple_nop_check();

import "DPI-C" function string rv_disass (input int inst);
import "DPI-C" function void rv_free (input string asm);


string s;
integer i, j;

initial begin
#1;
i = 32'h00000093;
j = 0;
end

always begin
#1
s = rv_disass(i);
$display(s);
rv_free(s);
j = j+1;
if (j == 500) begin
$finish();
end
end


endmodule