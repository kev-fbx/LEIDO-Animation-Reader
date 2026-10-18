package pzanimator.agent;

import net.bytebuddy.asm.Advice;

/**
 * Inlined at the end of {@code IsoGameCharacter.postupdate()}. At that point the character's
 * {@code AnimationPlayer} has been updated for this tick (see {@code postUpdateAnimating}), so its
 * model transforms are final.
 */
final class PostUpdateAdvice {
    private PostUpdateAdvice() {}

    @Advice.OnMethodExit(suppress = Throwable.class)
    static void exit(@Advice.This Object character) {
        Bridge.onPostUpdate(character);
    }
}
