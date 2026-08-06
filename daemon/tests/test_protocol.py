from daemon import core, protocol
from daemon.core import FleetKey, Status


def test_encode_status_shape():
    b = protocol.encode_status(Status.WAITING)
    assert len(b) == core.RAW_REPORT_SIZE + 1
    assert b[0] == 0x00 and b[1] == core.CMD_STATUS and b[2] == int(Status.WAITING)
    assert set(b[3:]) == {0}


def test_decode_fleet_valid():
    assert protocol.decode_fleet(bytes([core.CMD_FLEET, 3] + [0]*30)) == FleetKey.NEW


def test_decode_fleet_rejects_other():
    assert protocol.decode_fleet(bytes([0x01, 3] + [0]*30)) is None
    assert protocol.decode_fleet(b"") is None


def test_decode_fleet_unknown_key():
    assert protocol.decode_fleet(bytes([core.CMD_FLEET, 99] + [0]*30)) is None
