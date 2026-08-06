import json
import socket
import time

from daemon.aggregator import Aggregator
from daemon.core import Config, Status
from daemon.listener import Listener, parse_line


def test_parse_line_ok():
    d = parse_line(json.dumps({"event": "Notification", "session_id": "s", "cwd": "/a", "tmux_pane": "%1", "ts": 1.0}))
    assert d["event"] == "Notification" and d["tmux_pane"] == "%1"


def test_parse_line_bad():
    assert parse_line("not json") is None


def test_listener_feeds_aggregator():
    agg = Aggregator()
    cfg = Config(port=8899)
    lis = Listener(cfg, agg)
    lis.start()
    try:
        s = socket.create_connection(("127.0.0.1", 8899), timeout=2)
        s.sendall((json.dumps({"event": "Notification", "session_id": "s1", "cwd": "/a", "tmux_pane": "%2", "ts": 1.0}) + "\n").encode())
        s.close()
        for _ in range(50):
            if agg.aggregate() == Status.WAITING:
                break
            time.sleep(0.02)
        assert agg.aggregate() == Status.WAITING and agg.waiting_pane() == "%2"
    finally:
        lis.stop()
