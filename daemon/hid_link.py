from __future__ import annotations

from daemon import core, protocol
from daemon.core import Config, FleetKey, Status


class HidLink:
    """Raw HID transport to the pad. Isolates the `hid` (hidapi) import to
    this module so the rest of the daemon tests without hardware or hidapi
    installed. Construction never touches a device."""

    def __init__(self, cfg: Config):
        self.cfg = cfg
        self._dev = None
        self._open = False

    @property
    def is_open(self) -> bool:
        return self._open

    def open(self) -> bool:
        try:
            import hid  # lazy; optional
        except ImportError:
            self._open = False
            return False

        try:
            path = None
            for d in hid.enumerate(self.cfg.vid, self.cfg.pid):
                if (
                    d.get("usage_page") == self.cfg.usage_page
                    and d.get("usage") == self.cfg.usage
                ):
                    path = d.get("path")
                    break
            if path is None:
                self._open = False
                return False
            dev = hid.device()
            dev.open_path(path)
        except OSError:
            self._open = False
            return False

        self._dev = dev
        self._open = True
        return True

    def send_status(self, status: Status) -> None:
        if not self._open or self._dev is None:
            return
        try:
            self._dev.write(protocol.encode_status(status))
        except OSError:
            self._open = False

    def read_fleet(self, timeout_ms: int = 50) -> FleetKey | None:
        if not self._open or self._dev is None:
            return None
        try:
            report = self._dev.read(core.RAW_REPORT_SIZE, timeout_ms)
        except OSError:
            self._open = False
            return None
        if not report:
            return None
        return protocol.decode_fleet(bytes(report))

    def close(self) -> None:
        if self._dev is not None:
            try:
                self._dev.close()
            except OSError:
                pass
        self._open = False
        self._dev = None
