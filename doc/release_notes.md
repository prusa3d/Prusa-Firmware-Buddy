# Prusa-Firmware-Buddy Release Notes Style

## Title
Format: `X.Y.Z Firmware for Prusa <printer list>`
Pre-releases append `-alpha`, `-RC`, `-RC2` etc. to the version.

## Structure

### 1. Optional urgent notice (H1 `# Update`)
Used only when a release has a critical issue (e.g. withdrawn for a specific printer).
Written in plain prose, bold emphasis on key terms, actionable instructions.

### 2. Summary (H1 `# Summary`)
Bulleted list grouped into categories:
- **New features** — bold heading, one bullet per feature
    - Only genuinely new things (a feature, a setting, a preset).
- **Changes and improvements** — bold heading, one bullet per change
    - Describe the new behavior - for example "XXX now YYY" or "Increased/Improved XXX (old → new)".
- **Fixes** (not "bugfixes") — bold heading, one bullet per fix
    - Make it a sentence - for example "fixed XXX" or "XXX no longer YYY".

Further points:
- Each bullet is a short sentence-like fragment (no trailing period).
- Keep each bullet to about one line (~90 characters, not counting tags and references). Details belong to the detail section.
- Put the most important thing (what the user will look for) at the start of the bullet. A short prefix such as "Fixed" or "New" may precede it.
- Do not start with an article ("The ...").
- Extra important things and bits may be bolded out. For example "New **XXX** filament preset".
- Printer tags and GitHub issue references go at the **end** of the bullet/title, not at the beginning.
- Printer-specific items tagged with `[XL]`, `[C1]`, `[C1/+]`, `[MK4 family]` etc. MMU-specific items tagged with `[MMU]`.
- Tag only items that apply to a subset of the printers in the release title.
- Items already released earlier for other printers get a `[x.y.z]` tag with the first stable release that contained them.
- GitHub issue references inline as `(#1234)`.
- Smaller/patch releases may skip the categorized grouping and just list 3-5 bullets directly.

### 3. Context paragraph
One paragraph placed between the summary and the feature sections. It follows a consistent opening pattern:

> This is the **stable release** of firmware **X.Y.Z** for the **(printer list)**, bringing (brief characterization of changes).

Variants of the opener:
- Stable: `"This is the stable release of firmware 6.4.0 for the CORE One+, CORE One, MK4S, ... and XL."`
- RC: `"This is the (second/nth) release candidate for the upcoming firmware 6.4.0 for the Prusa CORE One, MK4S, ..."`
- Introductory (new printer): `"This release introduces support for the new Prusa CORE One L and its unique features, including ..."`

The opener is followed by a clause describing the release's purpose, connected with a comma, dash, or period:
- `", bringing targeted stability improvements and fixes."`
- `", addressing several improvements and bug fixes, most notably ..."`
- `"– along with bringing new features, quality-of-life improvements, and fixes across all platforms."`

When the diff base is not the immediately preceding version (e.g. the release contains changes already shipped for other printers, marked with `[x.y.z]` tags), the paragraph states the base version and explains the tags.

When relevant, the paragraph also explains broader context such as branch reunification or development history (e.g. "It reunifies the firmware development branch after the temporary split during the CORE One launch, where 6.3.x was released exclusively for CORE One and other printers remained on 6.2.x.").

For pre-releases, the paragraph ends with a sentence linking back to earlier alpha/RC notes:
> For a complete overview of features introduced in firmware 6.4.0, users can refer to the 6.4.0-RC release notes published earlier.

### 4. Detail sections (H2)
Each notable change — whether a new feature, improvement, or fix — gets its own H2 section. There is no structural distinction between them. The sections follow the order of the summary. Each contains:
- **Title**: Same title as in the summary, including printer tags and github references.
- **Prose description**: 1-3 paragraphs, written for end users. Explains *what* changed, *why*, and *how to use it*. References menu paths in bold (`**Settings → Hardware → ...**`). Mentions relevant G-codes with syntax examples.
- **Notes/caveats**: Italicized `_NOTE: ..._` blocks at the end of sections.

### 5. Known issues (H2, pre-releases only)
Bulleted list of known regressions. Ends with a note that they'll be addressed in the final release.

## Tone & conventions
- Written for end users, not developers. Never mention internal function names, variable names, code constructs, or implementation details. Describe what the user would observe (the symptom and the resolution), not how the code was changed.
- Explanatory but not verbose — each section is self-contained.
- Bold for printer models, key terms and UI elements (menu paths, menu items, buttons, settings and their values, on-screen messages). Do not use quotes for UI elements.
- Italics for notes and caveats.
- GitHub issues referenced as `(#NNNN)`.
- No emojis. No exclamation marks. Professional, calm register.
- Do not mention iX and iX-only changes, iX goes through different release channels.

## Instructions for the AI
- Notify the user if you think some change warrants a screenshot to be added.
- For review purposes (will be removed before the actual release), add references to all relevant tickets in the change bullets (ONLY in the summary). The relevant tickets should always be present in each commit message.
    - Warn if there is a commit without a ticket and instead report the short commit hash.
- Ticket formats:
    - `PD-XXXXX` (Asana)
    - `BFW-XXXX` (Jira, older commits)
- Put the output to the "CHANGELOG-X.X.X.md" file.
- Always ask the user for:
    - Which printers the release is for.
    - HTML snapshot of the GitHub releases page. Use it for previous versions and wording of past notes.
    - CSV export from the Asana release project.
    - The previous version diff base (should match the last release for the printers the user provided).
- All releases have an appropriate `vX.Y.Z` git tag. When tasked to do release notes for some new version, make it for the diff between the head `RELEASE-X.Y` branch and the appropriate tag.
    - Fetch from `prusa3d/Prusa-Firmware-Buddy-Private` first.
    - The previous tag is usually not an ancestor; diff against `git merge-base` and skip changes already shipped in the previous tag.
- For fixes cherry-picked from a sibling release line, reuse its published wording.
- Do not mention that the translations have been updated - that's implicit for every release.
- Don't mention dependent firmware version bumps, except for the bootloader and MMU.
    - If bootloader gets updated, prompt the user to explain the changes.
    - For the MMU, the users need to update the FW themselves.
- Check the changed code as well, not only the commit messages. They can be misleading.
- Do not mention rewordings
- Order the changes from the most impactful to the least.
- Please keep in mind that some of the commits/bugfixes are bugfixes to things introduced when adding the new features for the same version. Those bugfixes should not be mentioned.
- The RN sketching workflow should be as follows:
    - First generate just a summary/list of the changes with "{TODO: Description}" for points that deserve further description in a separate section.
    - The user then sifts through your proposals, throws out unimportant ones, and possibly adds their own.
    - The user can add {NOTES} for you to apply. When prompted, apply them and remove the {} record.
    - When that is settled, add descriptions and remove the TODOs.
    - Another round with the user.

Final checklist:
- Check that titles match with the summary
- Check descriptions match the titles
- Check that all the formatting rules are correctly applied
