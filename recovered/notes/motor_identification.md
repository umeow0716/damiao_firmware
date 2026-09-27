# Motor-identification recovery note

This note records the current evidence for original
`run_motor_parameter_identification@0x000257c4` (3,084 bytes). It is an
energized commissioning routine and is not part of the ordinary arming path.

## Recovered phase structure

1. IRQ2 is disabled and the bridge is entered through the commissioning
   path. The rotor is locked with a `0.1` normalized vector for `6,284`
   iterations with `100 us` delay.
2. A `60,000`-sample electrical-identification loop injects a sinusoidal
   current target. Its amplitude increases every 100 samples to 5 A, the
   target is refreshed every 10 samples, and the phase expression uses
   `100 * count * 0.00031415926059708`.
3. After sample `20,000`, `identification_rls2_step@0x1fffa160` fits
   `i[k] = a*i[k-1] + b*v[k-1]`. The caller derives
   `R = (1-a)/b` and `L = Ts/b`.
4. A `40,000`-sample stage calls
   `identification_flux_observer_step@0x1fffa234`. It operates on rotating
   d/q current and voltage, electrical speed, and a 23-float observer
   state. Its literal adaptation gains are `15.2` and `0.1`. The q-axis
   normalized voltage rises by `0.001` every 20 samples and is capped at
   `0.2`; the d-axis command integrates `-10 * Ts * i_d` and is clamped to
   `[-0.3, 0.3]`. Electrical speed uses the captured 20-sample low-pass.
5. The mechanical stage runs `80,000` driven samples followed by `20,000`
   coast samples. Two instances of
   `identification_filter_step@0x1fff9e44` produce saturated control outputs,
   while double-precision normal-equation accumulators are later used to
   derive viscous damping and rotor inertia.
6. The result frame begins with byte `e` and contains R, L, flux, damping and
   inertia as five little-endian floats.

## Exact RLS state

The 212-byte RAM helper consumes nine floats in this order:

| Index | Source name | Meaning |
|---:|---|---|
| 0 | `measurement` | current sample `i[k]` |
| 1 | `previous_measurement` | current regressor `i[k-1]` |
| 2 | `applied_voltage` | voltage regressor `v[k-1]` |
| 3 | `coefficient_a` | estimated discrete current coefficient |
| 4 | `coefficient_b` | estimated voltage coefficient |
| 5..8 | `covariance_00..11` | 2×2 RLS covariance |

Recovered constants in the RAM image are:

- lambda: float bits `0x3f7d70a4` = `0.9900000095367432`;
- positive inverse lambda: `0x3f814afd` = `1.0101009607315063`;
- negative inverse lambda: `0xbf814afd` = `-1.0101009607315063`.

`app/src/commissioning_math.c` preserves the helper's operation order and
separate positive/negative inverse-lambda literals. Host tests check the
first update and convergence against a synthetic discrete R/L plant.

## 23-float observer and 10-float filter

The two additional RAM functions were missing from the previous Ghidra
function list because only absolute thunks referenced them. The reproducible
analysis script now explicitly creates and names all three boundaries before
export:

- `0x1fff9e44`, 98 bytes: saturated integral/filter update;
- `0x1fffa160`, 212 bytes: two-parameter RLS;
- `0x1fffa234`, 202 bytes: coupled rotating-frame d/q electrical observer.

Maintainable typed translations and deterministic host vectors live in
`app/include/commissioning.h`, `app/src/commissioning_math.c`, and
`tests/test_commissioning_math.c`.

## Current source status

The source commissioning routine now uses the original 60,000-sample injected
stimulus and exact RLS state to obtain R/L. It then runs the recovered
40,000-sample coupled flux observer, followed by 80,000 driven samples at 2 Hz
and 20,000 coast samples. The mechanical stage uses the two saturated current
filters, the captured 20-sample low-pass coefficients, double-precision
`sum(i^2)`, `sum(i*w)` and `sum(w^2)`, and the original correlation/phase
decomposition to derive damping and inertia.

The electrical regressor uses physical phase voltage
`normalized_command * bus_voltage / sqrt(3)`; omitting the `1/sqrt(3)` factor
would scale both recovered R and L incorrectly. Each commissioning iteration
now follows the original order `wait/read ADC -> estimator and PWM -> clear the
three ADC flags`; INT002 remains disabled until the routine ends. The captured mechanical
low-pass coefficients are float bits `0x3f4c952f` (`0.7991513609886169`) and
`0x3e4dab44` (`0.20084863901138306`). A failed later phase does not partially
commit newly identified values to the live motor configuration.

The original final `acos`/`tan` decomposition is represented algebraically:
viscous damping uses the absolute current/speed correlation, while a negative
correlation makes rotor inertia negative. This sign split is intentionally
covered by a host regression test.

The semantic-coverage row is now `host_verified`: direct QEMU differential
covers the three relocated math helpers, a caller-flow host test covers the
direction/output setup sequencing, and the final-ELF gate pins all motor-identification
phase counts and boundaries. This is still not a target-equivalence claim: the
maintainable D/Q caller translation needs a current-limited, tick-level energized
comparison with the original routine. Promotion to target-verified requires a
capture containing, at minimum,
the injected target, measured d/q current, applied d/q voltage,
RLS coefficients/covariance, observer states, rotor speed, accumulated normal
equations and final five-float result for both original and source firmware.
