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


def test_run_continues_after_missing_tmux(monkeypatch):
    calls = []

    def _raise(argv, **kwargs):
        calls.append(argv)
        raise FileNotFoundError("tmux not found")

    monkeypatch.setattr(actions.subprocess, "run", _raise)
    # Must not raise, and must still attempt every command.
    actions.run([["select-window", "-t", "%1"], ["select-pane", "-t", "%1"]])
    assert len(calls) == 2
