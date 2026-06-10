#!/usr/bin/env python3
"""Generate OBJ meshes + scene.json for the SSAO demo pillar-hall scene."""
import json, math, os

OUT = "models/ssao-demo"
os.makedirs(OUT, exist_ok=True)

def write_obj(path, verts, faces):
    with open(path, 'w') as f:
        f.write("# SSAO demo mesh\n")
        for v in verts:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        for face in faces:
            f.write("f " + " ".join(str(i) for i in face) + "\n")

def cube_verts_faces(hx, hy, hz):
    verts = [
        (-hx,-hy,-hz),(+hx,-hy,-hz),(+hx,+hy,-hz),(-hx,+hy,-hz),  # 0: -Z
        (-hx,-hy,+hz),(+hx,-hy,+hz),(+hx,+hy,+hz),(-hx,+hy,+hz),  # 1: +Z
        (-hx,-hy,-hz),(-hx,+hy,-hz),(-hx,+hy,+hz),(-hx,-hy,+hz),  # 2: -X
        (+hx,-hy,-hz),(+hx,+hy,-hz),(+hx,+hy,+hz),(+hx,-hy,+hz),  # 3: +X
        (-hx,-hy,-hz),(-hx,-hy,+hz),(+hx,-hy,+hz),(+hx,-hy,-hz),  # 4: -Y
        (-hx,+hy,-hz),(-hx,+hy,+hz),(+hx,+hy,+hz),(+hx,+hy,-hz),  # 5: +Y
    ]
    faces = []
    for fi in range(6):
        b = fi * 4 + 1
        if fi in (0, 2, 4):  # -Z, -X, -Y: reverse winding for outward normal
            faces += [(b,b+3,b+2),(b,b+2,b+1)]
        else:                # +Z, +X, +Y: standard CCW from outside
            faces += [(b,b+1,b+2),(b,b+2,b+3)]
    return verts, faces

def sphere_verts_faces(r, slices=24, stacks=12):
    verts = [(0.0, r, 0.0)]
    for i in range(1, stacks):
        phi = math.pi * i / stacks
        y = r * math.cos(phi)
        rr = r * math.sin(phi)
        for j in range(slices):
            th = 2.0 * math.pi * j / slices
            verts.append((rr * math.cos(th), y, rr * math.sin(th)))
    verts.append((0.0, -r, 0.0))
    faces = []
    for j in range(slices):
        nj = (j+1) % slices
        faces.append((1, 2+nj, 2+j))  # CW from above → outward +Y normal
    for i in range(stacks-2):
        b = 1 + i*slices
        nb = b + slices
        for j in range(slices):
            nj = (j+1) % slices
            faces += [(b+1+j, b+1+nj, nb+1+j), (b+1+nj, nb+1+nj, nb+1+j)]
    pole = len(verts)
    b = 1 + (stacks-2)*slices
    for j in range(slices):
        nj = (j+1) % slices
        faces.append((b+1+j, b+1+nj, pole))
    return verts, faces

def floor_verts_faces(w, d):
    verts = [(0,0,0),(w,0,0),(w,0,d),(0,0,d)]
    return verts, [(1,4,3),(1,3,2)]  # normal +Y (up)

def wall_xy(w, h):
    verts = [(0,0,0),(w,0,0),(w,h,0),(0,h,0)]
    return verts, [(1,4,3),(1,3,2)]  # normal -Z (into room)

def wall_yz_left(h, d):
    """Left wall at x=0, faces +X into room."""
    verts = [(0,0,0),(0,0,d),(0,h,d),(0,h,0)]
    return verts, [(1,4,3),(1,3,2)]  # normal +X

def wall_yz_right(x, h, d):
    """Right wall at x, faces -X into room."""
    verts = [(x,0,0),(x,0,d),(x,h,d),(x,h,0)]
    return verts, [(1,2,3),(1,3,4)]  # normal -X

