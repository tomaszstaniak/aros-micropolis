# Chalk tests

`bash tests/chalk/run-tests.sh`: host ASan/UBSan checks of `src/chalk-overlay.h`.
Stroke building, duplicate points and bounding boxes; the eraser follows
classic `InkInBox`/`EraserTo` (`w_tool.c`): a 17x17 box around the pointer,
single points by bounding box, longer strokes by segment bounding box, whole
strokes removed. Pixel checks cover the 3 px line, the 7x7 click dot, origin
offset, edge clipping and the 3/16 overview scaling.

No native window drawing or mouse input is exercised here.
