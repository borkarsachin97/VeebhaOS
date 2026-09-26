# VeebhaOS Coding Standards & Verification

This document outlines the coding standards, architecture constraints, and testing procedures required for contributions to VeebhaOS.

---

## 1. Core Architectural Constraints

1. **Strict Zero Floating-Point Arithmetic**:
   * Never use `float` or `double`.
   * Never include `<math.h>` or invoke float math functions.
   * All geometry, physics, scaling, and trigonometry must be pure integer or fixed-point arithmetic (see [Zero-FPU Policy](../architecture/no-fpu-policy.md)).

2. **C99 Freestanding Portability**:
   * The codebase must compile under freestanding C99 without depending on full POSIX host libraries.
   * Target hardware (OBTEL B10) runs without a hosted C runtime. Avoid POSIX functions (`clock_gettime`, `fork`, `pthread`, `socket`) inside core and app code.

3. **Zero Heap Fragmentation**:
   * Core drivers and kernel services must not allocate dynamic memory during steady-state operation.
   * All application dynamic memory allocations must fit inside their declared `req_heap_bytes` budget.

---

## 2. Naming Conventions

| Prefix | Usage | Examples |
| :--- | :--- | :--- |
| `hal_` | Hardware Abstraction Layer drivers | `hal_display_init()`, `hal_audio_play_tone()` |
| `win_mgr_` | Window Manager functions | `win_mgr_push()`, `win_mgr_pop()` |
| `tpl_` | Declarative UI templates | `tpl_list_create()`, `tpl_dialog_show()` |
| `app_` | Built-in system applications | `app_dialer_open()`, `app_funzone_open()` |
| `vapp_` | Dynamic package loader & VAPPs | `vapp_loader_init()`, `vapp_brick_breaker_launch()` |
| `os_` | Kernel and OSAL primitives | `os_kernel_init()`, `os_nvram_read()` |

---

## 3. Pre-Commit Verification Checklist

Before opening a pull request or submitting code, every change must pass three rigorous gates:

### Gate 1: Regression Test Suite
```bash
make test
```
* **Expectation**: All 60 phases and 1096 frames must complete with exit code `0`.

### Gate 2: AddressSanitizer (ASan) Memory Verification
```bash
make asan
```
* **Expectation**: Zero memory leaks, zero buffer overflows, zero use-after-free, zero undefined behaviors.

### Gate 3: Real Hardware Baremetal Cross-Compilation
```bash
make clean BOARD=obtel_b10
make BOARD=obtel_b10
```
* **Expectation**: Clean compilation of `build/obtel_b10/veebha_os.bin` using the MIPS toolchain with zero compiler errors.
