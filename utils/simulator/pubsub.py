import weakref
import asyncio


class Subscriber:
    def __init__(self):
        self.queue = asyncio.Queue()

    async def __anext__(self):
        return await self.queue.get()


class Publisher:
    def __init__(self):
        self.subscribers = set()

    async def publish(self, value):
        dead_refs = set()
        for ref in self.subscribers:
            subscriber = ref()
            if not subscriber:
                dead_refs.add(ref)
            else:
                subscriber.queue.put_nowait(value)
        self.subscribers -= dead_refs

    def __aiter__(self):
        subscriber = Subscriber()
        self.subscribers.add(weakref.ref(subscriber))
        return subscriber
