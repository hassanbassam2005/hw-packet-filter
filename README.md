# Hardware-Accelerated Network Packet Filtering Engine & Priority Scheduler

**Document Ref:** HW-NET-2026-V4[cite: 2]
**Status:** Approved for Prototyping[cite: 2]
**Author:** Senior Network Hardware Architect[cite: 2]
**Original Source:** CISCO SYSTEMS / EMBEDDED SYSTEMS ARCHITECTURE DIVISION[cite: 2]

---

## 🏢 Engineering Assignment & Real-World Context

In enterprise networking switches and 5G core infrastructure, software-based packet inspection (using CPU loops) creates severe latency bottlenecks[cite: 2]. Next-generation routers utilize dedicated hardware logic pipelines (ASICs/FPGAs) to evaluate security flags and route priority in parallel within nanoseconds[cite: 2]. This specification outlines a prototype implementation using an ESP32 host, discrete digital logic gate synthesis, D Flip-Flop hardware registers, and Multiplexer priority routing coupled with real-time I2C telemetry[cite: 2].

---

## 1. Project Architecture & Functional Specification

The System is divided into three functional domains: (1) High-Speed Data Bus & Traffic Generator, (2) Hardware Combinational Logic & Pipeline Register Engine, and (3) Real-Time Telemetry & Visual Analytics Controller[cite: 2].

### 1.1 Subsystem Breakdown

*   **Traffic Generator & Bus Master (ESP32 Core 0):** Emulates high-speed packet header arrival by driving 4 digital output lines representing security and operational header bits (A, B, C, D) alongside a dedicated system clock (CLK)[cite: 2].
*   **Hardware Gating & Register Stage (Discrete Logic):** Evaluates packet validity and security threats instantaneously via boolean gating logic[cite: 2]. Outputs are registered into a 74HC74 D Flip-Flop pipeline stage to prevent glitch propagation to downstream routing networks[cite: 2].
*   **Hardware Priority Scheduler (Multiplexer Stage):** Uses a 74HC153 Multiplexer driven by priority select signals to dynamically map valid packets to High-Priority VIP Queues or Standard Queues while grounding dropped/quarantined packet streams[cite: 2].
*   **Telemetry & OLED Visualizer (ESP32 Core 1):** Reads latched hardware status flags, calculates real-time metrics (Packets Per Second, Pass/Drop %, Active Threat Alerts), and updates a low-latency 0.96" I2C OLED display driver[cite: 2].

---

## 2. Hardware Bill of Materials

Exact Components Required:

| Component Name | IC Part Number | Quantity | Functional Role in Circuit |
| :--- | :--- | :---: | :--- |
| **ESP32 Microcontroller** | ESP32-WROOM-32 | 1 Unit | Traffic Generator (Core 0), System Clock Generator, Telemetry & I2C Display Manager (Core 1)[cite: 2] |
| **OLED Display Module** | 0.96" SSD1306 (I2C) | 1 Unit | Real-time Telemetry Dashboard (Displays Throughput, Drop Rates, Priority Queue Status)[cite: 2] |
| **Quad 2-Input AND Gate** | 74HC08 / 74LS08 | 1 IC | Synthesizes boolean decision function $F_{pass} = A' \cdot B' \cdot D$[cite: 2] |
| **Dual D Flip-Flop IC** | 74HC74 | 1 IC | Acts as 1-stage Pipeline Register to synchronize Pass/Drop flags with system clock[cite: 2] |
| **Dual 4-to-1 Multiplexer** | 74HC153 / 74LS153 | 1 IC | Hardware Priority Queue Selector (Routes data to Queue 3 VIP, Queue 1 Standard, or Drop)[cite: 2] |

---

## 3. Formal Digital Logic Synthesis & Truth Table

The input vector consists of 4 real-time packet control header bits: **A** (Malware Signature Detected), **B** (SYN-Flood Attack Pattern), **C** (VIP Class of Service), **D** (Valid CRC/Parity Checksum)[cite: 2].

| A (Malware) | B (SYN-Flood) | C (VIP) | D (Checksum) | F_drop | F_pass | P1 (Pri 1) | P0 (Pri 0) | Hardware Action State |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :--- |
| 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | DROP (Corrupt Standard Header)[cite: 2] |
| 0 | 0 | 0 | 1 | 0 | 1 | 0 | 1 | PASS (Standard Queue 1)[cite: 2] |
| 0 | 0 | 1 | 0 | 1 | 0 | 0 | 0 | DROP (Corrupt VIP Header)[cite: 2] |
| 0 | 0 | 1 | 1 | 0 | 1 | 1 | 1 | PASS (High-Priority Queue 3 VIP)[cite: 2] |
| 0 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | DROP (Corrupt + SYN-Flood)[cite: 2] |
| 0 | 1 | 0 | 1 | 1 | 0 | 0 | 0 | DROP (SYN-Flood Attack Detected)[cite: 2] |
| 0 | 1 | 1 | 0 | 1 | 0 | 0 | 0 | DROP (Corrupt VIP + SYN-Flood)[cite: 2] |
| 0 | 1 | 1 | 1 | 1 | 0 | 0 | 0 | DROP (Blocked SYN-Flood VIP)[cite: 2] |
| 1 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | DROP (Corrupt + Malware)[cite: 2] |
| 1 | 0 | 0 | 1 | 1 | 0 | 0 | 0 | DROP (Malware Signature Flagged)[cite: 2] |
| 1 | 0 | 1 | 0 | 1 | 0 | 0 | 0 | DROP (Corrupt VIP + Malware)[cite: 2] |
| 1 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | DROP (Blocked Malware VIP)[cite: 2] |
| 1 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | DROP (Multi-Threat Vector)[cite: 2] |
| 1 | 1 | 0 | 1 | 1 | 0 | 0 | 0 | DROP (Multi-Threat Vector)[cite: 2] |
| 1 | 1 | 1 | 0 | 1 | 0 | 0 | 0 | DROP (Multi-Threat Vector)[cite: 2] |
| 1 | 1 | 1 | 1 | 1 | 0 | 0 | 0 | DROP (Multi-Threat Vector)[cite: 2] |

