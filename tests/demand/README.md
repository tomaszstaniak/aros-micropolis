# R/C/I demand indicator model

Run `bash tests/demand/run-tests.sh` on the host (Clang, ASan/UBSan).

Covers only `src/demand-model.h`: storing the latest engine valve values
(`FrontendCallback::updateDemand`, currently a no-op in `src/game.cpp`),
the has-data/reset lifecycle for load/new game, and the signed
zero-baseline bar height used by whead.tcl's demand canvas
(`whead.tcl:428-468`, `UISetDemand` in `micropolis.tcl:2703`).

`FrontendCallback::updateDemand` feeds a live `DemandModel` (see
`src/game.cpp`); the overview window (F7) draws it as the R/C/I bars.
