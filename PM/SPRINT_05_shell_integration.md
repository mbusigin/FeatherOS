# Sprint 5: Shell Integration

**Epic:** FeatherOS User Space Shell  
**Phase:** Phase 4  
**Status:** TODO  
**Priority:** P1 (High)  
**Estimated Duration:** 2 days  

## Goal
Ensure shell input/output works correctly via syscalls.

## Background
The shell needs to read keyboard input and write to console. sys_read and sys_write exist but may not be connected properly.

## Tasks

### 5.1 Debug keyboard buffer (Day 1)
- Add debug prints to `keyboard_buffer_put()`
- Verify ISR fires on keypress
- Check buffer doesn't overflow
- Test with serial debug output

### 5.2 Debug console output (Day 2)
- Test `sys_write` outputs to VGA
- Test `sys_write` outputs to serial
- Verify `console_write_char()` works
- Check for encoding issues

### 5.3 Test interactive loop (Day 2)
```bash
# Manual test in QEMU
qemu-system-x86_64 -fda featheros.img -serial mon:stdio
# Type 'echo hello' - should see output
```

## Deliverables
- Keyboard input reaches shell
- Shell output appears on screen
- No infinite loops or crashes

## Acceptance Criteria
```bash
# Test: Interactive test (requires manual verification)
# echo command produces output
# help command shows help text
# Cursor moves as you type
```

## Dependencies
- Sprint 3: syscalls implemented
- keyboard.c: keyboard ISR
- syscall_impl.c: sys_read/sys_write

## Notes
- Serial output is helpful for debugging
- Add prints at each stage
- Test one command at a time
