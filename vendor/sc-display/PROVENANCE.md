Display-only code from https://github.com/inwenis/decompile-sc
Commit: 475f0a123dcda77eff89cdf045d6980b1b7203fa
License: MIT (see LICENSE)

Used modules: screen geometry, console relocation, menu centering, Storm presentation,
engine access guards, logging, and inline detours. Unit selection and production changes
are not installed by the BG display entry point.

The BG entry gate waits until MPQDraft has loaded its own plugins before checking
and applying the generated screen signatures.

BG v0.2.0 modifications: console roots centre horizontally; terrain covers the full
screen height; generated tables relocate console hit testing, command-card bounds,
and minimap input/clip bounds. tools/renderer_patch_sites.py derives from the same
MIT upstream generator, with repeated power-of-two terms allowed for 1024x576.
