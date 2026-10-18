"""Headless import check for LEIDO PZ Animation Exporter GLBs (Blender 4.x and 5.x).

    blender -b --factory-startup --python tools/blender_check.py -- <take.glb> <output_dir>

Imports the GLB into an empty scene, prints what Blender built (armature, action, mesh, texture),
measures the skinned body at a few frames, and renders a front view of each to PNG.
"""

import math
import os
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
glb_path, out_dir = os.path.abspath(argv[0]), os.path.abspath(argv[1])
os.makedirs(out_dir, exist_ok=True)
tag = "blender%d.%d" % bpy.app.version[:2]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)
scene = bpy.context.scene

armatures = [o for o in scene.objects if o.type == "ARMATURE"]
meshes = [o for o in scene.objects if o.type == "MESH"]
print("[%s] objects: %s" % (tag, ", ".join("%s(%s)" % (o.name, o.type) for o in scene.objects)))
if not armatures or not meshes:
    sys.exit("[%s] FAIL: missing armature or mesh" % tag)
arm, body = armatures[0], meshes[0]
action = arm.animation_data.action if arm.animation_data else None
start, end = (int(action.frame_range[0]), int(action.frame_range[1])) if action else (1, 1)
print("[%s] armature %s: %d bones; action %s frames %d-%d; scene fps %d"
      % (tag, arm.name, len(arm.data.bones), action.name if action else None, start, end, scene.render.fps))
images = [img.name for img in bpy.data.images]
print("[%s] mesh %s: %d vertices, modifiers %s, images %s"
      % (tag, body.name, len(body.data.vertices), [m.type for m in body.modifiers], images))

# Root motion lives on the armature's parent empty (PZ_Root) when Blender keeps it as an object.
root = arm.parent


def bone_world(name):
    pb = arm.pose.bones[name]
    return arm.matrix_world @ pb.head


def evaluated_vertices():
    deps = bpy.context.evaluated_depsgraph_get()
    ev = body.evaluated_get(deps)
    mesh = ev.to_mesh()
    verts = [ev.matrix_world @ v.co for v in mesh.vertices]
    ev.to_mesh_clear()
    return verts


# Workbench render with textures, neutral background.
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "TEXTURE"
scene.render.resolution_x, scene.render.resolution_y = 480, 640
scene.render.image_settings.file_format = "PNG"
cam_data = bpy.data.cameras.new("CheckCam")
cam_data.lens = 50
cam = bpy.data.objects.new("CheckCam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
world = bpy.data.worlds.new("CheckWorld")
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.05, 0.05, 0.05, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
scene.world = world
sun = bpy.data.objects.new("CheckSun", bpy.data.lights.new("CheckSun", "SUN"))
sun.data.energy = 3.0
sun.rotation_euler = (0.9, 0.0, 0.6)
scene.collection.objects.link(sun)

frames = sorted({start, (start + end) // 2, start + (end - start) * 3 // 4})
for frame in frames:
    scene.frame_set(frame)
    verts = evaluated_vertices()
    lo = Vector((min(v.x for v in verts), min(v.y for v in verts), min(v.z for v in verts)))
    hi = Vector((max(v.x for v in verts), max(v.y for v in verts), max(v.z for v in verts)))
    head = bone_world("Bip01_Head")
    hand = bone_world("Bip01_R_Hand")
    nearest_head = min((v - head).length for v in verts)
    nearest_hand = min((v - hand).length for v in verts)
    # Facing: average foot->toe direction, flattened.
    fwd = Vector((0, 0, 0))
    for side in "LR":
        fwd += bone_world("Bip01_%s_Toe0" % side) - bone_world("Bip01_%s_Foot" % side)
    fwd.z = 0
    fwd = fwd.normalized() if fwd.length > 1e-6 else Vector((0, -1, 0))
    print("[%s] frame %d: body height %.3f m, bbox %s..%s, head joint %.3f m from nearest vertex, "
          "hand %.3f m; root at %s"
          % (tag, frame, hi.z - lo.z, tuple(round(c, 2) for c in lo), tuple(round(c, 2) for c in hi),
             nearest_head, nearest_hand,
             tuple(round(c, 2) for c in (root.matrix_world.translation if root else arm.matrix_world.translation))))

    centre = (lo + hi) / 2
    side = Vector((fwd.y, -fwd.x, 0))
    views = (("front", fwd * 4.0, "TEXTURE"), ("solid", fwd * 4.0, "SINGLE"), ("side", side * 4.0, "SINGLE"))
    for label, offset, colour in views:
        cam.location = centre + offset + Vector((0, 0, 0.3))
        cam.rotation_euler = (centre - cam.location).to_track_quat("-Z", "Y").to_euler()
        # Workbench ignores alpha clipping, which hides the body's dress panel; use EEVEE for the
        # textured view ("BLENDER_EEVEE_NEXT" in 4.2-4.5, "BLENDER_EEVEE" in 5.x).
        engines = scene.render.bl_rna.properties["engine"].enum_items.keys()
        scene.render.engine = (("BLENDER_EEVEE_NEXT" if "BLENDER_EEVEE_NEXT" in engines else "BLENDER_EEVEE")
                               if colour == "TEXTURE" else "BLENDER_WORKBENCH")
        scene.display.shading.color_type = colour
        scene.render.filepath = os.path.join(out_dir, "%s_frame%04d_%s.png" % (tag, frame, label))
        bpy.ops.render.render(write_still=True)
    print("[%s] rendered frame %d (front, solid, side)" % (tag, frame))

print("[%s] DONE" % tag)
