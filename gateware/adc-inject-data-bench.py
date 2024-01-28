#!/usr/bin/env python3
from adc import ADC
from inject_data import InjectData
from amaranth import *
from amlib.stream  import StreamInterface

__all__ = ["ADCInjectDataBench"]

class ADCInjectDataBench(Elaboratable):
    def __init__(self):
        self.usb_stream_in = StreamInterface(name="usb_stream_in")
        self.usb_stream_out = StreamInterface(name="usb_stream_out")
        self.adc_data = Signal(14)
        self.adc_over_range = Signal()
        self.adc_dry = Signal()
        self.adc_en = Signal(2)

    def elaborate(self, platform):
        m = Module()
        m.domains.usb  = ClockDomain()
        m.domains.fast = ClockDomain()

        m.submodules.inject_data = inject_data = DomainRenamer("usb")(InjectData(simulation=True))
        m.submodules.adc = adc = DomainRenamer("fast")(ADC(simulation=True))
        adc_memory = Memory(width=16, depth=512, name="adc_memory", simulate=True)
        m.submodules.adc_memory_write_port = adc_mem_w = DomainRenamer("fast")(adc_memory.write_port(granularity=16))
        m.submodules.adc_memory_read_port = adc_mem_r = adc_memory.read_port(domain="comb")
        m.d.comb += [
            adc_mem_w.addr.eq(adc.addr),
            adc_mem_w.data.eq(adc.data),
            adc_mem_w.en.eq(adc.data_ready),
            adc_mem_r.addr.eq(inject_data.adc_mem_addr),
            inject_data.usb_stream_in.stream_eq(self.usb_stream_in),
            self.usb_stream_out.stream_eq(inject_data.usb_stream_out),
            inject_data.adc_data.eq(adc_mem_r.data),
            inject_data.adc_done.eq(adc.done),
            adc.trig.eq(inject_data.adc_trig),
            adc.limit.eq(inject_data.adc_limit),
            adc.adc.eq(self.adc_data),
            adc.over_range.eq(self.adc_over_range),
            adc.dry.eq(self.adc_dry),
            self.adc_en.eq(adc.encode)
        ]

        return m


from amaranth.sim import Simulator, Tick

if __name__ == "__main__":
    dut = ADCInjectDataBench()
    
    words_to_transfer_max = 10
    
    def process_inject_data_input():
        words_to_transfer = 1

        for i in range(words_to_transfer_max):
            yield dut.usb_stream_in.payload.eq((words_to_transfer & 0x100) >> 8)
            yield dut.usb_stream_in.first.eq(1)
            yield dut.usb_stream_in.valid.eq(1)
            yield Tick(domain="usb")
            while((yield dut.usb_stream_in.ready) == 0):
                yield Tick(domain="usb")
            yield dut.usb_stream_in.first.eq(0)
            yield dut.usb_stream_in.payload.eq(words_to_transfer & 0xff)
            yield dut.usb_stream_in.last.eq(1)
            yield Tick(domain="usb")
            while((yield dut.usb_stream_in.ready) == 0):
                yield Tick(domain="usb")
            yield dut.usb_stream_in.valid.eq(0)
            yield dut.usb_stream_in.last.eq(0)
            yield dut.usb_stream_out.ready.eq(1)
            while((yield dut.usb_stream_out.valid) == 0):
                yield Tick(domain="usb")
            for j in range(words_to_transfer):
                yield Tick(domain="usb")
                yield Tick(domain="usb")
            yield dut.usb_stream_out.ready.eq(0)
            words_to_transfer += 1

    def process_adc_input():
        yield dut.adc_data.eq(0)
        yield dut.adc_over_range.eq(0)
        yield dut.adc_dry.eq(0)
        for i in range(words_to_transfer_max):
            yield dut.adc_data.eq(0)
            for j in range(i + 1):
                while ((yield dut.adc_en[0]) == 0):
                    yield Tick(domain="fast")
                yield Tick(domain="fast")
                yield dut.adc_dry.eq(1)
                yield Tick(domain="fast")
                yield dut.adc_dry.eq(0)
                yield dut.adc_data.eq(dut.adc_data + 1)
      
    sim = Simulator(dut)
    sim.add_clock(1.0/60e6, domain="usb")
    sim.add_clock(1.0/200e6, domain="fast")
    sim.add_sync_process(process_inject_data_input, domain="usb")
    sim.add_sync_process(process_adc_input, domain="fast")

    with sim.write_vcd(f'adc-inject-data.vcd'):
        sim.run()
