from __future__ import annotations

from dataclasses import dataclass

from daemon import core
from daemon.core import Status


@dataclass
class _Session:
    status: Status
    cwd: str
    pane: str | None
    ts: float


class Aggregator:
    def __init__(self) -> None:
        self._s: dict[str, _Session] = {}

    def apply(self, event: str, session_id: str, cwd: str, pane: str | None, ts: float) -> None:
        status = core.EVENT_STATUS.get(event)
        if status is None:
            return
        if status == Status.NONE:            # SessionEnd
            self._s.pop(session_id, None)
            return
        self._s[session_id] = _Session(status, cwd, pane, ts)

    def aggregate(self) -> Status:
        if not self._s:
            return Status.NONE
        return max((x.status for x in self._s.values()), key=lambda s: core.STATUS_RANK[s])

    def waiting_pane(self) -> str | None:
        waiting = [x for x in self._s.values() if x.status == Status.WAITING]
        if not waiting:
            return None
        return max(waiting, key=lambda x: x.ts).pane

    def panes(self) -> list[str]:
        return [x.pane for x in self._s.values() if x.pane]

    def expire(self, now: float, ttl: float) -> None:
        dead = [k for k, v in self._s.items() if now - v.ts > ttl]
        for k in dead:
            del self._s[k]
