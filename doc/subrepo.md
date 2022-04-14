# Synchronizing subrepo with upstream

`parent =` in .gitrepo must point to existing commit in repository otherwise, subrepo can not be updated with similar error:
```
git subrepo pull lib/Prusa-Error-Codes
git-subrepo: Command failed: 'git rev-list --reverse --ancestry-path --topo-order 7cbf8dcc09cb2606a9b6e9bbf45e562baa0b3227..HEAD'.
```

Copying modifications from / to upstream by hand instead of using git-subrepo creates merge conflicts later.

## Do not

1. Rebase commits manipulating subrepo (e.g. do not rebase and merge PR using github interface)
1. Squash commits if it would lead commit pointed by `parent =` to stop existing.
   E.g. it is possible to squash more commits, if commit manipulating subrepo is the first one.
   It is not possible otherwise.
1. Squash commits if this would combine more subrepo actions
   or subrepo action with modifying subrepo files by top level repository.
1. Copy modifications from / to upstream by hand.

## Do

1. Use `git subrepo pull` or `git subrepo push` when you want to synchronize changes with subrepo upstream.
