#!/usr/bin/python3
"""Generative property-based testing for Dumpulse using ctypes and Hypothesis.


"""
import pytest
from hypothesis import settings
from hypothesis.stateful import (rule,
                                 precondition,
                                 RuleBasedStateMachine,
                                 Bundle)
from hypothesis.strategies import integers, binary

import server
import udpclient


class DumpulseTest(RuleBasedStateMachine):
    Server = Bundle('server')

    def get_timestamp(self):
        return self.timestamp

    def send_packet(self, packet):
        self.packets.append(packet)

    @rule(target=Server)
    def new_server(self):
        return (server.Dumpulse(get_timestamp=self.get_timestamp,
                                send_packet=self.send_packet),
                {})

    @rule(target=Server,
          instance=Server,
          when=integers(min_value=0, max_value=65535),
          variable=integers(min_value=0, max_value=255),
          sender=integers(min_value=0, max_value=255),
          value=integers(min_value=0, max_value=255))
    def valid_variable_set_packet(self, instance, when, variable, sender, value):
        server, state = instance

        self.timestamp = when
        packet = udpclient.set_packet(variable, sender, value)
        assert (1 if variable < 64 else 0) == server.process_packet(packet)
        del self.timestamp

        if variable < 64:
            state[variable] = when, sender, value

        return server, state


    @rule(target=Server, instance=Server)
    def try_health_check(self, instance):
        server, state = instance

        self.packets = []
        assert 1 == server.process_packet(udpclient.query_packet)
        health_check, = self.packets
        del self.packets

        settings = udpclient.variable_settings(health_check)
        assert [(i, when, sender, value) for i, (when, sender, value) in
                ((i, state.get(i, (0,0,0))) for i in range(64))] == settings

        return instance

    @rule(target=Server, instance=Server, packet=binary(average_size=8))
    def send_invalid_packet(self, instance, packet):
        server, state = instance

        if len(packet) == 8:
            assert 0 == server.process_packet(packet)
        else:
            with pytest.raises(Exception):
                server.process_packet(packet)

        return instance


TestDumpulse = DumpulseTest.TestCase
