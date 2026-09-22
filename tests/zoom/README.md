# Zoom tests

`bash tests/zoom/run-tests.sh`: host ASan/UBSan checks of `src/view-geometry.h`
at 8, 16 and 32 px per tile: snapping, minimum and whole-world limits,
coordinates, camera clamping, zoom steps, and `scaleWorldFrame` (2x2
duplication, per-channel 2x2 averaging with rounding, plain copy) with buffers
sized exactly as the frontend sizes them. Default 16 px behaviour matches
`tests/display`.

Native presentation and wheel input are not exercised here.
