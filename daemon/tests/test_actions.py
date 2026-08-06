from daemon import actions
from daemon.core import Config


def test_jump_argv_selects_pane():
    cmds = actions.jump_argv("%3")
    assert ["select-window", "-t", "%3"] in cmds
    assert ["select-pane", "-t", "%3"] in cmds


def test_cycle_argv_direction():
    assert actions.cycle_argv(-1) == ["select-window", "-t", "-1"]
    assert actions.cycle_argv(+1) == ["select-window", "-t", "+1"]


def test_new_argv_from_config():
    assert actions.new_argv(Config()) == ["new-window", "claude"]
