module {
  hw.module @IcmpCeq(in %a : i8, in %b : i8, out y_ceq : i1, out y_cne : i1, out y_weq : i1, out y_wne : i1) {
    %0 = comb.icmp ceq %a, %b : i8
    %1 = comb.icmp cne %a, %b : i8
    %2 = comb.icmp weq %a, %b : i8
    %3 = comb.icmp wne %a, %b : i8
    hw.output %0, %1, %2, %3 : i1, i1, i1, i1
  }
}
