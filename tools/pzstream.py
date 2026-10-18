"""Command-line client for the LEIDO PZ Animation Exporter agent (protocol v1, see docs/protocol.md).

Used for Experiments 0 and 1 before the desktop app exists. Standard library only.

    python tools/pzstream.py live                    # print agent info and live stats
    python tools/pzstream.py live --record 10        # record a 10 s take, then stop
    python tools/pzstream.py read runtime/takes/x.pzrec [--csv bones.csv]
"""

import argparse
import json
import socket
import struct
import sys
import time

HELLO, SKELETON, FRAME, STATUS = 0x01, 0x02, 0x03, 0x04
RECORD_START, RECORD_STOP, PING = 0x81, 0x82, 0x83
FRAME_HEADER = struct.Struct("<QqfffffIHH")
PZREC_MAGIC = b"PZREC\x00\x01\x00"


def parse_frame(payload):
    seq, mono, anim_dt, x, y, z, angle, skeleton_id, flags, bones = FRAME_HEADER.unpack_from(payload)
    matrices = struct.unpack_from("<%df" % (bones * 16), payload, FRAME_HEADER.size)
    return {
        "seq": seq, "monoNanos": mono, "animDelta": anim_dt, "pos": (x, y, z), "angle": angle,
        "skeletonId": skeleton_id, "recording": bool(flags & 1), "bones": bones, "matrices": matrices,
    }


def read_messages(read_exact):
    """Yields (type, payload) from a callable that returns exactly n bytes or raises EOFError."""
    while True:
        (length,) = struct.unpack("<I", read_exact(4))
        body = read_exact(length)
        yield body[0], body[1:]


def socket_reader(sock, on_idle):
    """Blocking exact reads; calls on_idle() whenever the socket has been quiet for its timeout."""
    def read_exact(n):
        chunks = []
        while n:
            try:
                chunk = sock.recv(n)
            except socket.timeout:
                on_idle()
                continue
            if not chunk:
                raise EOFError
            chunks.append(chunk)
            n -= len(chunk)
        return b"".join(chunks)
    return read_exact


def send(sock, msg_type, payload=b""):
    sock.sendall(struct.pack("<IB", len(payload) + 1, msg_type) + payload)


def describe_skeleton(skel):
    names = [b["name"] for b in skel["bones"]]
    return "skeleton %d: %d bones (%s ... %s)" % (skel["skeletonId"], len(names), ", ".join(names[:3]), names[-1])


def cmd_live(args):
    sock = socket.create_connection(("127.0.0.1", args.port), timeout=10)
    sock.settimeout(0.25)
    print("connected to 127.0.0.1:%d" % args.port)
    state = {"record_until": None, "window_start": time.monotonic(), "frames": 0, "last": None}
    if args.record:
        send(sock, RECORD_START, json.dumps({"name": args.name} if args.name else {}).encode())
        state["record_until"] = time.monotonic() + args.record

    def tick():
        now = time.monotonic()
        if now - state["window_start"] >= 1.0:
            last = state["last"]
            if last:
                x, y, z = last["pos"]
                print("%5.1f fps  seq=%d  pos=(%.2f, %.2f, %.2f)  angle=%.2f  bones=%d%s" % (
                    state["frames"] / (now - state["window_start"]), last["seq"], x, y, z, last["angle"],
                    last["bones"], "  [REC]" if last["recording"] else ""))
            else:
                print("connected; no frames yet (load a single-player save)")
            state["window_start"], state["frames"] = now, 0
        if state["record_until"] and now >= state["record_until"]:
            send(sock, RECORD_STOP)
            state["record_until"] = None

    try:
        for msg_type, payload in read_messages(socket_reader(sock, tick)):
            if msg_type == HELLO:
                print("HELLO", payload.decode())
            elif msg_type == SKELETON:
                print(describe_skeleton(json.loads(payload)))
            elif msg_type == STATUS:
                print("STATUS", payload.decode())
            elif msg_type == FRAME:
                state["last"] = parse_frame(payload)
                state["frames"] += 1
            tick()
    except (EOFError, ConnectionError):
        print("agent closed the connection")
    except KeyboardInterrupt:
        pass


def cmd_read(args):
    with open(args.file, "rb") as f:
        data = f.read()
    if not data.startswith(PZREC_MAGIC):
        sys.exit("not a .pzrec v1 file")
    pos = len(PZREC_MAGIC)

    def read_exact(n):
        nonlocal pos
        if pos + n > len(data):
            raise EOFError
        chunk = data[pos:pos + n]
        pos += n
        return chunk

    frames, skeleton = [], None
    try:
        for msg_type, payload in read_messages(read_exact):
            if msg_type == HELLO:
                print("HELLO", payload.decode())
            elif msg_type == SKELETON:
                skeleton = json.loads(payload)
                print(describe_skeleton(skeleton))
            elif msg_type == FRAME:
                frames.append(parse_frame(payload))
    except EOFError:
        pass
    if not frames:
        sys.exit("no frames")
    seconds = (frames[-1]["monoNanos"] - frames[0]["monoNanos"]) / 1e9
    gaps = sum(1 for a, b in zip(frames, frames[1:]) if b["seq"] != a["seq"] + 1)
    print("%d frames over %.2f s (%.1f fps), %d sequence gaps" % (
        len(frames), seconds, (len(frames) - 1) / seconds if seconds else 0, gaps))
    print("start pos=(%.2f, %.2f, %.2f)  end pos=(%.2f, %.2f, %.2f)" % (frames[0]["pos"] + frames[-1]["pos"]))
    if args.csv and skeleton:
        names = [b["name"] for b in skeleton["bones"]]
        with open(args.csv, "w") as out:
            out.write("seq,seconds,bone,tx,ty,tz\n")
            t0 = frames[0]["monoNanos"]
            for fr in frames:
                m = fr["matrices"]
                for i in range(fr["bones"]):
                    # Translation lives in m03, m13, m23 (indices 3, 7, 11).
                    out.write("%d,%.4f,%s,%.5f,%.5f,%.5f\n" % (
                        fr["seq"], (fr["monoNanos"] - t0) / 1e9, names[i] if i < len(names) else i,
                        m[i * 16 + 3], m[i * 16 + 7], m[i * 16 + 11]))
        print("wrote", args.csv)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    live = sub.add_parser("live", help="connect to the running agent")
    live.add_argument("--port", type=int, default=47400)
    live.add_argument("--record", type=float, metavar="SECONDS", help="record a take of this length")
    live.add_argument("--name", help="take file name (without .pzrec)")
    live.set_defaults(func=cmd_live)
    read = sub.add_parser("read", help="summarise a .pzrec take")
    read.add_argument("file")
    read.add_argument("--csv", help="write per-bone translations to a CSV file")
    read.set_defaults(func=cmd_read)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
