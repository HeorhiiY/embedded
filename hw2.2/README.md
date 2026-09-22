# Homework 2.2

Measuring the switching delay of a transistor-driven load.

GPIO 5 (`CONTROL_PIN`) drives the transistor, GPIO 9 (`SENSE_PIN`) reads the
result back with an internal pull-up. An interrupt on the sense pin counts
falling edges, so the main loop can timestamp both the *first* edge (when the
load starts responding) and the *last* one (when it has settled) — the gap
between them is the bounce.

Each sample switches the transistor on, busy-waits for `DELAY_MS` while
watching the edge counter, and reports both timestamps. Ten samples are
averaged. Timing uses `micros()`: the FreeRTOS tick behind `millis()` only
advances in 10 ms steps, which is far too coarse here.

Pins and timings live in [`src/config.h`](src/config.h).

## Output

```
Sample 1
First edge detected at: 3792 us after switching on the transistor
Last edge detected at: 4412 us after switching on the transistor
Sample 2
First edge detected at: 3792 us after switching on the transistor
Last edge detected at: 4389 us after switching on the transistor
Sample 3
First edge detected at: 3793 us after switching on the transistor
Last edge detected at: 4391 us after switching on the transistor
Sample 4
First edge detected at: 3792 us after switching on the transistor
Last edge detected at: 4413 us after switching on the transistor
Sample 5
First edge detected at: 3794 us after switching on the transistor
Last edge detected at: 4415 us after switching on the transistor
Sample 6
First edge detected at: 3793 us after switching on the transistor
Last edge detected at: 4415 us after switching on the transistor
Sample 7
First edge detected at: 3794 us after switching on the transistor
Last edge detected at: 4413 us after switching on the transistor
Sample 8
First edge detected at: 3795 us after switching on the transistor
Last edge detected at: 4392 us after switching on the transistor
Sample 9
First edge detected at: 3794 us after switching on the transistor
Last edge detected at: 4415 us after switching on the transistor
Sample 10
First edge detected at: 3795 us after switching on the transistor
Last edge detected at: 4393 us after switching on the transistor
Mean sense time: 4404.80 us
Mean first edge time: 3793.40 us
```

The load starts responding ~3793 us after the switch and stops moving ~4405 us
after it, so the turn-on delay is ~3.79 ms and the bounce tail lasts ~611 us on
top of that.

The first edge is extremely repeatable (3792–3795 us, ±2 us) — that is the
deterministic turn-on delay of the circuit. The last edge scatters more
(4389–4415 us) and falls into two clusters ~22 us apart, which is the bounce
ending on one contact bounce more or one fewer from sample to sample.
