# Graph tick tests

`bash tests/graph-ticks/run-tests.sh`: host ASan/UBSan checks that
`graphTicks()` in `src/graph-model.h` reproduces classic `DoUpdateGraph`
(`w_graph.c`): January ticks over ten years, decade ticks over 120 years,
compared with an independent transcription for several dates (January,
December, years ending in 0 and 9) at the native 466 px plot width.

Label placement in the native window is not exercised here.
