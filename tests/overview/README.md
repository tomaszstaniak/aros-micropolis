# Overview map model

Run `bash tests/overview/run-tests.sh` on the host (Clang, ASan/UBSan).

This covers only `src/overview-model.h`: the classic wmap.tcl layer list,
the engine's own block-size split between full-resolution accessors
(`getTile`, `getPowerGrid`) and reduced-resolution ones that index in block
space (`getPopulationDensity`, `getRateOfGrowth`, ...), tile-to-zone
classification boundaries, layer colour selection and the camera pan/rect
math used by the (not yet built) Intuition overview window.

`src/overview-window.cpp` (F7) is the Intuition window this model drives.
