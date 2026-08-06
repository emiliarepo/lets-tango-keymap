from __future__ import annotations

from daemon import core
from daemon.core import FleetKey, Status


def encode_status(status: Status) -> bytes:
    buf = bytearray(core.RAW_REPORT_SIZE + 1)  # [0]=report id 0x00
    buf[1] = core.CMD_STATUS
    buf[2] = int(status)
    return bytes(buf)


def decode_fleet(report: bytes) -> FleetKey | None:
    if len(report) < 2 or report[0] != core.CMD_FLEET:
        return None
    try:
        return FleetKey(report[1])
    except ValueError:
        return None
