from collections.abc import Callable
from typing import Any

class BenchmarkFixture:
    extra_info: dict[str, Any]

    def __call__(
        self,
        func: Callable[..., Any],
        *args: Any,
        **kwargs: Any,
    ) -> Any: ...

    def pedantic(
        self,
        target: Callable[..., Any],
        args: tuple[Any, ...] = ...,
        kwargs: dict[str, Any] | None = ...,
        setup: Callable[[], None] | None = ...,
        teardown: Callable[[], None] | None = ...,
        rounds: int = ...,
        warmup_rounds: int = ...,
        iterations: int = ...,
    ) -> Any: ...
