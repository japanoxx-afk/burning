Display and selection-circle code from https://github.com/inwenis/decompile-sc
Commit: 475f0a123dcda77eff89cdf045d6980b1b7203fa
License: MIT (see LICENSE)

Used modules: screen geometry, console relocation, menu centering, Storm presentation,
engine access guards, logging, inline detours, and overflow selection circles.
The upstream selection and production modules are not installed; native/bg_gameplay.cpp
preserves BG's existing 24-unit selection and its own game-function detours.

The BG entry gate waits until MPQDraft has loaded its own plugins before checking
and applying the generated screen signatures.

BG v0.2.0 modifications: console roots centre horizontally; terrain covers the full
screen height; generated tables relocate console hit testing, command-card bounds,
and minimap input/clip bounds. tools/renderer_patch_sites.py derives from the same
MIT upstream generator, with repeated power-of-two terms allowed for 1024x576.
