from daemon import agentpad_daemon
from daemon.agentpad_daemon import Daemon
from daemon.core import Config, FleetKey, Status


class FakeLink:
    def __init__(self):
        self.sent = []
        self.fleet = [FleetKey.JUMP]
        self.is_open = True

    def open(self):
        return True

    def send_status(self, s):
        self.sent.append(s)

    def read_fleet(self, timeout_ms=50):
        return self.fleet.pop() if self.fleet else None


def test_tick_sends_status_and_dispatches(monkeypatch):
    d = Daemon(Config())
    d.link = FakeLink()
    d.agg.apply("Notification", "s1", "/a", "%1", 1.0)
    calls = []
    monkeypatch.setattr(
        "daemon.agentpad_daemon.actions.dispatch",
        lambda k, c, p: calls.append((k, p)),
    )
    d.tick()
    assert d.link.sent[-1] == Status.WAITING
    assert calls and calls[0] == (FleetKey.JUMP, "%1")


def test_tick_safe_survives_dispatch_failure(monkeypatch):
    d = Daemon(Config())
    d.link = FakeLink()  # has one queued FleetKey.JUMP
    d.agg.apply("Notification", "s1", "/a", "%1", 1.0)

    def _raise(k, c, p):
        raise RuntimeError("tmux exploded")

    monkeypatch.setattr("daemon.agentpad_daemon.actions.dispatch", _raise)

    # First iteration: dispatch raises, but the guard must swallow it, and
    # the exception must not propagate to the caller (the main loop).
    agentpad_daemon._tick_safe(d)
    assert d.link.sent == []

    # Second iteration: no fleet key left, dispatch isn't called, and the
    # daemon keeps ticking normally -- status still gets sent.
    agentpad_daemon._tick_safe(d)
    assert d.link.sent and d.link.sent[-1] == Status.WAITING
