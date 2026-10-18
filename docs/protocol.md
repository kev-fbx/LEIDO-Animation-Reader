# LEIDO PZ Animation Exporter wire protocol (v1)

The in-game agent and the desktop app talk over one TCP connection on
`127.0.0.1` (default port `47400`). The agent is the server and accepts one
client at a time. Take files (`.pzrec`) store the same messages back to back.

## Framing

All integers and floats are **little-endian**.

```
u32  length        number of bytes that follow (type + payload)
u8   type
...  payload       length - 1 bytes
```

JSON payloads are UTF-8 without a terminator.

## Agent -> app

| Type | Name | Payload | When |
| --- | --- | --- | --- |
| `0x01` | HELLO | JSON | Once, right after the app connects |
| `0x02` | SKELETON | JSON | After HELLO if a skeleton is known, and whenever the player's skeleton changes |
| `0x03` | FRAME | binary | Every game tick of the local player (player 0) |
| `0x04` | STATUS | JSON | After a command, and when recording starts or stops |
| `0x10` | STATE | JSON | Reserved: action/animation state, `Weapon` variable, hand items |
| `0x11` | ITEMS | binary | Reserved: held-item matrices |
| `0x12` | OBSTACLES | binary | Reserved: changed squares around the player |

### HELLO

```json
{"protocol":1,"agent":"0.1.0","game":"42.21.0","revision":"4a0e9546ec","supported":true,"port":47400}
```

`supported` is `false` when the running build is not in the agent's verified
list. The agent still runs; the app should show a warning.

### SKELETON

```json
{
  "skeletonId": 1,
  "bones": [{"name": "Dummy01", "parent": -1}, {"name": "Bip01", "parent": 0}],
  "bindPose": [[16 floats], ...],
  "boneOffset": [[16 floats], ...]
}
```

* `bones[i]` matches matrix `i` in every FRAME with the same `skeletonId`.
* `bindPose` is `SkinningData.bindPose` (per-bone, parent-relative).
* `boneOffset` is `SkinningData.boneOffset` (the mesh's inverse bind matrices).
  May be shorter than `bones` or contain `null` entries.

### FRAME

```
u64  seq              increments every frame the agent produces
i64  monoNanos        System.nanoTime() on the game thread
f32  animDelta        IsoGameCharacter.getAnimationTimeDelta()
f32  x, y, z          character world position (tiles, tiles, levels)
f32  renderedAngle    AnimationPlayer.getRenderedAngle() (radians)
u32  skeletonId
u16  flags            bit 0 = recording
u16  boneCount
f32  matrices[boneCount * 16]
```

### Matrix layout

Each matrix is the 16 public fields of `org.lwjgl.util.vector.Matrix4f` in this
order:

```
m00 m01 m02 m03  m10 m11 m12 m13  m20 m21 m22 m23  m30 m31 m32 m33
```

PZ stores bone matrices so that **translation is in `m03`, `m13`, `m23`**
(see `Model.boneToWorldCoords`), and composes them as
`model[i] = local[i] * model[parent]`.

FRAME matrices are `AnimationPlayer.modelTransforms`: model space, Y-up. To
reach world space the game mirrors X, rotates about Y by `renderedAngle`,
swaps Y and Z, scales x and y by 1.5 and z by 0.61237, then adds the
character position (`Model.vectorToWorldCoords`).

### STATUS

```json
{"recording":true,"take":"C:/.../2026-10-13_201500.pzrec","framesWritten":0,"droppedFrames":0}
```

## App -> agent

| Type | Name | Payload |
| --- | --- | --- |
| `0x81` | RECORD_START | JSON, optional: `{"dir":"C:/takes","name":"walk_test"}` |
| `0x82` | RECORD_STOP | empty |
| `0x83` | PING | empty; the agent answers with STATUS |

## `.pzrec` take files

```
8 bytes   magic "PZREC" 0x00 0x01 0x00   (format version 1)
...       HELLO, SKELETON, then FRAME messages, framed exactly as on the wire
```

A SKELETON message may appear again mid-file if the skeleton changed.
