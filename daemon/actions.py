from __future__ import annotations

import subprocess

from daemon.core import Config, FleetKey


def jump_argv(pane: str) -> list[list[str]]:
    return [["select-window", "-t", pane], ["select-pane", "-t", pane]]


def cycle_argv(direction: int) -> list[str]:
    return ["select-window", "-t", "+1" if direction > 0 else "-1"]


def new_argv(cfg: Config) -> list[str]:
    return list(cfg.new_window_argv)


def run(cmds: list[list[str]]) -> None:
    for c in cmds:
        try:
            subprocess.run(["tmux", *c], check=False)
        except OSError:
            pass  # e.g. FileNotFoundError when tmux isn't on native Windows PATH


def foreground(cfg: Config) -> None:
    if not cfg.foreground:
        return
    try:
        import psutil  # lazy; optional
        import win32gui  # lazy; optional
        import win32process  # lazy; optional

        def _cb(hwnd, acc):
            _, pid = win32process.GetWindowThreadProcessId(hwnd)
            try:
                if psutil.Process(pid).name().lower() == cfg.terminal_exe.lower():
                    acc.append(hwnd)
            except Exception:  # noqa: BLE001, S110 - best-effort process lookup
                pass

        acc: list[int] = []
        win32gui.EnumWindows(_cb, acc)
        if acc:
            win32gui.SetForegroundWindow(acc[0])
    except Exception:  # noqa: BLE001, S110 - best-effort foreground, never fatal
        pass


def dispatch(key: FleetKey, cfg: Config, waiting_pane: str | None) -> None:
    if key == FleetKey.JUMP and waiting_pane:
        run(jump_argv(waiting_pane))
        foreground(cfg)
    elif key == FleetKey.AGENT_PREV:
        run([cycle_argv(-1)])
    elif key == FleetKey.AGENT_NEXT:
        run([cycle_argv(+1)])
    elif key == FleetKey.NEW:
        run([new_argv(cfg)])
