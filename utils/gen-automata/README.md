# Automata generators

The `lib/WUI/automata` algorithms (parsers) need raw tables of states and
transitions. These are hard to get right (and modify afterwards) by hand.

The scripts here are helpers to make it somewhat easier by:
* Getting the indexing and other low-level details right when given structure in
  memory.
* Helpers to generate bits and connect them together (eg. automata combinators).

## But why?

There are many libraries out there that help one generate parsers. But all
popular ones seem to want to get the _whole input_ at once. There are some
„async ready“, but these usually just can complain that the input is incomplete
and to please accumulate more data.

We want a _streaming_ parser. That is, we feed it part of the input and _forget_
that input. We don't want to have buffers to accumulate the data. We want to
process them right away and minimize what we remember (we can replace a method
_string_ by method _enum_, we can store a content length as a number, we can
ignore unknown headers instead of storing them to ignore them later).

A raw automaton seems to get the job done, but all the libraries want to be
convenient and wrap it up into _something_ and won't give us just the state ID
to return to it later :-(.

## How to use

Add a new script here that will build an in-memory automaton (either by adding
transitions, or by connecting smaller bits together). Then output the results:

```python
compiled = automaton.compile()
header_content = compiled.cpp_header()
implementation_content = compiled.cpp_file()
# Write them to files
```

Then tell cmake to call your script at an appropriate moment so you get the
files.
