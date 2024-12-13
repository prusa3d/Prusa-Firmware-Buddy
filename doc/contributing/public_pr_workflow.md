# Public PR workflow

Most of the development occurs privately, utilizing an internal ticketing
system and a private fork of this repository.
However, we encourage and value meaningful contributions from the community.

This workflow defines the following roles:
 * **Contributor**: An individual submitting a pull request (PR) to this repository.
 * **Owner**: A member of the firmware team at Prusa Research a.s.,
   distinct from the Contributor.

The workflow is as follows:
1. The Contributor creates a PR based on the current `master` branch.
2. The Owner reviews the PR.
3. If approved, the Owner creates a new ticket in the internal ticketing
   system with the following attributes:
    * **Title**: `[PR {{pr-number-from-github}}] {{title}}`
    * **Description**: A link to the PR
    * **Epic**: A reference to `BFW-5107`
4. Once the ticket is created, the Owner updates the PR title to include
   a cross-reference to the ticket and merges the changes to the public
   `master` branch.
5. The firmware team will cherry-pick PR commits into relevant internal
   branches according to internal release schedules.
