"""Navigate and query a structured S-expression document."""

from sexp import SExpNode, parse

SOURCE = """
(scene
  (camera (pos 0.0 5.0 -10.0) (fov 60))
  (light  (pos 0.0 10.0 0.0)  (intensity 1.0))
  (mesh   (file "monkey.obj") (pos 0.0 0.0 0.0))
  (mesh   (file "floor.obj")  (pos 0.0 -1.0 0.0)))
"""

scene = parse(SOURCE)

# Index by head atom
camera = scene["camera"]
fov_val = camera["fov"][1].value
print(f"Camera FOV: {fov_val}")  # 60

# Collect top-level tag names
sections = [node.head.value for node in scene if not node.is_atom]
print(f"Sections: {sections}")  # ['camera', 'light', 'mesh', 'mesh']

# Collect every mesh file path
meshes = [node["file"][1].value for node in scene if not node.is_atom and node.head.value == "mesh"]
print(f"Meshes: {meshes}")  # ['"monkey.obj"', '"floor.obj"']


def read_vec3(node: SExpNode) -> tuple[str, str, str]:
    """Return the (x, y, z) atoms from a (tag x y z) node."""
    return node[1].value, node[2].value, node[3].value


light_pos = read_vec3(scene["light"]["pos"])
print(f"Light position: {light_pos}")  # ('0.0', '10.0', '0.0')
