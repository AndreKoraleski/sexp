"""A minimal Lisp-like expression evaluator built on sexp.

Supported forms: integers, symbols, (if cond then else),
(let ((sym val) ...) body), and arithmetic (+ - * /).
"""

from __future__ import annotations

from sexp import SExpNode, parse

Env = dict[str, int]


def is_number(s: str) -> bool:
    """Return True if s is a decimal integer literal."""
    return s.lstrip("-").isdigit()


def evaluate(node: SExpNode, env: Env) -> int:
    """Evaluate an S-expression node and return its integer value."""
    if node.is_atom:
        if is_number(node.value):
            return int(node.value)
        if node.value in env:
            return env[node.value]
        raise NameError(f"Undefined symbol: {node.value!r}")

    if len(node) == 0:
        raise ValueError("Cannot evaluate empty list")

    head = node[0].value

    if head == "if":
        return evaluate(node[2] if evaluate(node[1], env) else node[3], env)

    if head == "let":
        local_env = dict(env)
        for binding in node[1]:
            local_env[binding[0].value] = evaluate(binding[1], env)
        return evaluate(node[2], local_env)

    args = [evaluate(child, env) for child in node.tail]
    match head:
        case "+":
            return sum(args)
        case "-":
            return args[0] - sum(args[1:])
        case "*":
            result = 1
            for a in args:
                result *= a
            return result
        case "/":
            return args[0] // args[1]
        case _:
            raise NameError(f"Unknown operator: {head!r}")


def eval_str(source: str, env: Env | None = None) -> int:
    """Parse and evaluate a source expression."""
    tree = parse(source)
    return evaluate(tree.head.parent, env or {})


print(eval_str("(+ 1 2 3)"))  # 6
print(eval_str("(* 2 (+ 3 4))"))  # 14
print(eval_str("(if 1 42 0)"))  # 42
print(eval_str("(if 0 99 (+ 10 5))"))  # 15
print(eval_str("(let ((x 3) (y 4)) (+ (* x x) (* y y)))"))  # 25
print(eval_str("(- 100 (let ((n 7)) (* n n)))"))  # 51
