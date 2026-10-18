package pzanimator.agent;

import java.lang.instrument.Instrumentation;

import net.bytebuddy.agent.builder.AgentBuilder;
import net.bytebuddy.asm.Advice;
import net.bytebuddy.description.type.TypeDescription;
import net.bytebuddy.dynamic.DynamicType;
import net.bytebuddy.utility.JavaModule;

import static net.bytebuddy.matcher.ElementMatchers.named;
import static net.bytebuddy.matcher.ElementMatchers.takesNoArguments;

/**
 * Java agent entry point, loaded with {@code -javaagent:pz-animator-agent.jar=port=47400,home=...}.
 *
 * Nothing here may touch {@code zombie.*} classes: premain runs before the game's own main, and
 * initialising game classes this early can break start-up. All game access happens later, on the
 * game thread, from {@link Bridge}.
 */
public final class AgentMain {
    private AgentMain() {}

    public static void premain(String args, Instrumentation inst) {
        AgentConfig config = AgentConfig.parse(args);
        AgentLog.init(config.home());
        AgentLog.info("LEIDO PZ Animation Exporter agent " + AgentInfo.VERSION + " starting (protocol " + AgentInfo.PROTOCOL
                + ", java " + System.getProperty("java.version") + ", args=" + args + ")");

        try {
            new AgentBuilder.Default()
                    .with(new HookListener())
                    .type(named(AgentInfo.HOOK_CLASS))
                    .transform((builder, type, loader, module, domain) -> builder.visit(
                            Advice.to(PostUpdateAdvice.class).on(named(AgentInfo.HOOK_METHOD).and(takesNoArguments()))))
                    .installOn(inst);
            Bridge.start(config);
        } catch (Throwable t) {
            AgentLog.error("Agent failed to start; the game will run without it", t);
        }
    }

    private static final class HookListener extends AgentBuilder.Listener.Adapter {
        @Override
        public void onTransformation(TypeDescription type, ClassLoader loader, JavaModule module, boolean loaded,
                                     DynamicType dynamicType) {
            AgentLog.info("Hooked " + type.getName() + "." + AgentInfo.HOOK_METHOD + "()");
        }

        @Override
        public void onError(String typeName, ClassLoader loader, JavaModule module, boolean loaded, Throwable error) {
            AgentLog.error("Failed to hook " + typeName, error);
        }
    }
}
