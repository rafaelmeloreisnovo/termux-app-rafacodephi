module rafaelia_ctrl_fpga(
    input  logic        clk_100m,
    input  logic        rst_n,
    input  logic        parity_opt_in,
    output logic [6:0]  dac_freq,
    output logic [6:0]  dac_weight,
    output logic        rollback_o
);
  localparam int DEPTH=1008;
  logic [41:0] bram [0:DEPTH-1];
  logic [9:0] idx;
  logic [41:0] w;

  // stride toroidal: gcd(7,1000)=1 => cobertura completa dos osciladores
  function automatic [9:0] stride7(input [9:0] cur);
    if (cur>=10'd1000) stride7 = (cur+10'd7)-10'd1000; else stride7 = cur+10'd7;
  endfunction

  always_ff @(posedge clk_100m or negedge rst_n) begin
    if(!rst_n) begin idx<=0; rollback_o<=0; end
    else begin
      w <= bram[idx];
      dac_freq   <= w[6:0];
      dac_weight <= w[13:7];
      rollback_o <= ~parity_opt_in; // falha na paridade óptica ativa rollback
      idx <= stride7(idx);
    end
  end
endmodule
