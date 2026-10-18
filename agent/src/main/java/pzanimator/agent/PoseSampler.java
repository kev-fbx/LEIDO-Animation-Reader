package pzanimator.agent;

import java.nio.ByteBuffer;
import java.util.List;

import org.lwjgl.util.vector.Matrix4f;

import zombie.characters.IsoPlayer;
import zombie.core.skinnedmodel.animation.AnimationPlayer;
import zombie.core.skinnedmodel.model.ModelInstance;
import zombie.core.skinnedmodel.model.SkinningBone;
import zombie.core.skinnedmodel.model.SkinningData;

/**
 * Copies the local player's final model-space bone matrices once per tick (game thread only).
 *
 * The copy is taken under {@code ModelInstance.lock}, the lock {@code ModelManager.ModelSlot.Update}
 * holds while it runs {@code AnimationPlayer.Update}.
 */
final class PoseSampler {
    private SkinningData currentSkeleton;
    private int skeletonId;
    private long seq;

    void sample(IsoPlayer player, Dispatcher out) {
        AnimationPlayer anim = player.getAnimationPlayer();
        if (anim == null || !anim.isReady()) {
            return;
        }
        SkinningData skinning = anim.getSkinningData();
        if (skinning == null) {
            return;
        }
        if (skinning != currentSkeleton) {
            currentSkeleton = skinning;
            skeletonId++;
            out.publishSkeleton(Messages.json(Messages.SKELETON, skeletonJson(skinning, skeletonId)));
            AgentLog.info("Skeleton " + skeletonId + ": " + skinning.numBones() + " bones");
        }

        ModelInstance model = player.getModelInstance();
        Object lock = model != null ? model.lock : anim;
        ByteBuffer frame;
        synchronized (lock) {
            int boneCount = anim.getModelTransformsCount();
            frame = Messages.begin(Messages.FRAME, 8 + 8 + 4 * 5 + 4 + 2 + 2 + boneCount * 64);
            frame.putLong(seq++);
            frame.putLong(System.nanoTime());
            frame.putFloat(player.getAnimationTimeDelta());
            frame.putFloat(player.getX());
            frame.putFloat(player.getY());
            frame.putFloat(player.getZ());
            frame.putFloat(anim.getRenderedAngle());
            frame.putInt(skeletonId);
            frame.putShort((short) (out.isRecording() ? 1 : 0));
            frame.putShort((short) boneCount);
            for (int i = 0; i < boneCount; i++) {
                putMatrix(frame, anim.getModelTransformAt(i));
            }
        }
        out.publishFrame(frame.array());
    }

    private static void putMatrix(ByteBuffer buf, Matrix4f m) {
        buf.putFloat(m.m00).putFloat(m.m01).putFloat(m.m02).putFloat(m.m03);
        buf.putFloat(m.m10).putFloat(m.m11).putFloat(m.m12).putFloat(m.m13);
        buf.putFloat(m.m20).putFloat(m.m21).putFloat(m.m22).putFloat(m.m23);
        buf.putFloat(m.m30).putFloat(m.m31).putFloat(m.m32).putFloat(m.m33);
    }

    private static String skeletonJson(SkinningData skinning, int id) {
        Json json = new Json().beginObject().field("skeletonId", id);
        json.name("bones").beginArray();
        for (int i = 0; i < skinning.numBones(); i++) {
            SkinningBone bone = skinning.getBoneAt(i);
            json.beginObject()
                    .field("name", bone.name)
                    .field("parent", skinning.getParentBoneIdx(i))
                    .endObject();
        }
        json.endArray();
        matrixList(json, "bindPose", skinning.bindPose);
        matrixList(json, "boneOffset", skinning.boneOffset);
        return json.endObject().toString();
    }

    private static void matrixList(Json json, String name, List<Matrix4f> matrices) {
        json.name(name).beginArray();
        if (matrices != null) {
            for (Matrix4f m : matrices) {
                if (m == null) {
                    json.nullValue();
                    continue;
                }
                json.floats(m.m00, m.m01, m.m02, m.m03, m.m10, m.m11, m.m12, m.m13,
                        m.m20, m.m21, m.m22, m.m23, m.m30, m.m31, m.m32, m.m33);
            }
        }
        json.endArray();
    }
}
