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
