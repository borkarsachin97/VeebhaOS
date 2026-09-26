# Strict Zero-FPU Integer Arithmetic Policy

VeebhaOS enforces a strict policy: **Zero floating-point arithmetic throughout the entire codebase.**

This document details the rationale, architectural rules, and fixed-point patterns used across all drivers, window management, animations, and game engines.

---

## 1. Rationale

The primary target silicon (RDA8809 MIPS32r1) and many popular low-power microcontroller cores (such as ARM Cortex-M0/M0+/M3) **do not have a hardware Floating-Point Unit (FPU)**.

When code includes `float` or `double` calculations:
1. **Severe Latency Penalties**: The compiler must generate software floating-point emulation library calls (`__adddf3`, `__muldf3`, `__divsf3`). A single multiplication can consume 100–300 CPU clock cycles instead of 1 cycle.
2. **Binary Bloat**: Software float emulation libraries add 12–25 KB of unnecessary code to the flash image.
3. **Non-Deterministic Execution**: Emulation algorithms have variable cycle execution times based on operands, leading to frame drops in 60 FPS games and audio synthesis jitter.

By strictly prohibiting floating-point types, VeebhaOS guarantees predictable, cycle-accurate performance on 312 MHz MIPS and microcontrollers alike.

---

## 2. Core Rules

1. **No `float` or `double` Types**:
   Never declare variables as `float` or `double`. The build system enforces this across both GCC and Clang builds.
2. **No `<math.h>` Float Functions**:
   Do not use `sin()`, `cos()`, `sqrt()`, `pow()`, `atan2()`, etc.
3. **Use Fixed-Point Arithmetic for Fractions**:
   Use integer scaling factors (e.g. Q8.8, Q16.16, or basis points $1/10000$).
4. **Use Integer Lookup Tables (LUTs) for Trigonometry**:
   Trigonometric functions use precomputed integer tables scaled by 256 or 1024.
5. **Integer Percentage Calculation**:
   Always compute integer ratios using multiplication before division:
   ```c
   /* INCORRECT: float pct = (val / total) * 100.0f; */

   /* CORRECT: pure integer math */
   int32_t pct = (total > 0) ? ((val * 100) / total) : 0;
   ```

---

## 3. Practical Patterns in VeebhaOS

### Fixed-Point Linear Interpolation (Lerp)
To animate an object from position $A$ to position $B$ over $N$ steps:
```c
/* Progress factor t from 0 to 256 (where 256 = 100%) */
int32_t current_x = start_x + ((end_x - start_x) * t) / 256;
```

### Fast Integer Sine / Cosine LUT (8-bit Scale)
For circular motion or oscillating effects:
```c
/* Precomputed sin table: 0..360 degrees mapped to 0..64 index, scaled to -128..127 */
static const int8_t SIN_LUT[64] = {
      0,  12,  25,  37,  49,  60,  71,  81,
     90,  98, 106, 112, 117, 122, 125, 126,
    127, 126, 125, 122, 117, 112, 106,  98,
     90,  81,  71,  60,  49,  37,  25,  12,
      0, -12, -25, -37, -49, -60, -71, -81,
    -90, -98,-106,-112,-117,-122,-125,-126,
   -127,-126,-125,-122,-117,-112,-106, -98,
    -90, -81, -71, -60, -49, -37, -25, -12
};

int32_t get_sine_scaled(uint8_t angle_64, int32_t amplitude) {
    return (SIN_LUT[angle_64 & 63] * amplitude) / 128;
}
```

### Game Physics & Velocity (Space Shooter Example)
In [`apps/vapps/space_shooter/vapp_space_shooter.c`](file:///home/vixxkigoli/pm/VeebhaOS/apps/vapps/space_shooter/vapp_space_shooter.c), alien wave formation, laser velocity, and particle explosion vectors use scaled integer math:
```c
/* Particle explosion with 8 directions */
static const int8_t DIR_X[8] = { 0,  2,  3,  2,  0, -2, -3, -2 };
static const int8_t DIR_Y[8] = { -3, -2,  0,  2,  3,  2,  0, -2 };

st->particles[i].x += DIR_X[st->particles[i].dir];
st->particles[i].y += DIR_Y[st->particles[i].dir];
```
This ensures zero runtime emulation and runs at solid 60 FPS with negligible CPU utilization.
