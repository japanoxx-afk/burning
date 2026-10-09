Display-only code from https://github.com/inwenis/decompile-sc
Commit: 475f0a123dcda77eff89cdf045d6980b1b7203fa
License: MIT (see LICENSE)

Used modules: screen geometry, console relocation, menu centering, Storm presentation,
engine access guards, logging, and inline detours. Unit selection and production changes
are not installed by the BG display entry point.

The BG entry gate waits until MPQDraft has loaded its own plugins before checking
and applying the generated screen signatures.
