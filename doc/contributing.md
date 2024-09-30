# Contribution Guidelines

## Git

### Merging: Use rebase whenever possible

The goal is to have a simple and linear git history.
It is not always possible; for example when merging master into private.
In such cases, it is ok to use git-merge.

### Commit: Atomicity

A commit should be a single complete unit of work.
Every commit should be buildable and follow the rules in this document.

### Commit: Message

- Commit message comprises a subject and a body (separated by an empty line).
- Commit message is written in English.
- Subject uses the imperative mood.
- Subject starts with a capital letter and does not end with a period.
- When a commit is relevant to some subset/module of the project (most of the time), use it as a prefix of the subject as follows:
  ```
  metrics: Add support for syslog
  ```
  or
  ```
  gui: Add touch support
  ```
- Write the module in lowercase
- Limit the subject to 72 letters.
- Wrap the body to 72 letters per line.
- Put an issue tracker reference (BFW-xxxx) at the end of the body if you have one. Do not put it in the subject line.

## Formatting & Code Organization

### Formatting

All the source code in this repository is automatically formatted:

- C/C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html),
- Python files using [yapf](https://github.com/google/yapf),
- and CMake files using [cmake-format](https://github.com/cheshirekow/cmake_format).

If you want to contribute, make sure to install [pre-commit](https://pre-commit.com) and then run `pre-commit install` within the repository. This makes sure that all your future commits will be formatted appropriately. Our build server automatically rejects improperly formatted pull requests.

### Files: Include Guards
Use the `#pragma once` as a file guard.
Do not use the `#ifdef FILE_X`, `#define FILE_X`, `#endif` pattern.

### Files: Author & Copyright

Do not add file headers with author/creation time/copyright, etc.
Those data are already stored in the commit, and we don't want to duplicate them.

This does not apply to 3rd party code in our repository.

### Code Style: C/C++ Naming Conventions

- Types & Classes are in `PascalCase`.
- Global constants in `SCREAMING_CASE`
- Variables (local, class, etc), class-level constants, `enum class` items, methods and namespaces are in `snake_case`.
- File names are in `snake_case.cpp` (even if the only thing the file contains is a class named in `PascalCase`).
- Types never end with a `'_t'`.

### Code Style: Coding Conventions
- (recommendation) Prefer using anonymous namespaces instead of `static` cpp members.

## Documentation

### G-Code command documentation
 - Use following format and syntax
```
/**
*### [G|M]<number>: <Description><web link to RepRap wiki> (no space between * and ### . Header size)
*
*#### Usage  (no space between * and #### . Sub-header size)
*
* G<number> [ <parameter 1> | <parameter 2> | ... | <parameter n> ] (four spaces between *  and [G|M] to format a code)
*
*#### Parameters (no space between * and #### . Sub-header size)
*
* - '<parameter 1' - <description parameter 1> (one space between * and -)
*
*    -  '<param. value>' -  <description param.value>
*
*#### Examples
*
*    [G|M]<number> <parameter 1><param. value> ; <Description of this example gcode> (four spaces between *  and [G|M] to format a code)
*
*/
```

 - Example G-Code documentation

```
/**
 *### G27: Park the nozzle <a href="https://reprap.org/wiki/G-code#G27:_Park_toolhead">G27: Park toolhead</a>
 *
 *#### Usage
 *
 * G27 [ X | Y | Z | P ]
 *
 *#### Parameters
 *
 * - `X` - X park position
 * - `Y` - Y park position
 * - `Z` - Z park position
 * - `P` - [value] Z action
 *   - `0` - (Default) Relative raise by NOZZLE_PARK_Z_RAISE_MIN before XY parking
 *   - `1` - Absolute move to NOZZLE_PARK_POINT.z before XY parking. This may move the nozzle down, so use with caution!
 *   - `2` - Relative raise by NOZZLE_PARK_POINT.z before XY parking.
 * - 'W' - [value] Pre-defined positions. Usable only if X, Y and Z are not present as they override pre-defined behaviour. Positions defined in configuration via NOZZLE_PARK_POINT, WASTEBIN_POINT and AXIS_LOAD_POS. WASTEBIN is replaced by PARK in case it is not defined (currently defined only for iX).
 *   - `0` - Predefined park position
 *   - `1` - Predefined purge position
 *   - `2` - Predefined load position
 */
```
