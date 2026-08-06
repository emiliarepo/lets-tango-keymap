from daemon.core import Config, FleetKey, Status
from daemon.hid_link import HidLink


class FakeDev:
    def __init__(self):
        self.written = []

    def write(self, b):
        self.written.append(bytes(b))
        return len(b)

    def read(self, n, timeout_ms=0):
        return list(bytes([0x10, 3]) + bytes(30))

    def close(self):
        pass


def test_send_status_writes_encoded(monkeypatch):
    link = HidLink(Config())
    link._dev = FakeDev()
    link._open = True
    link.send_status(Status.WAITING)
    assert link._dev.written[0][1] == 0x01 and link._dev.written[0][2] == int(Status.WAITING)


def test_read_fleet_decodes(monkeypatch):
    link = HidLink(Config())
    link._dev = FakeDev()
    link._open = True
    assert link.read_fleet() == FleetKey.NEW
