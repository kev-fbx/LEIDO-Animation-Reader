package pzanimator.agent;

import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * Agent options from the {@code -javaagent:...=key=value,key=value} argument.
 *
 * <ul>
 *   <li>{@code port} - TCP port on 127.0.0.1 (default 47400)</li>
 *   <li>{@code home} - folder for logs and default takes (default %USERPROFILE%/Zomboid/pz-animator)</li>
 * </ul>
 */
record AgentConfig(int port, Path home) {

    static AgentConfig parse(String args) {
        int port = AgentInfo.DEFAULT_PORT;
        Path home = Paths.get(System.getProperty("user.home"), "Zomboid", "pz-animator");
        if (args != null && !args.isBlank()) {
            for (String pair : args.split(",")) {
                int eq = pair.indexOf('=');
                if (eq <= 0) {
                    continue;
                }
                String key = pair.substring(0, eq).trim();
                String value = pair.substring(eq + 1).trim();
                switch (key) {
                    case "port" -> port = Integer.parseInt(value);
                    case "home" -> home = Paths.get(value);
                    default -> { }
                }
            }
        }
        return new AgentConfig(port, home);
    }

    Path takesDir() {
        return home.resolve("takes");
    }
}
