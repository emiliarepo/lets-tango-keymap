from __future__ import annotations

import sys
import time

from daemon import actions
from daemon.aggregator import Aggregator
from daemon.core import Config, FleetKey, Status
from daemon.hid_link import HidLink
from daemon.listener import Listener


class Daemon:
    """Wires Listener -> Aggregator -> HidLink -> actions together.

    `tick()` is the single-step unit exercised by the smoke test (with a fake
    `HidLink`); `main()` drives the real, un-tested run loop.
    """

    def __init__(self, cfg: Config) -> None:
        self.cfg = cfg
        self.agg = Aggregator()
        self.link = HidLink(cfg)
        self.listener = Listener(cfg, self.agg, on_event=self._dirty)
        self._last_status: Status | None = None
        self._last_sent_at: float = 0.0

    def _dirty(self) -> None:
        """Callback wired to the Listener on every accepted event. tick()
        already recomputes `agg.aggregate()` on every call, so no extra
        state is needed here today; kept as the wiring point for a future
        immediate-wake optimization."""

    def tick(self) -> None:
        if not self.link.is_open:
            self.link.open()

        key: FleetKey | None = self.link.read_fleet()
        if key is not None:
            actions.dispatch(key, self.cfg, self.agg.waiting_pane())

        now = time.time()
        status = self.agg.aggregate()
        changed = status != self._last_status
        stale = (now - self._last_sent_at) >= self.cfg.keepalive_s
        if changed or stale:
            self.link.send_status(status)
            self._last_status = status
            self._last_sent_at = now

        self.agg.expire(now, self.cfg.session_ttl_s)


def _tick_safe(d: Daemon) -> None:
    """Run one `Daemon.tick()`, absorbing any unexpected exception so a
    single bad iteration (e.g. a fleet-key action failing because tmux is
    missing) never takes down the whole daemon -- the status underglow must
    keep working even when the tmux/Win32 side degrades. Best-effort: report
    to stderr and move on."""
    try:
        d.tick()
    except Exception as exc:  # noqa: BLE001 - must never kill the main loop
        print(f"agentpad_daemon: tick failed: {exc!r}", file=sys.stderr)


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else list(argv)
    config_path = args[0] if args else None
    cfg = Config.load(config_path)
    d = Daemon(cfg)
    d.listener.start()
    try:
        while True:
            _tick_safe(d)
            time.sleep(0.05)
    except KeyboardInterrupt:
        pass
    finally:
        d.listener.stop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
