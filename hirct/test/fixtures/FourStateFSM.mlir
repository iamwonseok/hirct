module {
hw.module @FourStateFSM(in %clk : !seq.clock, in %rst : i1,
                         in %start : i1, in %ack : i1,
                         out busy : i1, out done : i1) {
  %c0_i2 = hw.constant 0 : i2
  %c1_i2 = hw.constant 1 : i2
  %c2_i2 = hw.constant 2 : i2
  %c3_i2 = hw.constant 3 : i2
  %true = hw.constant true
  %state = seq.compreg %next_state, %clk reset %rst, %c0_i2 : i2
  %is_idle = comb.icmp eq %state, %c0_i2 : i2
  %is_running = comb.icmp eq %state, %c1_i2 : i2
  %is_waiting = comb.icmp eq %state, %c2_i2 : i2
  %is_done = comb.icmp eq %state, %c3_i2 : i2
  %idle_next = comb.mux %start, %c1_i2, %c0_i2 : i2
  %waiting_next = comb.mux %ack, %c3_i2, %c2_i2 : i2
  %ns3 = comb.mux %is_done, %c0_i2, %state : i2
  %ns2 = comb.mux %is_waiting, %waiting_next, %ns3 : i2
  %ns1 = comb.mux %is_running, %c2_i2, %ns2 : i2
  %next_state = comb.mux %is_idle, %idle_next, %ns1 : i2
  %c1_i8 = hw.constant 1 : i8
  %c0_i8 = hw.constant 0 : i8
  %cnt = seq.compreg %cnt_next_final, %clk reset %rst, %c0_i8 : i8
  %cnt_inc = comb.add %cnt, %c1_i8 : i8
  %cnt_next = comb.mux %is_running, %cnt_inc, %cnt : i8
  %cnt_next_final = comb.mux %is_idle, %c0_i8, %cnt_next : i8
  %not_idle = comb.xor %is_idle, %true : i1
  hw.output %not_idle, %is_done : i1, i1
}
}