def ceiling_quad(w, d, y):
    verts = [(0,y,0),(w,y,0),(w,y,d),(0,y,d)]
    return verts, [(1,3,4),(1,2,3)]

# ── Generate meshes ──
print("Generating meshes...")
write_obj(f"{OUT}/floor.obj",      *floor_verts_faces(600, 600))
write_obj(f"{OUT}/wall_back.obj",  *wall_xy(600, 400))
write_obj(f"{OUT}/wall_left.obj",  *wall_yz_left(400, 600))
write_obj(f"{OUT}/wall_right.obj", *wall_yz_right(600, 400, 600))
write_obj(f"{OUT}/ceiling.obj",    *ceiling_quad(600, 600, 400))
write_obj(f"{OUT}/box_80.obj",     *cube_verts_faces(40, 40, 40))
write_obj(f"{OUT}/box_30.obj",     *cube_verts_faces(15, 15, 15))
write_obj(f"{OUT}/pillar.obj",     *cube_verts_faces(15, 110, 15))
write_obj(f"{OUT}/arch.obj",       *cube_verts_faces(80, 15, 15))
write_obj(f"{OUT}/sphere_40.obj",  *sphere_verts_faces(40, 24, 12))
write_obj(f"{OUT}/sphere_22.obj",  *sphere_verts_faces(22, 20, 10))
print(f"  -> {OUT}/")

# ── scene.json ──
scene = {
    "camera": {
        "position": [300.0, 180.0, -250.0],
        "target":   [300.0, 150.0, 300.0],
        "up":       [0.0, 1.0, 0.0],
        "fov_y_degrees": 55.0,
        "near": 10.0, "far": 2000.0
    },
    "area_light": {
        "origin": [250.0, 395.0, 250.0],
        "edge_u": [40.0, 0.0, 0.0],
        "edge_v": [0.0, 0.0, 40.0],
        "color":  [1.0, 0.94, 0.82]
    },
    "shadow": {
        "target": [300.0, 0.0, 300.0],
        "up": [1.0, 0.0, 0.0],
        "fov_y_degrees": 120.0, "near": 1.0, "far": 2000.0
    },
    "objects": [
        {"mesh":"floor.obj",      "color":[0.58,0.53,0.47]},
        {"mesh":"wall_back.obj",  "color":[0.70,0.65,0.58], "offset":[0,0,600]},
        {"mesh":"wall_left.obj",  "color":[0.62,0.42,0.38]},
        {"mesh":"wall_right.obj", "color":[0.44,0.54,0.42]},
        {"mesh":"ceiling.obj",    "color":[0.72,0.68,0.62]},
        {"mesh":"box_30.obj",     "color":[1.0,0.94,0.78], "offset":[280,390,280], "emissive":True},
        # SSAO showcase
        {"mesh":"sphere_40.obj",  "color":[0.85,0.75,0.60], "offset":[130,40,180]},
        {"mesh":"sphere_22.obj",  "color":[0.75,0.70,0.80], "offset":[40,22,160]},
        {"mesh":"box_80.obj",     "color":[0.65,0.58,0.50], "offset":[330,40,280]},
        {"mesh":"box_30.obj",     "color":[0.72,0.62,0.55], "offset":[80,15,500]},
        {"mesh":"pillar.obj",     "color":[0.68,0.63,0.57], "offset":[150,110,400]},
        {"mesh":"pillar.obj",     "color":[0.68,0.63,0.57], "offset":[450,110,400]},
        {"mesh":"box_30.obj",     "color":[0.55,0.50,0.62], "offset":[510,15,240]},
        {"mesh":"sphere_22.obj",  "color":[0.70,0.78,0.72], "offset":[410,22,420]},
        {"mesh":"arch.obj",       "color":[0.62,0.57,0.52], "offset":[220,185,400]},
    ]
}

with open(f"{OUT}/scene.json", 'w') as f:
    json.dump(scene, f, indent=2)
print(f"  -> {OUT}/scene.json")
print("Done! Run: ./build/SSAO_Renderer --model-dir models/ssao-demo")
