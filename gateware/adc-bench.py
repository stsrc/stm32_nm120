#!/usr/bin/env python3
from adc import ADC
from amaranth.sim import Simulator, Tick
from amaranth import *

if __name__ == "__main__":
    dut = ADC(simulation=True)

    def process():
        yield dut.adc.eq(0)
        yield dut.over_range.eq(0)
        yield dut.dry.eq(0)
        yield dut.trig.eq(0)
        yield dut.limit.eq(5)
        for i in range(200):
            yield dut.trig.eq(1)
            yield Tick()
            while ((yield dut.encode[0]) == 0):
                yield Tick()
            yield dut.trig.eq(0)
            for i in range(5):
                yield Tick()
                yield dut.dry.eq(1)
                yield Tick()
                yield dut.dry.eq(0)
                yield dut.adc.eq(dut.adc + 1)
            yield Tick()
        for i in range(512):
            yield Tick()
      
    sim = Simulator(dut)
    sim.add_clock(1.0/200e6,)
    sim.add_sync_process(process)

    with sim.write_vcd(f'adc.vcd'):
        sim.run()
