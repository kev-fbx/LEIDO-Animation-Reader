package pzanimator.agent;

import zombie.characters.IsoPlayer;
import zombie.core.Core;
import zombie.network.GameClient;
import zombie.network.GameServer;

/**
 * Game-thread entry point called from the inlined {@link PostUpdateAdvice}. Must be public: the
 * advice code lives inside {@code zombie.characters.IsoGameCharacter} after inlining.
 *
 * Everything here runs on the game thread, so it only samples and hands bytes to the
 * {@link Dispatcher}; all socket and file work happens on the dispatcher's own thread.
 */
public final class Bridge {
    private Bridge() {}

    private static final int STATS_EVERY_TICKS = 600;

    private static Dispatcher dispatcher;
    private static PoseSampler sampler;
    private static boolean helloSent;
    private static boolean multiplayerWarned;

    private static long statTicks;
    private static long statNanos;
    private static long statMaxNanos;

    static void start(AgentConfig config) {
        dispatcher = new Dispatcher(config);
        sampler = new PoseSampler();
        dispatcher.start();
    }

    public static void onPostUpdate(Object character) {
        if (dispatcher == null || character != IsoPlayer.players[0]) {
            return;
        }
        if (GameClient.client || GameServer.server) {
            if (!multiplayerWarned) {
                multiplayerWarned = true;
                AgentLog.warn("Multiplayer session detected; capture is disabled (single-player only)");
            }
            return;
        }

        long start = System.nanoTime();
        try {
            if (!helloSent) {
                helloSent = true;
                sendGameHello();
            }
            sampler.sample((IsoPlayer) character, dispatcher);
        } catch (Throwable t) {
            AgentLog.error("Sampling failed; capture disabled for this session", t);
            dispatcher = null;
            return;
        }
        recordStats((IsoPlayer) character, System.nanoTime() - start);
    }

    /** Reads the game build on the game thread, where touching {@link Core} is safe. */
    private static void sendGameHello() {
        String game = Core.getInstance().getGameAndBuildVersion();
        String revision = Core.getGitRevisionString();
        boolean supported = AgentInfo.VERIFIED_BUILDS.contains(game + " " + revision);
        if (supported) {
            AgentLog.info("Game build " + game + " " + revision + " (verified)");
        } else {
            AgentLog.warn("Game build " + game + " " + revision + " is not verified; capture may be wrong."
                    + " Verified: " + AgentInfo.VERIFIED_BUILDS);
        }
        dispatcher.setGameInfo(game, revision, supported);
    }

    private static void recordStats(IsoPlayer player, long nanos) {
        statTicks++;
        statNanos += nanos;
        statMaxNanos = Math.max(statMaxNanos, nanos);
        if (statTicks % STATS_EVERY_TICKS == 0) {
            AgentLog.info(String.format("ticks=%d hook avg=%.1fus max=%.1fus pos=(%.2f, %.2f, %.2f) dropped=%d",
                    statTicks, statNanos / 1000.0 / STATS_EVERY_TICKS, statMaxNanos / 1000.0,
                    player.getX(), player.getY(), player.getZ(), dispatcher.droppedMessages()));
            statNanos = 0;
            statMaxNanos = 0;
        }
    }
}
