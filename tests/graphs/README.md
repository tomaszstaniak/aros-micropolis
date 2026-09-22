# Classic history graphs

Run `bash tests/graphs/run-tests.sh` on the host (Clang, Python 3).

The ASan/UBSan model tests cover all 120 samples in either history bank,
oldest/newest ordering, shared R/C/I scaling, index clamping, range/mask
changes and plot bounds. The 271 assertions include 256 bounds cases;
this is not a count of distinct gameplay features.

`integration.py` compiles the actual `src/graph-window.cpp` with a simulated
Intuition transport and recording RTG blits. Its 22 assertions exercise
icons, range/series controls, date/history refresh, forwarded keys/ticks,
focus, modal draining, close/reopen and resource ownership. The native shim
is shared with the messages test by reading its `stub` literal; that test
is not imported/executed. Engine history is a fixture here, not a guest.

The native window is checked by hand on the target machine; this suite
covers the model and the window code against a recording transport.
