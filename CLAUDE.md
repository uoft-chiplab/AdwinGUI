# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AdwinGUI is a **physics lab experimental control GUI** (v17.1.5) for a cold atoms laboratory. It controls ADwin real-time data acquisition hardware, laser DDS systems, microwave generators, and function generators to execute precise pulse sequences for atomic physics experiments.

## Build System

This project is built using **National Instruments LabWindows CVI 2019** (Windows-only IDE). There is no command-line build system — compilation is done through the CVI IDE using `GUIDesign.prj`/`GUIDesign.cws`.

- Release binary: `GUIDesign.exe`
- Debug binary: `GUIDesign_dbg.exe`
- No Makefile, CMakeLists.txt, or automated test/lint infrastructure exists.

## Architecture

### Layers

1. **Hardware Interface Layer**
   - `Adwin.c/h` — Wrapper around `ADwin32.dll` for ADwin real-time processor communication
   - `VISA_SRS_SETUP.c` — VISA protocol for ethernet/USB instrument control (replaced legacy GPIB)
   - `RabbitCom9858.h` — Rabbit DDS controller protocol (UDP-based)

2. **Device Control Modules** (each has `*Settings.c` + `*Control.c` pattern)
   - **Lasers**: `LaserSettings.c`, `LaserControl.c`, `LaserTuning.c` — 4 laser DDS channels
   - **DDS**: `DDSSettings.c`, `DDSControl.c`, `ddstranslator.c` — Three DDS types: GP DDS (UDP), Micro, PhaseO (Rabbit-based)
   - **Digital I/O**: `DigitalSettings.c` — 64 digital channels (expandable to 96)
   - **Analog**: `AnalogSettings.c`, `AnalogControl.c` — 48 analog channels
   - **Anritsu Microwave**: `AnritsuCommands.c` — VISA-based microwave generator
   - **Rigol Function Generators**: VISA aliases at pseudo-addresses 100–104

3. **Sequence Execution Engine**
   - `GUIDesign.c` (3979 lines) — Main GUI event loop and sequence execution
   - `multiscan.c` (3205 lines) — Multi-parameter scanning
   - `scan.c` — Single-parameter scan operations
   - `saveload.c` (3839 lines) — Save/load `.seq` sequence files

4. **Global State**
   - `vars.h` (626 lines) — All global variables, configuration constants, and data structures
   - `main.c` — Application entry point, initialization

5. **UI**
   - `.uir` files — Binary LabWindows CVI UI resource files (edit only in CVI IDE)
   - `GUIDesign.uir` — Main panel; sub-panels for each device settings group

### Data Flow

```
User Input (GUI panels) → Parameter validation → Sequence building
→ ADwin bytecode upload (TransferData.TA1/TB1) → Real-time execution
→ Hardware outputs (analog/digital/DDS/laser) → Status display
```

### Key Globals Pattern

Panel handles and device state are stored as globals in `vars.h`. All modules share this state directly — there is no dependency injection or object model.

### DDS Types

- **GP DDS**: General-purpose DDS using UDP connection (added v17.1.5)
- **Micro DDS**: Rabbit-based DDS for microwave control
- **PhaseO DDS**: Rabbit-based DDS for phase offset control
- **Laser DDS**: 4-channel laser frequency control via TCP

### Scanning Modes

The scan engine supports sweeping: Analog values, Time parameters, DDS frequency/amplitude (single cell and floor modes), and Digital states.

## Key Files Reference

| File | Purpose |
|------|---------|
| `vars.h` | All globals and data structures — read this first |
| `GUIDesign.c` | Main event loop; understand control IDs from `GUIDesign.h` |
| `saveload.c` | Sequence file format (`.seq`) — read when modifying save/load |
| `ddstranslator.c` | DDS command translation logic |
| `Adwin.c` | ADwin hardware communication |
| `change_log.txt` | Version history and context for recent changes |
| `feature_requests.txt` | Pending and completed features |

## Coding Conventions

- C code with LabWindows CVI extensions (`userint.h`, `cvirte.h`)
- Control IDs are auto-generated constants in `GUIDesign.h` — never hardcode numeric control IDs
- Windows-specific APIs are used throughout (`windows.h`)
- VISA resource strings use aliases defined in the NI-VISA configuration (not hardcoded addresses), except Rigol generators at pseudo-addresses 100–104
- ADwin processor type is configurable via `processordefault.txt`
