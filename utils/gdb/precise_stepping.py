import gdb

PS_TICK_FREQ = 1000000


def _bit_flag(bits, pos, fmt):
    v = int((bits & (1 << pos)) != 0)
    return fmt[v]


# StepEventFlag_t prettyprinter
class StepEventFlagPrinter:
    def __init__(self, val):
        self.val = val

    @staticmethod
    def xyze(bits):
        v = ''
        for i, a in enumerate('XYZE'):
            v += a if bits & (1 << i) else '-'
        return v

    @staticmethod
    def dirs(bits):
        v = ''
        for i, a in enumerate('XYZE'):
            v += a if bits & (1 << i) else a.lower()
        return v

    def to_string(self):
        bits = int(self.val)

        step = self.xyze(bits)
        dirs = self.dirs(bits >> 4)
        act = self.xyze(bits >> 8)

        flags = ''
        flags += _bit_flag(bits, 12, '-B')  # BEGINNING_OF_MOVE_SEGMENT
        flags += _bit_flag(bits, 13, '-E')  # FLAG_END_OF_MOTION
        flags += _bit_flag(bits, 14, '-K')  # KEEP_ALIVE
        flags += _bit_flag(bits, 15, '-F')  # FIRST_STEP_EVENT

        return f'|S:{step} D:{dirs} A:{act} F:{flags}|'

    @classmethod
    def register(cls, val):
        if str(val.type) == 'StepEventFlag_t':
            return cls(val)
        return None


gdb.pretty_printers.append(StepEventFlagPrinter.register)


# step_event_{i32,u16}_t prettyprinter
class StepEventPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        ticks = int(self.val['time_ticks'])
        flags = str(self.val['flags'])

        if str(self.val.type) == 'step_event_i32_t':
            return f'SEi32 {ticks:11} {flags}'
        elif str(self.val.type) == 'step_event_u16_t':
            return f'SEu16 {ticks:5} {flags}'

        raise Exception('unhandled type')

    @classmethod
    def register(cls, val):
        if str(val.type) in {'step_event_i32_t', 'step_event_u16_t'}:
            return cls(val)
        return None


gdb.pretty_printers.append(StepEventPrinter.register)


# Full step-event-queue prettyprinter
class PrintStepEventQueue(gdb.Command):
    def __init__(self):
        super().__init__('ps-step-event-queue', gdb.COMMAND_USER)

    def invoke(self, cmd, from_tty):
        seq = gdb.lookup_symbol('PreciseStepping::step_event_queue')[0].value()
        data = seq['data']
        head = int(seq['head'])
        tail = int(seq['tail'])
        size = data.type.range()[1] + 1
        items = head - tail if head >= tail else (head + size) - tail
        ticks = 0
        print(f'step_event_queue: {items}/{size}')
        for n in range(tail, tail + items):
            i = n % size
            ms = ticks / PS_TICK_FREQ * 1000
            print(f'[{i:4}]', str(data[i]), f'{ms:7.3f}ms')
            ticks += int(data[i]['time_ticks'])


PrintStepEventQueue()


# MoveFlag_t prettyprinter
class MoveFlagPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        bits = int(self.val)

        dirs = StepEventFlagPrinter.dirs(bits >> 4)
        act = StepEventFlagPrinter.xyze(bits >> 8)
        reset = StepEventFlagPrinter.xyze(bits >> 16)

        phase = ''
        phase += _bit_flag(bits, 0, '-A')  # ACCELERATION_PHASE
        phase += _bit_flag(bits, 2, '-C')  # CRUISE_PHASE
        phase += _bit_flag(bits, 1, '-D')  # DECELERATION_PHASE

        flags = ''
        flags += _bit_flag(bits, 12, '-F')  # FIRST_MOVE_SEGMENT_OF_BLOCK
        flags += _bit_flag(bits, 13, '-L')  # LAST_MOVE_SEGMENT_OF_BLOCK
        flags += _bit_flag(bits, 14, '-B')  # BEGINNING_EMPTY_MOVE
        flags += _bit_flag(bits, 15, '-E')  # ENDING_EMPTY_MOVE

        return f'|P:{phase} D:{dirs} A:{act} R:{reset} F:{flags}|'

    @classmethod
    def register(cls, val):
        if str(val.type) == 'MoveFlag_t':
            return cls(val)
        return None


gdb.pretty_printers.append(MoveFlagPrinter.register)


# Full move-segment-queue prettyprinter
class PrintMoveSegmentQueue(gdb.Command):
    def __init__(self):
        super().__init__('ps-move-segment-queue', gdb.COMMAND_USER)

    def invoke(self, cmd, from_tty):
        seq = gdb.lookup_symbol(
            'PreciseStepping::move_segment_queue')[0].value()
        data = seq['data']
        head = int(seq['head'])
        tail = int(seq['tail'])
        unprocessed = int(seq['unprocessed'])
        size = data.type.range()[1] + 1
        items = head - tail if head >= tail else (head + size) - tail
        move_time = 0.
        processed = True
        print(f'move_segment_queue: {items}/{size}')
        for n in range(tail, tail + items):
            i = n % size
            ms = move_time * 1000

            if unprocessed == i:
                processed = False
            state = 'P' if processed else 'U'

            print(f'[{i:4} {state}] {ms:10.3f}ms', str(data[i]))
            move_time += float(data[i]['move_time'])


PrintMoveSegmentQueue()
