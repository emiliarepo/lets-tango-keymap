from __future__ import annotations

import io
import json
import socket
import threading

from daemon import reporter


def test_build_payload_reads_stdin_and_env():
    p = reporter.build_payload(
        "Notification",
        {"session_id": "s1", "cwd": "/work"},
        {"TMUX_PANE": "%4"},
    )
    assert p["event"] == "Notification" and p["session_id"] == "s1"
    assert p["cwd"] == "/work" and p["tmux_pane"] == "%4" and "ts" in p


def test_build_payload_missing_pane():
    p = reporter.build_payload("Stop", {"session_id": "s1"}, {})
    assert p["tmux_pane"] is None


def test_main_sends_one_json_line_and_returns_0():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.bind(("127.0.0.1", 0))
    srv.listen(1)
    port = srv.getsockname()[1]
    received = {}

    def accept_one():
        conn, _ = srv.accept()
        with conn:
            data = b""
            conn.settimeout(1.0)
            try:
                while not data.endswith(b"\n"):
                    chunk = conn.recv(4096)
                    if not chunk:
                        break
                    data += chunk
            except TimeoutError:
                pass
            received["data"] = data

    t = threading.Thread(target=accept_one, daemon=True)
    t.start()

    stdin = io.StringIO(json.dumps({"session_id": "s1", "cwd": "/work"}))
    rc = reporter.main(
        ["reporter.py", "Notification"], stdin, {"AGENTPAD_PORT": str(port)}
    )
    t.join(timeout=2)
    srv.close()

    assert rc == 0
    payload = json.loads(received["data"].decode("utf-8"))
    assert payload["event"] == "Notification"
    assert payload["session_id"] == "s1"
    assert payload["cwd"] == "/work"


def test_main_swallows_errors_when_no_listener():
    stdin = io.StringIO("")
    # Nothing is listening on this port -> connect should fail; main must
    # still return 0 and must not raise.
    rc = reporter.main(["reporter.py", "Stop"], stdin, {"AGENTPAD_PORT": "1"})
    assert rc == 0


def test_main_handles_malformed_stdin_json():
    stdin = io.StringIO("not-json{{{")
    rc = reporter.main(["reporter.py", "Stop"], stdin, {"AGENTPAD_PORT": "1"})
    assert rc == 0
