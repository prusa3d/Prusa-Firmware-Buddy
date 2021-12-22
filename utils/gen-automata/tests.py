"""
Testing automata.

This script generates few automata for checking the generator works as
expected. They are generated and used during the unit tests.
"""
from common import *

# Automaton no. 1: accept everything up to a comma, eg `.*,`.
until_comma = Automaton()
start = until_comma.start()
comma = until_comma.add_state("Comma")
comma.mark_enter()
start.add_transition(',', LabelType.Char, comma)
start.add_transition("All", LabelType.Special, start)

compiled = until_comma.compile("automata::test::until_comma")

with open("until_comma.h", "w") as header:
    header.write(compiled.cpp_header())

with open("until_comma.cpp", "w") as file:
    file.write(compiled.cpp_file())
