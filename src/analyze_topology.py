# Let's list the faces and see which vertices are connected to form the blocks
faces = []
with open("/Users/erenkutsal/courses/comp410/project/possible_object/models/reutersvard_rectangle.obj", "r") as f:
    for line in f:
        if line.startswith("f "):
            parts = line.split()
            face_verts = []
            for p in parts[1:]:
                # format is v/vt/vn
                v_idx = int(p.split("/")[0])
                face_verts.append(v_idx)
            faces.append(face_verts)

print(f"Total faces: {len(faces)}")

# Let's find connected components of vertices using the faces
parent = {}
def find(i):
    if i not in parent:
        parent[i] = i
    if parent[i] == i:
        return i
    parent[i] = find(parent[i])
    return parent[i]

def union(i, j):
    root_i = find(i)
    root_j = find(j)
    if root_i != root_j:
        parent[root_i] = root_j

for face in faces:
    for u in face:
        for v in face:
            union(u, v)

components = {}
for i in range(1, 41): # 40 vertices
    root = find(i)
    if root not in components:
        components[root] = []
    components[root].append(i)

print(f"Number of connected components (independent blocks): {len(components)}")
for k, v in components.items():
    print(f"Component {k}: vertices {v}")
