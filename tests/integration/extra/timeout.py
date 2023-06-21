import asyncio
import functools
import pytest


def timeoutable(func=None, *, timeout=60):
    __tracebackhide__ = True

    if not func:
        return functools.partial(timeoutable, timeout=timeout)

    @functools.wraps(func)
    async def wrapper(*args, **kwargs):
        __tracebackhide__ = True
        try:
            return await asyncio.wait_for(func(*args, **kwargs),
                                          timeout=timeout)
        except asyncio.TimeoutError:
            pytest.fail(f'{func.__name__} timed out')

    return wrapper
