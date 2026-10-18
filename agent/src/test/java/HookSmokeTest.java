/**
 * Offline check that the agent weaves its advice into IsoGameCharacter.postupdate().
 * Loads the class without initialising it, so no game code runs.
 *
 * Run (see agent/smoke-test.ps1):
 *   java -javaagent:pz-animator-agent.jar=home=<tmp> -Dnet.bytebuddy.dump=<dir> -cp <classes>;projectzomboid.jar HookSmokeTest
 */
public final class HookSmokeTest {
    public static void main(String[] args) throws Exception {
        Class<?> type = Class.forName("zombie.characters.IsoGameCharacter", false, HookSmokeTest.class.getClassLoader());
        System.out.println("Loaded " + type.getName() + " (not initialised)");
        if (args.length == 2 && args[0].equals("--serve")) {
            // Keeps the agent's server up so a client can exercise HELLO/STATUS/RECORD_* without the game.
            Thread.sleep(Long.parseLong(args[1]) * 1000);
        }
    }
}