### 3.1 Karnaugh Map (K-Map) Reduction & Boolean Equations

Applying Boolean algebraic reduction and Karnaugh Mapping yields the optimized gate equations executed by the discrete hardware layer[cite: 2]:

1.  **Pass Decision Line ($F_{pass}$):** $F_{pass} = A' \cdot B' \cdot D$[cite: 2]
2.  **Drop Decision Line ($F_{drop}$):** $F_{drop} = (F_{pass})' = A + B + D'$[cite: 2]
3.  **Priority Bit 1 Selection (P1):** $P_1 = F_{pass} \cdot C = A' \cdot B' \cdot C \cdot D$[cite: 2]
4.  **Priority Bit 0 Selection (P0):** $P_0 = F_{pass} = A' \cdot B' \cdot D$[cite: 2]

---

## 4. Pinout Wiring Guide & Interconnect Mapping

Follow this exact physical pin layout to bridge the ESP32 host, 74xx logic gates, D Flip-Flop registers, 4:1 Multiplexer, and OLED visualizer[cite: 2].

| Source Device & Pin | Target Device & Pin | Signal Name | Description |
| :--- | :--- | :--- | :--- |
| **ESP32 GPIO 16** | 74HC08 Pin 1 (1A) | `Signal A` | Malware Header Flag Bus Line[cite: 2] |
| **ESP32 GPIO 17** | 74HC08 Pin 2 (1B) | `Signal B` | SYN-Flood Flag Bus Line[cite: 2] |
| **ESP32 GPIO 18** | 74HC153 Pin 14 (S1) | `Signal C` | VIP Class Select Line to MUX[cite: 2] |
| **ESP32 GPIO 19** | 74HC08 Pin 4 (2A) | `Signal D` | Valid Checksum Flag Bus Line[cite: 2] |
| **ESP32 GPIO 4** | 74HC74 Pin 3 (1CLK) | `SYS_CLK` | System Clock Pulse to D Flip-Flop Register[cite: 2] |
| **74HC08 Output (Pin 6)** | 74HC74 Pin 2 (1D) | `F_pass_raw` | Combinational Pass Result to D Flip-Flop Input[cite: 2] |
| **74HC74 Output Pin 5 (1Q)** | ESP32 GPIO 21 & 74HC153 S0 | `Q_pass (Latched)` | Latched Pass Flag fed back to ESP32 & MUX Select S0[cite: 2] |
| **74HC153 Output Pin 7 (1Y)** | ESP32 GPIO 22 | `MUX_OUT` | Hardware Queue Route Feedback Signal[cite: 2] |
| **ESP32 GPIO 21 (SDA)** | OLED SDA Pin | `I2C Data` | I2C Serial Data line for OLED Display[cite: 2] |
| **ESP32 GPIO 22 (SCL)** | OLED SCL Pin | `I2C Clock` | I2C Serial Clock line for OLED Display[cite: 2] |

---

## 5. Industry Positioning & Resume (CV) Bullet Points

### ■ HOW TO FEATURE THIS PROJECT ON YOUR CV FOR CISCO, NVIDIA, OR APPLE
Avoid basic project titles like 'ESP32 OLED Project'[cite: 2]. Frame this project as a Hardware-Accelerated Network Co-Processor[cite: 2]. Highlight key computer engineering terms: RTL Logic Synthesis, Pipeline Registers, Priority Scheduling, Dual-Core Multiprocessing, and Real-Time Hardware Telemetry[cite: 2].

**Project Title Entry:** Hardware-Accelerated Network Packet Filtering Engine & Pipeline Priority Scheduler[cite: 2]

*   Designed and synthesized modern combinational boolean logic gating (K-Maps) and discrete pipeline register stages (74HC74 D Flip-Flops) to inspect streaming packet headers at line-rate speed[cite: 2].
*   Engineered a 4-to-1 Multiplexer (74HC153) priority scheduler to dynamically route traffic across high-priority VIP queues and standard queues while grounding malicious SYN-flood and corrupt frames[cite: 2].
*   Implemented a FreeRTOS dual-core firmware driver on ESP32 to decouple non-blocking packet traffic generation from a 100Hz I2C OLED network telemetry dashboard displaying throughput metrics (PPS) and drop ratios[cite: 2].
