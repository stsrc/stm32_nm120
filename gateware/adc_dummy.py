#!/usr/bin/env python3
#
# Copyright (c) 2024 Konrad Gotfryd <gotfrydkonrad@gmail.com>
# SPDX-License-Identifier: CERN-OHL-W-2.0

import os

from amaranth import *
from amaranth_soc import memory

__all__ = ["ADC"]

class ADC_dummy(Elaboratable):
    def __init__(self, simulation=False):

        # ADC signals
        self.adc = Signal(14)
        self.over_range = Signal()
        self.encode = Signal(2)
        self.dry = Signal()

        # Logic signals
        self.data = Signal(16)
        self.data_ready = Signal()
        self.addr = Signal(9)
        self.trig = Signal()
        self.limit = Signal(9)
        self.done = Signal()

        # misc
        self.debug_byte = Signal(8)
        self.wait_counter = Signal(26)

        self.simulation = simulation

    def set_encode(self, m):
         m.d.comb += self.encode[0].eq(1)
         m.d.comb += self.encode[1].eq(0)

    def reset_encode(self, m):
         m.d.comb += self.encode[0].eq(0)
         m.d.comb += self.encode[1].eq(1)

    def elaborate(self, platform):
        m = Module()

        m.d.sync += self.debug_byte.eq(self.data)
        limit = Signal(9)
        limit_in_1 = Signal(9)
        limit_in = Signal(9)
        trig_1 = Signal()
        trig = Signal()

        counter = Signal(14)

        m.d.sync += [
            trig_1.eq(self.trig),
            trig.eq(trig_1),
            limit_in_1.eq(self.limit),
            limit_in.eq(limit_in_1)
        ]

        with m.FSM(reset="RESET"):
            with m.State("RESET"):
                self.reset_encode(m)
                m.d.comb += self.data_ready.eq(0)
                m.d.sync += self.done.eq(0)
                m.d.sync += self.addr.eq(0)
                m.d.sync += self.wait_counter.eq(0)
                m.next = "WAIT_BEFORE_START"

            with m.State("WAIT_BEFORE_START"):
                self.reset_encode(m)
                m.d.comb += self.data_ready.eq(0)
                m.d.sync += self.wait_counter.eq(self.wait_counter + 1)
                if self.simulation:
                    m.d.sync += self.wait_counter.eq(30000000)
                with m.If (self.wait_counter == 30000000):
                    m.next = "IDLE"
                    m.d.sync += self.wait_counter.eq(0)

            with m.State("IDLE"):
                self.reset_encode(m)
                m.d.comb += self.data_ready.eq(0)
                with m.If ((trig == 1) & (limit_in != 0)):
                    m.d.sync += self.done.eq(0)
                    m.d.sync += limit.eq(limit_in - 1)
                    m.next = "TRIGGER"

            with m.State("TRIGGER"):
                self.set_encode(m)
                m.d.comb += self.data_ready.eq(0)
                m.next = "RECEIVE"

            with m.State("RECEIVE"):
                self.reset_encode(m)
                m.d.comb += self.data.eq((self.over_range << 14) | counter)
                m.d.comb += self.data_ready.eq(1)
                m.d.sync += self.addr.eq(self.addr + 1)
                m.d.sync += counter.eq(counter + 1)
                with m.If(self.addr == limit):
                    m.d.sync += counter.eq(0)
                    m.d.sync += self.addr.eq(0)
                    m.next = "IDLE"
                    m.d.sync += self.done.eq(1)
                with m.Else():
                    m.next = "TRIGGER"
                    
        return m
