from __future__ import annotations

import json
import socketserver
import threading
from collections.abc import Callable

from daemon.aggregator import Aggregator
from daemon.core import Config

REQUIRED_KEYS = ("event", "session_id")


def parse_line(line: str) -> dict | None:
    """Parse a single newline-delimited JSON event line.

    Returns the decoded dict when it is valid JSON containing at least
    ``event`` and ``session_id`` keys, otherwise ``None``.
    """
    try:
        data = json.loads(line)
    except (json.JSONDecodeError, TypeError):
        return None
    if not isinstance(data, dict):
        return None
    if not all(k in data for k in REQUIRED_KEYS):
        return None
    return data


class Listener:
    """Localhost TCP server that feeds newline-delimited JSON events into an Aggregator."""

    def __init__(self, cfg: Config, agg: Aggregator, on_event: Callable[[], None] | None = None) -> None:
        self._cfg = cfg
        self._agg = agg
        self._on_event = on_event
        self._server: socketserver.ThreadingTCPServer | None = None
        self._thread: threading.Thread | None = None

    def start(self) -> None:
        listener = self

        class Handler(socketserver.StreamRequestHandler):
            def handle(self) -> None:
                for raw in self.rfile:
                    line = raw.decode("utf-8", errors="replace").strip()
                    if not line:
                        continue
                    data = parse_line(line)
                    if data is None:
                        continue
                    listener._agg.apply(
                        data["event"],
                        data["session_id"],
                        data.get("cwd", ""),
                        data.get("tmux_pane"),
                        data.get("ts", 0.0),
                    )
                    if listener._on_event is not None:
                        listener._on_event()

        class Server(socketserver.ThreadingTCPServer):
            allow_reuse_address = True
            daemon_threads = True

        self._server = Server(("127.0.0.1", self._cfg.port), Handler)
        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        if self._server is not None:
            self._server.shutdown()
            self._server.server_close()
            self._server = None
        if self._thread is not None:
            self._thread.join(timeout=2)
            self._thread = None
