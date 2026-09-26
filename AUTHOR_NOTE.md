# VeebhaOS v1.0 — Release Manifesto & Author Note

> "Oneself is truly the refuge of oneself; what other refuge could there be?  
> With oneself thoroughly disciplined, one attains a refuge rarely found."  
> — **The Buddha (Dhammapada, Verse 160)**

---

### The Philosophy: True Hardware Sovereignty

When we purchase hardware and pay for the device, we own it. Because we own it, we must have the absolute right to inspect, control, and modify the code running on it. This fundamental truth drives the entire free and open-source software movement.

Today, open computing thrives on desktops, servers, and modern smartphones through Linux distributions and open mobile operating systems. Yet, the feature phone ecosystem remains completely abandoned—locked behind proprietary blobs, closed vendor firmware, and intentional obsolescence. Millions of capable, low-power devices end up as electronic waste simply because their software stacks are walled gardens.

Rooted in self-reliance and guided by the teachings of the Buddha, I, **Sachin Arunrao Borkar (@vixxkigoli)**, am officially releasing the first public version of **VeebhaOS (v1.0)**.

---

### What VeebhaOS Is

VeebhaOS is a lightweight, embedded RTOS-based operating system engineered specifically for feature phones and deeply constrained MIPS/ARM silicon.

I do not claim it is universally perfect or finished. What I do claim is that **it works reliably on the hardware it has been ported to, and its architecture makes porting to new targets straightforward and modular.**

* **Dual-Kernel Portability:** Architected to run natively on top of FreeRTOS for bare-metal targets, while retaining clean adaptability for the Linux kernel and userland environments.
* **Modular Board Architecture:** Hardware-specific code is strictly isolated. We strongly recommend keeping vendor BSPs and board-specific files out of the core tree by integrating them cleanly as **Git submodules** (e.g., `boards/<board_name>`).
* **Multi-Target Verified:** VeebhaOS v1.0 is verified and running across three environments:
  1. Physical silicon (OBTEL B10 / RDA8809 XCPU).
  2. Emulated hardware via QEMU.
  3. A native desktop simulator for rapid UI and application prototyping.

---

### Roadmap & Future Horizons

VeebhaOS v1.0 establishes the foundation. Our future work will focus on continuous optimization, ecosystem expansion, and visual polish:

1. **Lightweight Graphics Engine:** While the UI framework currently relies on an LVGL base with a customized OS SDK wrapper, we will explore stripped-down, specialized rendering primitives tailored specifically for ultra-low-clock microcontrollers to eliminate all remaining frame drops.
2. **Smooth, Lag-Free Navigation:** Address rendering latency in resource-intensive views (such as large lists and file trees), bringing menu and app transitions down to zero perceived latency.
3. **Standalone App Packaging (`.vapp`):** The built-in application suite (currently categorized within the "VeebhaOS Fun Zone") is built directly into the system image. Future releases will decouple these into fully independent, shareable `.vapp` application binaries.
4. **Arbitrary Resolution Scaling:** The window manager and display pipeline are currently validated on 176×220 panels. We will generalize the coordinate system and asset pipelines to support standard 128×160, 240×320 (QVGA), and larger feature phone displays out of the box.
5. **Aesthetic & Iconography Evolution:** Continue refining the visual identity with high-contrast, professional, and culturally grounded icon sets meeting modern embedded design standards.
6. **Open-Domain Benchmarking:** Measure VeebhaOS directly against existing open and embedded phone environments to push execution speed, memory footprint, and battery efficiency to their absolute limits.

---

With this resolution and commitment to open engineering, I hereby release **VeebhaOS v1.0**.

**Sachin Arunrao Borkar**  
*Alias: @vixxkigoli*
