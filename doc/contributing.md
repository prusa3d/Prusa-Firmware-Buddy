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

### Files: Include Guards
Use the `#pragma once` as a file guard.
Do not use the `#ifdef FILE_X`, `#define FILE_X`, `#endif` pattern.

### Files: Author & Copyright

Do not add file headers with author/creation time/copyright, etc.
Those data are already stored in the commit, and we don't want to duplicate them.

This does not apply to 3rd party code in our repository.

### Formatting: pre-commit

The project contains a pre-commit configuration (see [pre-commit.com](https://pre-commit.com) and `.pre-commit-config.yaml`).
Make sure you have installed pre-commit in your clone of the repository.
The pre-commit will run before every commit you make, ensuring the code is formatted correctly.

### Code Style: C/C++ Naming Conventions

- Types & Classes are in `PascalCase`.
- Global constants in `SCREAMING_CASE`
- Variables (local, class, etc), methods and namespaces are in `snake_case`.
- File names are in `snake_case.cpp` (even if the only thing the file contains is a class named in `PascalCase`).
- Types never end with a `'_t'`.
