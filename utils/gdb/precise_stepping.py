import gdb


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
        flags = int(self.val)
        step = self.xyze(flags)
        dirs = self.dirs(flags >> 4)
        act = self.xyze(flags >> 8)
        desc = f'S:{step} D:{dirs} A:{act} F:'
        desc += 'B' if flags & (1 << 12) else '-'
        desc += 'E' if flags & (1 << 13) else '-'
        return '|' + desc + '|'

    @classmethod
    def register(cls, val):
        if str(val.type) == 'StepEventFlag_t':
            return cls(val)
        return None


gdb.pretty_printers.append(StepEventFlagPrinter.register)


# step_event_t prettyprinter
class StepEventPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        ticks = int(self.val['time_ticks'])
        flags = str(self.val['flags'])
        return f'SE +{ticks:04} {flags}'

    @classmethod
    def register(cls, val):
        if str(val.type) == 'step_event_t':
            return cls(val)
        return None


gdb.pretty_printers.append(StepEventPrinter.register)
