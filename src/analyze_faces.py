# Read vertices
vertices = {}
with open("/Users/erenkutsal/courses/comp410/project/possible_object/models/reutersvard_rectangle.obj", "r") as f:
    v_idx = 1
    for line in f:
        if line.startswith("v "):
            parts = line.split()
            vertices[v_idx] = [float(parts[1]), float(parts[2]), float(parts[3])]
            v_idx += 1

# Read faces
faces = []
with open("/Users/erenkutsal/courses/comp410/project/possible_object/models/reutersvard_rectangle.obj", "r") as f:
    for line in f:
        if line.startswith("f "):
            parts = line.split()
            face_verts = []
            for p in parts[1:]:
                v = int(p.split("/")[0])
                face_verts.append(v)
            faces.append(face_verts)

# Print each face with vertex positions and their bounding box
for i, face in enumerate(faces):
    coords = [vertices[v] for v in face]
    xs = [c[0] for c in coords]
    ys = [c[1] for c in coords]
    zs = [c[2] for c in coords]
    print(f"Face {i+1:2d}: vertices={face}, x_range=[{min(xs):.1f}, {max(xs):.1f}], y_range=[{min(ys):.1f}, {max(ys):.1f}], z_range=[{min(zs):.1f}, {max(zs):.1f}]")
