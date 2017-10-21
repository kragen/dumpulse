#!/usr/bin/python3
"""Generative property-based testing for Dumpulse using ctypes and Hypothesis.


"""
import pytest
from hypothesis import settings
from hypothesis.stateful import rule, RuleBasedStateMachine, Bundle
from hypothesis.strategies import integers, binary

import server
import client


u16 = integers(min_value=0, max_value=65535)
u8 = integers(min_value=0, max_value=255)


class DumpulseTest(RuleBasedStateMachine):
    Bb = Bundle('bb')

    def get_timestamp(self):
        return self.timestamp

    def send_packet(self, packet):
        self.packets.append(packet)

    @rule(target=Bb)
    def new_server(self):
        return (server.Dumpulse(get_timestamp=self.get_timestamp,
                                send_packet=self.send_packet),
                {})

    @rule(target=Bb, instance=Bb, when=u16, variable=u8, sender=u8, value=u8)
    def valid_variable_set_packet(self, instance, when, variable, sender, value):
        server, state = instance

        self.timestamp = when
        packet = client.set_packet(variable, sender, value)
        assert (1 if variable < 64 else 0) == server.process_packet(packet)
        del self.timestamp

        if variable < 64:
            state[variable] = when, sender, value

        return server, state


    @rule(target=Bb, instance=Bb)
    def try_health_check(self, instance):
        server, state = instance

        self.packets = []
        assert 1 == server.process_packet(client.query_packet)
        health_check, = self.packets
        del self.packets

        settings = client.variable_settings(health_check)
        assert [(i, when, sender, value) for i, (when, sender, value) in
                ((i, state.get(i, (0,0,0))) for i in range(64))] == settings

        return instance

    @rule(target=Bb, instance=Bb, packet=binary(average_size=8))
    def send_invalid_packet(self, instance, packet):
        server, state = instance

        if len(packet) == 8:
            assert 0 == server.process_packet(packet)
        else:
            with pytest.raises(Exception):
                server.process_packet(packet)

        return instance


TestDumpulse = DumpulseTest.TestCase
