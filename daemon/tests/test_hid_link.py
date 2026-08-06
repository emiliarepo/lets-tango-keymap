import sys

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


class RaisingWriteDev:
    def write(self, b):
        raise OSError("device unplugged")


class RaisingReadDev:
    def read(self, n, timeout_ms=0):
        raise OSError("device unplugged")


class FakeHidDevice:
    """Stand-in for the object returned by the real `hid.device()`."""

    def __init__(self):
        self.opened_path = None

    def open_path(self, path):
        self.opened_path = path


class FakeHidModule:
    """Stand-in for the lazily-imported `hid` module."""

    def __init__(self, entries, device_factory=FakeHidDevice):
        self._entries = entries
        self._device_factory = device_factory

    def enumerate(self, vid, pid):
        return self._entries

    def device(self):
        return self._device_factory()


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


def test_open_success_matches_usage_interface(monkeypatch):
    cfg = Config()
    entry = {
        "usage_page": cfg.usage_page,
        "usage": cfg.usage,
        "path": b"/fake/hidraw0",
    }
    fake_hid = FakeHidModule([entry])
    monkeypatch.setitem(sys.modules, "hid", fake_hid)

    link = HidLink(cfg)
    assert link.open() is True
    assert link.is_open is True


def test_open_returns_false_when_no_interface_matches(monkeypatch):
    cfg = Config()
    entry = {
        "usage_page": cfg.usage_page + 1,  # deliberately mismatched
        "usage": cfg.usage,
        "path": b"/fake/hidraw0",
    }
    fake_hid = FakeHidModule([entry])
    monkeypatch.setitem(sys.modules, "hid", fake_hid)

    link = HidLink(cfg)
    assert link.open() is False
    assert link.is_open is False


def test_send_status_oserror_closes_link():
    link = HidLink(Config())
    link._dev = RaisingWriteDev()
    link._open = True
    link.send_status(Status.ERROR)
    assert link.is_open is False


def test_read_fleet_oserror_closes_link():
    link = HidLink(Config())
    link._dev = RaisingReadDev()
    link._open = True
    assert link.read_fleet() is None
    assert link.is_open is False
