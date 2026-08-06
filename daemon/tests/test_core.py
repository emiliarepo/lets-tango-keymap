from daemon import core
from daemon.core import Config, Status


def test_wire_constants():
    assert core.RAW_REPORT_SIZE == 32
    assert core.CMD_STATUS == 0x01 and core.CMD_FLEET == 0x10


def test_status_rank_orders_urgency():
    r = core.STATUS_RANK
    assert r[Status.ERROR] > r[Status.WAITING] > r[Status.RUNNING] > r[Status.IDLE] > r[Status.NONE]


def test_event_status_mapping():
    m = core.EVENT_STATUS
    assert m["SessionStart"] == Status.IDLE
    assert m["UserPromptSubmit"] == Status.RUNNING
    assert m["PreToolUse"] == Status.RUNNING
    assert m["PostToolUse"] == Status.RUNNING
    assert m["Notification"] == Status.WAITING
    assert m["Stop"] == Status.IDLE
    assert m["SubagentStop"] == Status.IDLE
    assert m["SessionEnd"] == Status.NONE


def test_config_defaults():
    c = Config.load(None)
    assert c.vid == 0x1209 and c.pid == 0xBEE5
    assert c.usage_page == 0xFF60 and c.usage == 0x61
    assert c.port == 8787 and c.keepalive_s == 2.0


def test_config_load_toml(tmp_path):
    p = tmp_path / "c.toml"
    p.write_text('port = 9999\n[device]\nvid = 0x1234\n')
    c = Config.load(str(p))
    assert c.port == 9999 and c.vid == 0x1234 and c.pid == 0xBEE5  # unset keeps default
