# Simulation timing tests

Run `bash tests/simulation/run-tests.sh` for 14 control/date/batch assertions
and 13 assertions against the production timer wrapper with a recording Exec
transport. The latter checks failed acquisition cleanup, one pending request,
completion consumption, cancellation and teardown ordering.

Run `bash tests/simulation/engine-tests.sh` for the actual pinned+patched engine
with ASan/UBSan: equal 200-batch inputs must advance Slow < Medium < Fast.
These are functional assertions, not a host/guest speed benchmark.

A batch means up to 16 simTick calls, interrupted by a modal callback. It is
not a month. Date tests include December/January rollover and loaded-city time.
Stubs do not prove Exec scheduling, Intuition focus or native dialog behavior;
those need a run on the target machine.
