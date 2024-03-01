import gdb
import re


# XYZEval prettyprinter to avoid verbose aliases in type union
class XYZEvalPrinter:
    def __init__(self, val, elements):
        self.val = val
        self.elements = elements

    def children(self):
        for x in self.elements:
            yield (x, self.val[x])

    @classmethod
    def register(cls, val):
        if re.match(r'xyze_.*_t', str(val.type)):
            return cls(val, 'xyze')
        elif re.match(r'xyz_.*_t', str(val.type)):
            return cls(val, 'xyz')
        elif re.match(r'xy_.*_t', str(val.type)):
            return cls(val, 'xy')
        return None


gdb.pretty_printers.append(XYZEvalPrinter.register)
