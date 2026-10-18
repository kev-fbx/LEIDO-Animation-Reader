package pzanimator.agent;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

/** Minimal logger: writes to {@code <home>/agent.log} and to the game console. */
final class AgentLog {
    private AgentLog() {}

    private static final DateTimeFormatter TIME = DateTimeFormatter.ofPattern("HH:mm:ss.SSS");
    private static Writer file;

    static synchronized void init(Path home) {
        try {
            Files.createDirectories(home);
            file = Files.newBufferedWriter(home.resolve("agent.log"), StandardCharsets.UTF_8,
                    StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING, StandardOpenOption.WRITE);
        } catch (IOException e) {
            System.out.println("[pz-animator] could not open log file: " + e);
        }
    }

    static void info(String message) {
        write("INFO ", message);
    }

    static void warn(String message) {
        write("WARN ", message);
    }

    static void error(String message, Throwable t) {
        StringWriter trace = new StringWriter();
        t.printStackTrace(new PrintWriter(trace));
        write("ERROR", message + System.lineSeparator() + trace);
    }

    private static synchronized void write(String level, String message) {
        String line = LocalDateTime.now().format(TIME) + " " + level + " " + message;
        System.out.println("[pz-animator] " + line);
        if (file != null) {
            try {
                file.write(line);
                file.write(System.lineSeparator());
                file.flush();
            } catch (IOException ignored) {
                // Logging must never take the game down.
            }
        }
    }
}
