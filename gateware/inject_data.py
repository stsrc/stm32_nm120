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

    def get_bus(self):
        return self.simple_ports_to_wb.bus

    def elaborate(self, platform):
        m = Module()

        usb_valid = Signal()
        usb_first = Signal()
        usb_last = Signal()
        usb_payload = Signal(8)
        payload = Signal(32)
        counter = Signal(11)
        bit_rotate = Signal(8)
        state = Signal(8)

        m.d.comb += [
            usb_first.eq(self.usb_stream_in.first),
            usb_last.eq(self.usb_stream_in.last),
            usb_valid.eq(self.usb_stream_in.valid),
            usb_payload.eq(self.usb_stream_in.payload)
        ]

        m.d.comb += self.debug_byte.eq(state)
        m.d.comb += bit_rotate.eq((3 - (counter % 4)) * 8)

        with m.FSM(reset="RESET"):
            with m.State("RESET"):
                m.d.sync += state.eq(0b11110000)
                m.d.sync += self.wait_counter.eq(0)
                m.next = "WAIT_BEFORE_START"

            with m.State("WAIT_BEFORE_START"):
                m.d.sync += state.eq(0b11001100)
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
                    m.d.sync += counter.eq(counter + 1)
                    m.d.sync += payload.eq((usb_payload << bit_rotate) | payload)
                    with m.If((usb_last) | (counter > 3)):
                        m.d.sync += counter.eq(0)
                        m.next = "SEND_DATA_TO_USB"

            with m.State("SEND_DATA_TO_USB"):
                m.d.comb += self.usb_stream_in.ready.eq(0)
                with m.If(self.usb_stream_out.ready):
                    m.d.comb += self.usb_stream_out.valid.eq(1)
                    m.d.comb += self.usb_stream_out.payload.eq(payload >> bit_rotate)
                    m.d.sync += counter.eq(counter + 1)
                    with m.If(counter == 0):
                        m.d.comb += self.usb_stream_out.first.eq(1)
                        m.d.comb += self.usb_stream_out.last.eq(0)
                    with m.Elif(counter == 3):
                        m.d.sync += counter.eq(0)
                        m.d.comb += self.usb_stream_out.last.eq(1)
                        m.next = "IDLE"
                        m.d.sync += state.eq(state + 1)
                    with m.Else():
                        m.d.comb += self.usb_stream_out.first.eq(0)    
                with m.Else():
                    m.d.comb += self.usb_stream_out.valid.eq(0)

        return m
