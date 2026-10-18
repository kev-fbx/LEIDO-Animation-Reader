package pzanimator.agent;

import java.util.Set;

/** Constants shared across the agent. */
final class AgentInfo {
    private AgentInfo() {}

    static final String VERSION = "0.1.0";
    static final int PROTOCOL = 1;
    static final int DEFAULT_PORT = 47400;

    static final String HOOK_CLASS = "zombie.characters.IsoGameCharacter";
    static final String HOOK_METHOD = "postupdate";

    /**
     * Builds whose hook point and matrix conventions have been verified against decompiled source.
     * Format: {@code Core.getGameAndBuildVersion() + " " + Core.getGitRevisionString()}.
     */
    static final Set<String> VERIFIED_BUILDS = Set.of("42.21.0 4a0e9546ec");
}
