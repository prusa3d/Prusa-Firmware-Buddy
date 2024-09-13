# GCode Grammar

## Basic GCode
Basic GCode parser does not handle options, only parses the command and decides what is body.

```
GCode: Ws* LineNum? Ws* Command Ws* Body Ws* Checksum? Ws* Comment?

LineNum: r"N-?[0-9]+"
Comment: r";.*"
Checksum: r"\*[0-9]+"

Command: CommandLetter CommandCodenum CommandSubcode?
CommandLetter: r"[A-Z]"
CommandCodenum: r"[0-9]+"
CommandSubcode: r"\.[0-9]+"

Body: r"[^;*]*?" # Non-greedy - exclude trailing whitespace

Ws: r"[ \f\n\r\t\v]"
```

## GCode with options
When parsing gcode options, the `Body` from the basic parser must match `BodyWithOptions`:
```
BodyWithOptions: (OptionKey Ws* OptionValue? Ws*)*¨

OptionKey: r"[A-Z]"
OptionValue: BasicOptionValue | QuotedOptionValue

BasicOptionValue: r"[0-9+\-.,]"
QuotedOptionValue: r"\"([^\"]|\\.)*\""
```
