package pzanimator.agent;

import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Owns all I/O. The game thread only calls the non-blocking {@code publish*} methods; a single
 * dispatcher thread drains the queue and writes to the connected app and to the take file. An
 * accept thread and one reader thread per client feed commands into the same queue, so all
 * connection and recording state is touched by the dispatcher thread alone.
 */
final class Dispatcher {
    private static final int QUEUE_CAPACITY = 1024;
    private static final DateTimeFormatter TAKE_NAME = DateTimeFormatter.ofPattern("yyyy-MM-dd_HHmmss");

    private final AgentConfig config;
    private final BlockingQueue<Object> queue = new ArrayBlockingQueue<>(QUEUE_CAPACITY);
    private final AtomicLong dropped = new AtomicLong();
    private volatile boolean recording;

    // Dispatcher-thread state
    private byte[] hello;
    private byte[] skeleton;
    private Socket client;
    private OutputStream clientOut;
    private volatile TakeRecorder take; // also read by the shutdown hook
    private TakeRecorder lastTake;

    Dispatcher(AgentConfig config) {
        this.config = config;
        this.hello = helloMessage(null, null, false);
    }

    void start() {
        Thread io = new Thread(this::run, "pz-animator-io");
        io.setDaemon(true);
        io.start();
        Thread accept = new Thread(this::acceptLoop, "pz-animator-accept");
        accept.setDaemon(true);
        accept.start();
        // Quitting the game mid-take must still leave a readable file.
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            TakeRecorder open = take;
            if (open != null) {
                open.close();
                AgentLog.info("Closed take on exit: " + open.path());
            }
        }, "pz-animator-shutdown"));
    }

    // ---- Called from the game thread (never block) ----

    void publishFrame(byte[] message) {
        offer(message);
    }

    void publishSkeleton(byte[] message) {
        // Skeletons must not be lost, or later frames cannot be interpreted.
        enqueueControl(() -> {
            skeleton = message;
            send(message);
            if (take != null) {
                take.write(message);
            }
        });
    }

    void setGameInfo(String game, String revision, boolean supported) {
        enqueueControl(() -> {
            hello = helloMessage(game, revision, supported);
            send(hello);
        });
    }

    boolean isRecording() {
        return recording;
    }

    long droppedMessages() {
        return dropped.get();
    }

    private void offer(Object item) {
        if (!queue.offer(item)) {
            dropped.incrementAndGet();
        }
    }

    private void enqueueControl(Runnable action) {
        try {
            queue.put(action);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    // ---- Dispatcher thread ----

    private void run() {
        while (true) {
            try {
                Object item = queue.take();
                if (item instanceof Runnable action) {
                    action.run();
                } else {
                    byte[] message = (byte[]) item;
                    if (take != null) {
                        take.write(message);
                    }
                    send(message);
                }
                // Frames are buffered; flush once the backlog is drained to keep viewport latency low.
                if (queue.isEmpty()) {
                    flushClient();
                }
            } catch (InterruptedException e) {
                return;
            } catch (Throwable t) {
                AgentLog.error("Dispatcher error", t);
            }
        }
    }

    private void send(byte[] message) {
        if (clientOut == null) {
            return;
        }
        try {
            clientOut.write(message);
            if (Messages.type(message) != Messages.FRAME) {
                clientOut.flush();
            }
        } catch (IOException e) {
            AgentLog.info("App disconnected: " + e.getMessage());
            closeClient();
            stopTake();
        }
    }

    private void flushClient() {
        if (clientOut != null) {
            try {
                clientOut.flush();
            } catch (IOException e) {
                closeClient();
            }
        }
    }

    private void onClientConnected(Socket socket) throws IOException {
        closeClient();
        socket.setTcpNoDelay(true);
        client = socket;
        clientOut = new BufferedOutputStream(socket.getOutputStream(), 64 * 1024);
        AgentLog.info("App connected from " + socket.getRemoteSocketAddress());
        send(hello);
        if (skeleton != null) {
            send(skeleton);
        }
        sendStatus();
        Thread reader = new Thread(() -> readLoop(socket), "pz-animator-reader");
        reader.setDaemon(true);
        reader.start();
    }

    private void closeClient() {
        if (client != null) {
            try {
                client.close();
            } catch (IOException ignored) {
                // already gone
            }
        }
        client = null;
        clientOut = null;
    }

    private void handleCommand(byte type, byte[] payload) {
        switch (type) {
            case Messages.RECORD_START -> startTake(new String(payload, StandardCharsets.UTF_8));
            case Messages.RECORD_STOP -> stopTake();
            case Messages.PING -> { }
            default -> AgentLog.warn("Unknown command 0x" + Integer.toHexString(type & 0xff));
        }
        sendStatus();
    }

    private void startTake(String json) {
        if (take != null) {
            return;
        }
        Path dir = config.takesDir();
        String name = LocalDateTime.now().format(TAKE_NAME);
        String requestedDir = MiniJson.stringField(json, "dir");
        String requestedName = MiniJson.stringField(json, "name");
        if (requestedDir != null && !requestedDir.isBlank()) {
            dir = Path.of(requestedDir);
        }
        if (requestedName != null && !requestedName.isBlank()) {
            name = requestedName.replaceAll("[\\\\/:*?\"<>|]", "_");
        }
        try {
            take = TakeRecorder.open(dir.resolve(name + ".pzrec"), hello, skeleton);
            recording = true;
            AgentLog.info("Recording to " + take.path());
        } catch (IOException e) {
            AgentLog.error("Could not start take in " + dir, e);
        }
    }

    private void stopTake() {
        if (take == null) {
            return;
        }
        recording = false;
        take.close();
        AgentLog.info("Stopped recording: " + take.framesWritten() + " frames -> " + take.path());
        lastTake = take;
        take = null;
    }

    private void sendStatus() {
        TakeRecorder shown = take != null ? take : lastTake;
        String status = new Json().beginObject()
                .field("recording", take != null)
                .field("take", shown != null ? shown.path().toString() : null)
                .field("framesWritten", shown != null ? shown.framesWritten() : 0)
                .field("droppedFrames", dropped.get())
                .endObject().toString();
        send(Messages.json(Messages.STATUS, status));
        flushClient();
    }

    private byte[] helloMessage(String game, String revision, boolean supported) {
        return Messages.json(Messages.HELLO, new Json().beginObject()
                .field("protocol", AgentInfo.PROTOCOL)
                .field("agent", AgentInfo.VERSION)
                .field("game", game)
                .field("revision", revision)
                .field("supported", supported)
                .field("port", config.port())
                .endObject().toString());
    }

    // ---- Network threads ----

    private void acceptLoop() {
        try (ServerSocket server = new ServerSocket(config.port(), 1, InetAddress.getLoopbackAddress())) {
            AgentLog.info("Listening on 127.0.0.1:" + config.port());
            while (true) {
                Socket socket = server.accept();
                enqueueControl(() -> {
                    try {
                        onClientConnected(socket);
                    } catch (IOException e) {
                        AgentLog.error("Could not set up app connection", e);
                    }
                });
            }
        } catch (IOException e) {
            AgentLog.error("Agent server stopped (port " + config.port() + " in use?)", e);
        }
    }

    private void readLoop(Socket socket) {
        try (InputStream raw = socket.getInputStream()) {
            DataInputStream in = new DataInputStream(raw);
            byte[] header = new byte[4];
            while (true) {
                in.readFully(header);
                int length = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN).getInt();
                if (length < 1 || length > Messages.MAX_INCOMING) {
                    throw new IOException("bad message length " + length);
                }
                byte type = in.readByte();
                byte[] payload = new byte[length - 1];
                in.readFully(payload);
                enqueueControl(() -> handleCommand(type, payload));
            }
        } catch (IOException e) {
            enqueueControl(() -> {
                if (client == socket) {
                    AgentLog.info("App disconnected");
                    closeClient();
                    // The app owns takes; without it nobody can press Stop.
                    stopTake();
                }
            });
        }
    }
}
