from daemon.aggregator import Aggregator
from daemon.core import Status


def test_empty_is_none():
    assert Aggregator().aggregate() == Status.NONE


def test_most_urgent_wins():
    a = Aggregator()
    a.apply("SessionStart", "s1", "/a", "%1", 1.0)      # idle
    a.apply("PreToolUse",   "s2", "/b", "%2", 2.0)      # running
    a.apply("Notification", "s3", "/c", "%3", 3.0)      # waiting
    assert a.aggregate() == Status.WAITING
    assert a.waiting_pane() == "%3"


def test_waiting_clears_on_stop():
    a = Aggregator()
    a.apply("Notification", "s1", "/a", "%1", 1.0)
    a.apply("Stop",         "s1", "/a", "%1", 2.0)      # back to idle
    assert a.aggregate() == Status.IDLE
    assert a.waiting_pane() is None


def test_session_end_removes():
    a = Aggregator()
    a.apply("SessionStart", "s1", "/a", "%1", 1.0)
    a.apply("SessionEnd",   "s1", "/a", "%1", 2.0)
    assert a.aggregate() == Status.NONE
    assert a.panes() == []


def test_latest_waiting_pane_wins():
    a = Aggregator()
    a.apply("Notification", "s1", "/a", "%1", 1.0)
    a.apply("Notification", "s2", "/b", "%2", 5.0)
    assert a.waiting_pane() == "%2"


def test_expire_drops_stale():
    a = Aggregator()
    a.apply("PreToolUse", "s1", "/a", "%1", 1.0)
    a.expire(now=100.0, ttl=10.0)
    assert a.aggregate() == Status.NONE
