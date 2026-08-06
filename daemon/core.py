from __future__ import annotations
import tomllib
from dataclasses import dataclass, field
from enum import IntEnum

RAW_REPORT_SIZE = 32
CMD_STATUS = 0x01
CMD_FLEET = 0x10

class Status(IntEnum):
    NONE = 0; IDLE = 1; RUNNING = 2; WAITING = 3; ERROR = 4

class FleetKey(IntEnum):
    JUMP = 0; AGENT_PREV = 1; AGENT_NEXT = 2; NEW = 3

STATUS_RANK = {Status.NONE:0, Status.IDLE:1, Status.RUNNING:2, Status.WAITING:3, Status.ERROR:4}

EVENT_STATUS = {
    "SessionStart": Status.IDLE,
    "UserPromptSubmit": Status.RUNNING,
    "PreToolUse": Status.RUNNING,
    "PostToolUse": Status.RUNNING,
    "Notification": Status.WAITING,
    "Stop": Status.IDLE,
    "SubagentStop": Status.IDLE,
    "SessionEnd": Status.NONE,
}

@dataclass
class Config:
    vid: int = 0x1209
    pid: int = 0xBEE5
    usage_page: int = 0xFF60
    usage: int = 0x61
    port: int = 8787
    keepalive_s: float = 2.0
    stale_s: float = 5.0
    session_ttl_s: float = 3600.0
    foreground: bool = True
    terminal_exe: str = "WindowsTerminal.exe"
    new_window_argv: list[str] = field(default_factory=lambda: ["new-window", "claude"])

    @classmethod
    def load(cls, path: str | None) -> "Config":
        c = cls()
        if not path:
            return c
        with open(path, "rb") as f:
            data = tomllib.load(f)
        dev = data.get("device", {})
        for k in ("vid", "pid", "usage_page", "usage"):
            if k in dev: setattr(c, k, dev[k])
        for k in ("port", "keepalive_s", "stale_s", "session_ttl_s", "foreground",
                  "terminal_exe", "new_window_argv"):
            if k in data: setattr(c, k, data[k])
        return c
