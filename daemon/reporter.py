"""Claude Code hook reporter.

Invoked by Claude Code as: python reporter.py <EventName>

Reads the hook's event JSON from stdin (Claude Code always sends
`session_id` and `cwd` as part of the common hook input), adds the
tmux pane (if any) and a timestamp, and forwards one JSON line to the
companion daemon over localhost TCP. Never raises and never blocks
Claude Code: on any error it swallows the exception and exits 0.
"""

from __future__ import annotations

import json
import os
import socket
import sys
import time

DEFAULT_PORT = 8787
SOCKET_TIMEOUT = 0.3


def build_payload(event: str, stdin_json: dict, env: dict) -> dict:
    """Pure: build the JSON payload sent to the daemon.

    `stdin_json` is the parsed hook event JSON (from Claude Code's stdin);
    `env` is the process environment (used only for TMUX_PANE).
    """
    return {
        "event": event,
        "session_id": stdin_json.get("session_id"),
        "cwd": stdin_json.get("cwd"),
        "tmux_pane": env.get("TMUX_PANE"),
        "ts": time.time(),
    }


def main(argv: list[str], stdin, env: dict) -> int:
    """Entry point. Always returns 0 — errors are swallowed so a reporter
    failure never blocks or delays Claude Code."""
    try:
        event = argv[1] if len(argv) > 1 else ""

        raw = stdin.read()
        try:
            stdin_json = json.loads(raw) if raw else {}
        except ValueError:
            stdin_json = {}

        payload = build_payload(event, stdin_json, env)

        port = int(env.get("AGENTPAD_PORT", DEFAULT_PORT))
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(SOCKET_TIMEOUT)
            sock.connect(("127.0.0.1", port))
            sock.sendall((json.dumps(payload) + "\n").encode("utf-8"))
    except Exception:  # noqa: BLE001, S110 - never block/fail a Claude Code hook
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv, sys.stdin, dict(os.environ)))
