package pzanimator.agent;

import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;

/** Writes a {@code .pzrec} take: magic, then messages exactly as framed on the wire. Dispatcher thread only. */
final class TakeRecorder {
    private static final byte[] MAGIC = {'P', 'Z', 'R', 'E', 'C', 0, 1, 0};

    private final Path path;
    private final OutputStream out;
    private long frames;
    private boolean failed;
    private boolean closed;

    private TakeRecorder(Path path, OutputStream out) {
        this.path = path;
        this.out = out;
    }

    static TakeRecorder open(Path path, byte[] hello, byte[] skeleton) throws IOException {
        Files.createDirectories(path.getParent());
        TakeRecorder take = new TakeRecorder(path, new BufferedOutputStream(Files.newOutputStream(path), 256 * 1024));
        take.out.write(MAGIC);
        take.write(hello);
        if (skeleton != null) {
            take.write(skeleton);
        }
        return take;
    }

    synchronized void write(byte[] message) {
        if (failed || closed) {
            return;
        }
        try {
            out.write(message);
            if (Messages.type(message) == Messages.FRAME) {
                frames++;
            }
        } catch (IOException e) {
            failed = true;
            AgentLog.error("Take write failed: " + path, e);
        }
    }

    /** Idempotent; may be called from the shutdown hook while the dispatcher is still running. */
    synchronized void close() {
        if (closed) {
            return;
        }
        closed = true;
        try {
            out.close();
        } catch (IOException e) {
            AgentLog.error("Take close failed: " + path, e);
        }
    }

    Path path() {
        return path;
    }

    synchronized long framesWritten() {
        return frames;
    }
}
