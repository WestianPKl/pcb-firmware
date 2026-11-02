import time
from controller_client import ControllerClient

HOST = "192.168.18.200"
PORT = 5001
COMM_SLOT = 2
STATUS_OK = 0x40
RX_TIMEOUT_S = 1.0


def require_ok(response, operation):
    if response.get("status") != STATUS_OK:
        raise RuntimeError(f"{operation} failed: {response}")


def configure(client, brs):
    response = client.can_config(
        COMM_SLOT,
        nominal_bitrate=1,  # 500 kbps, not 500000 Hz
        data_bitrate=1,  # 2 Mbps, not 2000000 Hz
        fd_enabled=1,
        brs_enabled=int(brs),
        mode=2,  # External Loopback
        can_termination=0,
    )
    require_ok(response, f"CAN_CONFIG BRS={int(brs)}")
    print(f"\n[CONFIG OK] CAN2 500 kbps / 2 Mbps, BRS={int(brs)}, External Loopback")


def drain_mailbox(client):
    """Remove a stale frame; no other CAN2 producer should be active during testing."""
    for _ in range(8):
        response = client.can_receive(COMM_SLOT)
        require_ok(response, "CAN_RECEIVE (drain)")
        if response.get("data") is None:
            return
        print(f"  [INFO] Discarded stale RX: {response['data']}")
    raise RuntimeError("CAN2 mailbox did not become empty")


def wait_for_rx(client):
    deadline = time.monotonic() + RX_TIMEOUT_S
    while time.monotonic() < deadline:
        response = client.can_receive(COMM_SLOT)
        require_ok(response, "CAN_RECEIVE")
        frame = response.get("data")
        if frame is not None:
            return frame
        time.sleep(0.02)
    raise TimeoutError("CAN2 RX empty after TX (1 second)")


def test_frame(client, can_id, payload, is_fd, brs_expected):
    label = "FD" if is_fd else "Classic"
    send = client.can_fd_send if is_fd else client.can_std_send
    response = send(COMM_SLOT, can_id, payload)
    require_ok(response, f"{label} TX ID=0x{can_id:03X}")

    frame = wait_for_rx(client)
    problems = []
    if frame["id"] != can_id:
        problems.append(f"ID: expected 0x{can_id:03X}, received 0x{frame['id']:X}")
    if frame["extended"]:
        problems.append("Unexpected extended ID")
    if frame["fd"] != is_fd:
        problems.append(f"FDF: expected {is_fd}, received {frame['fd']}")
    if frame["brs"] != brs_expected:
        problems.append(f"BRS: expected {brs_expected}, received {frame['brs']}")
    if frame["length"] != len(payload):
        problems.append(
            f"DLC length: expected {len(payload)}, received {frame['length']}"
        )
    if frame["data"] != payload:
        problems.append(
            f"DATA: expected {payload.hex(' ')}, received {frame['data'].hex(' ')}"
        )
    if problems:
        raise AssertionError(
            f"{label} ID=0x{can_id:03X} failed: " + "; ".join(problems)
        )

    print(
        f"  PASS {label:7} ID=0x{can_id:03X} len={len(payload):2} "
        f"FDF={int(is_fd)} BRS={int(brs_expected)} data={payload.hex(' ')}"
    )


def patterned_payload(length, salt):
    # Nontrivial bytes to detect missing, swapped or overwritten bytes.
    return bytes(((i * 37 + salt * 13 + 0xA5) & 0xFF) for i in range(length))


def main():
    client = ControllerClient(HOST, PORT)
    passed = 0
    try:
        client.connect()

        # Stage 1: Classic + FD across DLC boundaries, BRS disabled.
        configure(client, brs=False)
        drain_mailbox(client)
        for n, length in enumerate((0, 4, 8)):
            test_frame(client, 0x200 + n, patterned_payload(length, n), False, False)
            passed += 1
        for n, length in enumerate((0, 4, 8, 12, 16, 20, 24, 32, 48)):
            test_frame(
                client, 0x300 + n, patterned_payload(length, 10 + n), True, False
            )
            passed += 1

        # Stage 2: enable BRS. Classic frame must still report BRS=0.
        configure(client, brs=True)
        drain_mailbox(client)
        test_frame(client, 0x400, patterned_payload(8, 30), False, False)
        passed += 1
        for n, length in enumerate((12, 48)):
            test_frame(client, 0x410 + n, patterned_payload(length, 40 + n), True, True)
            passed += 1

        print(f"\nSUCCESS: {passed} CAN2 External Loopback tests passed.")
        print("Normal mode / external transceiver and ACK are NOT tested.")
    finally:
        client.close()


if __name__ == "__main__":
    main()
