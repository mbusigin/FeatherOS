# FeatherOS Project Management Index

**Project:** FeatherOS User-Space Shell  
**Last Updated:** 2025-03-22  
**Status:** Active Development  
**Project Directory:** `./project/` (PM folder: `./project/PM/`)

## Overview

This directory contains sprint task files for implementing the user-space shell in FeatherOS.

## Sprint Stack (Priority Order)

| # | Sprint | Status | Priority | Duration |
|---|--------|--------|----------|----------|
| 1 | [SPRINT_01_create_process_c.md](SPRINT_01_create_process_c.md) | TODO | P0 | 3 days |
| 2 | [SPRINT_02_fix_main_c.md](SPRINT_02_fix_main_c.md) | TODO | P0 | 2 days |
| 3 | [SPRINT_03_implement_syscalls.md](SPRINT_03_implement_syscalls.md) | TODO | P0 | 3 days |
| 4 | [SPRINT_04_integrate_userland.md](SPRINT_04_integrate_userland.md) | TODO | P1 | 2 days |
| 5 | [SPRINT_05_shell_integration.md](SPRINT_05_shell_integration.md) | TODO | P1 | 2 days |
| 6 | [SPRINT_06_init_process.md](SPRINT_06_init_process.md) | TODO | P2 | 1 day |
| 7 | [SPRINT_07_integration_testing.md](SPRINT_07_integration_testing.md) | TODO | P1 | 2 days |
| 8 | [SPRINT_08_file_operations.md](SPRINT_08_file_operations.md) | BACKLOG | P3 | 2 days |
| 9 | [SPRINT_09_testing_docs.md](SPRINT_09_testing_docs.md) | BACKLOG | P3 | 1 day |
| 10 | [SPRINT_10_process_forking.md](SPRINT_10_process_forking.md) | BACKLOG | P2 | 3 days |
| 11 | [SPRINT_11_signal_handling.md](SPRINT_11_signal_handling.md) | BACKLOG | P2 | 2 days |
| 12 | [SPRINT_12_virtual_terminals.md](SPRINT_12_virtual_terminals.md) | BACKLOG | P3 | 3 days |

## Phase Overview

| Phase | Sprints | Focus | Status |
|-------|---------|-------|--------|
| Phase 1 | 1-4 | Foundation Fixes | TODO |
| Phase 2 | 5-7 | Working Shell | TODO |
| Phase 3 | 8-9 | Polish | BACKLOG |
| Phase 4 | 10-12 | Extended Features | BACKLOG |

## Quick Start

### Current Sprint
Start with Sprint 1: Create process.c

### Build Commands
```bash
cd project
make clean && make           # Build kernel
make userland                # Build shell
make run                     # Run in QEMU
```

### Testing
```bash
# Check serial output
cat serial.log

# Run specific test
make test
```

## Status Legend

| Status | Description |
|--------|-------------|
| **TODO** | Not started |
| **IN_PROGRESS** | Currently working |
| **DONE** | Completed |
| **BLOCKED** | Waiting on dependencies |
| **BACKLOG** | Lower priority, optional |

## Priority Legend

| Priority | Description |
|----------|-------------|
| **P0** | Critical - must complete |
| **P1** | High - important |
| **P2** | Medium - nice to have |
| **P3** | Low - optional |

## Sprint Dependencies

```
Sprint 1 (process.c)
    ↓
Sprint 2 (main.c)
    ↓
Sprint 3 (syscalls)
    ↓
Sprint 4 (userland build)
    ↓
Sprint 5 (shell integration)
    ↓
Sprint 6 (init process)
    ↓
Sprint 7 (integration testing)
    ↓
    ├─→ Sprint 8 (file ops)
    ├─→ Sprint 9 (docs)
    ├─→ Sprint 10 (forking)
    ├─→ Sprint 11 (signals)
    └─→ Sprint 12 (virtual terminals)
```

## Files

- `INDEX.md` - This file
- `SPRINT_*.md` - Individual sprint task files

## Total Estimated Time

| Phase | Days |
|-------|------|
| Phase 1 | 10 days |
| Phase 2 | 5 days |
| Phase 3 | 3 days |
| Phase 4 | 8 days |
| **Total** | **26 days** |

## Notes

- All P0 and P1 sprints must be completed before basic shell works
- P2 sprints are nice-to-have enhancements
- P3 sprints are optional features
- Total estimated time: 26 days
