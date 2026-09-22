# Native audio regressions

`bash tests/audio/run-tests.sh` runs production PCM WAV decoding and the actual
`GameAudio` implementation against a recording Exec/AHI transport, under fatal
ASan/UBSan. It covers signed sample extrema, every truncation, unsupported
formats, odd chunk padding, malformed lengths, resource allocation/device-open
failure cleanup, asynchronous completion, rejected names/missing effects,
four-voice overlap and fifth-voice busy dropping, device errors, abort/wait before freeing samples,
and close/reopen. Host transport tests do not prove the AHI driver works.

`bash scripts/build-audio-smoke.sh` builds `build/<target>/audio-smoke` for the
selected ABI. On a matching AROS guest, run:

```
audio-smoke <staged-sounds-directory> RAM:audio-report.txt
```

The harness plays all eleven effects sequentially, requires successful AHI
request completion, tests a four-effect burst and fifth busy drop, then abort/close/reopen and missing
assets. It exits by itself (each effect has a 1500-tick watchdog). The report
records the selected build target and backend; pair it with the toolchain,
SDK, named guest and audible/captured output in the run report. Device request
completion alone does not prove speakers produced the expected waveform.

## Runtime and source contract

The frontend uses `ahi.device`, default unit 0, API version 4, `CMD_WRITE`,
`AHIST_M16S`, native-endian signed 16-bit mono samples, 22050 Hz. WAV little
endian is decoded explicitly. Full volume, centered pan. Original effects are
preloaded once; callbacks only look up a fixed name and submit asynchronous I/O.

There are four independent voices and no queue. A fifth simultaneous request is
dropped; the engine's current multi-effect disaster bursts fit within the pool.
`poll()` reaps completed I/O without
blocking; it can be called per mainloop and is also called by `play()`. Close
aborts pending playback and waits before releasing the device, port and sample
storage. Missing/invalid individual files skip those effects. Missing AHI or all
files disables sound without preventing gameplay. Resource decode/allocation
failure disables sound gracefully. There is no music or synthesized substitute.

`scripts/stage-audio.py SOURCE DESTINATION` uses host ffmpeg to decode the eleven
exact engine callback names from the pinned MicropolisCore
`content/micropolis/sounds/*.mp3`. It writes WAVs and a source/output SHA-256
manifest to generated staging. These retain the upstream licensing and notices
already copied by `stage-game.sh`: upstream LICENSE (GPLv3 with the EA additional
terms), MicropolisGPLLicenseNotice.md and MicropolisPublicNameLicense.md. The original checkout is
never modified. Decoder/device implementation is project-authored; the API
recipe was checked against the local AROS AHI device `Examples/Device/PlayTest`.
