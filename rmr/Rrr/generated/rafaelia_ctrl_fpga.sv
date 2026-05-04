module rafaelia_ctrl_fpga (
    input  logic        clk_100m,
    input  logic        rst_n,
    input  logic        parity_opt_in,
    output logic [6:0]  dac_freq,
    output logic [6:0]  dac_weight,
    output logic [6:0]  dac_phase,
    output logic        rollback_o,
    output logic [9:0]  osc_idx_o
);
  localparam int DEPTH = 1008;
  localparam int PERIOD = 42;
  localparam int STRIDE = 7;

  logic [41:0] bram [0:DEPTH-1];
  logic [9:0]  osc_idx;
  logic [5:0]  cycle;
  logic [41:0] w;

  // BitRAF fields
  logic [6:0] f_q, w_q, p_q, crc7, load7, op7;

  // routing over 1000 active oscillators, leaves 8 control cells for parity/CRC/special points
  function automatic [9:0] next_idx(input [9:0] cur);
    automatic int tmp;
    begin
      tmp = cur + STRIDE;
      if (tmp >= 1000) tmp = tmp - 1000;
      next_idx = tmp[9:0];
    end
  endfunction

  always_ff @(posedge clk_100m or negedge rst_n) begin
    if (!rst_n) begin
      osc_idx     <= '0;
      cycle       <= '0;
      rollback_o  <= 1'b0;
      dac_freq    <= '0;
      dac_weight  <= '0;
      dac_phase   <= '0;
    end else begin
      w <= bram[osc_idx];
      f_q   <= w[6:0];
      w_q   <= w[13:7];
      p_q   <= w[20:14];
      crc7  <= w[27:21];
      load7 <= w[34:28];
      op7   <= w[41:35];

      dac_freq   <= f_q;
      dac_weight <= w_q;
      dac_phase  <= p_q;

      rollback_o <= ~parity_opt_in; // optical parity failure triggers rollback line

      osc_idx <= next_idx(osc_idx);
      cycle   <= (cycle == PERIOD-1) ? 0 : cycle + 1'b1;
    end
  end

  assign osc_idx_o = osc_idx;
endmodule
