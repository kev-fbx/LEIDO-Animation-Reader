package pzanimator.agent;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;

/** Message framing from docs/protocol.md: {@code u32 length | u8 type | payload}, little-endian. */
final class Messages {
    private Messages() {}

    // Agent -> app
    static final byte HELLO = 0x01;
    static final byte SKELETON = 0x02;
    static final byte FRAME = 0x03;
    static final byte STATUS = 0x04;

    // App -> agent
    static final byte RECORD_START = (byte) 0x81;
    static final byte RECORD_STOP = (byte) 0x82;
    static final byte PING = (byte) 0x83;

    static final int MAX_INCOMING = 64 * 1024;

    /** Allocates a buffer for one whole message and writes its header. */
    static ByteBuffer begin(byte type, int payloadLength) {
        ByteBuffer buf = ByteBuffer.allocate(4 + 1 + payloadLength).order(ByteOrder.LITTLE_ENDIAN);
        buf.putInt(1 + payloadLength);
        buf.put(type);
        return buf;
    }

    static byte[] json(byte type, String json) {
        byte[] payload = json.getBytes(StandardCharsets.UTF_8);
        return begin(type, payload.length).put(payload).array();
    }

    static byte type(byte[] message) {
        return message[4];
    }
}
