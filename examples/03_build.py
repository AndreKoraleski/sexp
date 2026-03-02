"""Build an S-expression tree programmatically — no parser needed."""

from sexp import SExp, SExpNode, parse


def vec3(tree: SExp, x: float, y: float, z: float) -> SExpNode:
    """Build an unattached (x y z) list node."""
    node = tree.new_list()
    for v in (x, y, z):
        node.append(tree.new_atom(str(v)))
    return node


root = parse("(scene)")
scene = root.head.parent  # SExpNode view of the root list

obj = root.new_list()
obj.append(root.new_atom("mesh"))
obj.append(root.new_atom("monkey.obj"))
obj.append(vec3(root, 1.0, 0.0, -3.0))
scene.append(obj)

print(repr(root))  # (scene (mesh monkey.obj (1.0 0.0 -3.0)))


def make_alist(tree: SExp, pairs: dict[str, str]) -> SExpNode:
    """Build an unattached association list from a dict."""
    alist = tree.new_list()
    for k, v in pairs.items():
        entry = tree.new_list()
        entry.append(tree.new_atom(k))
        entry.append(tree.new_atom(v))
        alist.append(entry)
    return alist


config_root = parse("(config)")
config_root.head.parent.append(
    make_alist(config_root, {"host": "localhost", "port": "8080", "workers": "4"})
)
print(repr(config_root))  # (config ((host localhost) (port 8080) (workers 4)))


# Insert a docstring after the parameter list
defn = parse("(defun greet (name) (print name))")
defn[2].parent.insert_after(defn[2], defn.new_atom('"Say hello."'))
print(repr(defn))  # (defun greet (name) "Say hello." (print name))
