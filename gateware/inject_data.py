#!/usr/bin/env python3
#
# Copyright (c) 2024 Konrad Gotfryd <gotfrydkonrad@gmail.com>
# SPDX-License-Identifier: CERN-OHL-W-2.0

import os

from amaranth import *
from amlib.stream  import StreamInterface

__all__ = ["InjectData"]

class InjectData(Elaboratable):
    def __init__(self, simulation):
        self.wait_counter = Signal(26)
        self.simulation = simulation

        self.usb_stream_in = StreamInterface(name="usb_stream_in")
        self.usb_stream_out = StreamInterface(name="usb_stream_out")
        self.debug_byte = Signal(8)

        self.adc_data = Signal(16)
        self.adc_mem_addr = Signal(9)
        self.adc_trig = Signal()
        self.adc_limit = Signal(9)
        self.adc_done = Signal()

    def get_bus(self):
        return self.simple_ports_to_wb.bus

    def elaborate(self, platform):
        m = Module()

        usb_valid = Signal()
        usb_first = Signal()
        usb_last = Signal()
        usb_payload = Signal(8)
        payload = Signal(32)
        counter = Signal(10)

        adc_done_in_1 = Signal()
        adc_done_in = Signal()

        m.d.sync += [
            adc_done_in_1.eq(self.adc_done),
            adc_done_in.eq(adc_done_in_1)
        ]

        m.d.comb += [
            usb_first.eq(self.usb_stream_in.first),
            usb_last.eq(self.usb_stream_in.last),
            usb_valid.eq(self.usb_stream_in.valid),
            usb_payload.eq(self.usb_stream_in.payload)
        ]

        with m.FSM(reset="RESET"):
            with m.State("RESET"):
                m.d.sync += self.wait_counter.eq(0)
                m.next = "WAIT_BEFORE_START"

            with m.State("WAIT_BEFORE_START"):
                m.d.sync += self.wait_counter.eq(self.wait_counter + 1)
                if self.simulation:
                    m.d.sync += self.wait_counter.eq(30000000)
                with m.If (self.wait_counter == 30000000):
                    m.next = "IDLE"
                    m.d.sync += self.wait_counter.eq(0)

            with m.State("IDLE"):
               m.d.comb += self.usb_stream_out.valid.eq(0)
               m.d.comb += self.usb_stream_out.last.eq(0)
               with m.If(usb_valid):
                   m.next = "RECEIVE_DATA_FROM_USB"
                   m.d.sync += payload.eq(0)

            with m.State("RECEIVE_DATA_FROM_USB"):
                with m.If(usb_valid):
                    m.d.comb += self.usb_stream_in.ready.eq(1)
                    with m.If(~usb_last):
                        m.d.sync += self.adc_limit.eq((usb_payload & 0x01) << 8)
                    with m.Else():
                        m.d.sync += self.adc_limit.eq(self.adc_limit | usb_payload)
                        m.next = "WAIT_FOR_ADC"
                        m.d.sync += self.adc_trig.eq(1)

            with m.State("WAIT_FOR_ADC"):
                m.d.comb += self.usb_stream_in.ready.eq(0)
                m.d.sync += self.adc_trig.eq(0)
                with m.If(self.adc_limit == 0):
                    m.next = "IDLE";
                with m.Elif(adc_done_in):
                    m.next = "SEND_DATA_TO_USB"
    
            with m.State("SEND_DATA_TO_USB"):
                with m.If(self.usb_stream_out.ready):
                    m.d.comb += self.usb_stream_out.valid.eq(1)
                    m.d.sync += counter.eq(counter + 1)
                    with m.If(counter & 1 == 0):                         
                        m.d.comb += self.usb_stream_out.payload.eq((self.adc_data & 0xff00) >> 8)
                    with m.Elif(counter & 1 == 1):
                        m.d.comb += self.usb_stream_out.payload.eq(self.adc_data & 0xff)
                        m.d.sync += self.adc_mem_addr.eq(self.adc_mem_addr + 1)

                    with m.If(counter == 0):
                        m.d.comb += self.usb_stream_out.first.eq(1)
                        m.d.comb += self.usb_stream_out.last.eq(0)
                    with m.Elif(self.adc_mem_addr == self.adc_limit - 1):
                        m.d.comb += self.usb_stream_out.last.eq(1)
                        m.d.sync += self.adc_mem_addr.eq(0)
                        m.next = "IDLE"
                    with m.Else():
                        m.d.comb += self.usb_stream_out.first.eq(0)    

                with m.Else():
                    m.d.comb += self.usb_stream_out.valid.eq(0)

        return m
