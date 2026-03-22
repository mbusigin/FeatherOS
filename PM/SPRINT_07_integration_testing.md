# Sprint 7: Integration Testing

**Epic:** FeatherOS User Space Shell  
**Phase:** Phase 4  
**Status:** TODO  
**Priority:** P1 (High)  
**Estimated Duration:** 2 days  

## Goal
Verify `make run` produces a working system with user-space shell.

## Background
All previous sprints should now work together. This sprint verifies everything works end-to-end.

## Tasks

### 7.1 Build test (Day 1)
```bash
make clean && make
make userland
make run
```

### 7.2 Manual verification (Day 1-2)
Test each of these:
- [ ] Shell prompt appears
- [ ] `echo hello` prints "hello"
- [ ] `help` shows commands
- [ ] `pwd` shows "/"
- [ ] `ls` shows "."
- [ ] `exit` terminates

### 7.3 Create test script (Day 2)
```bash
#!/bin/bash
# test_shell.sh
make clean && make && make userland
timeout 5 qemu-system-x86_64 -fda featheros.img \
    -serial file:test_serial.log -nographic || true
grep "echo hello" test_serial.log && echo "PASS" || echo "FAIL"
```

## Deliverables
- Working `make run` command
- Test script for regression
- Documentation of expected output

## Acceptance Criteria
```bash
# Test: make run produces working shell
make run 2>&1 | head -20
# QEMU launches with serial output
# Serial log contains shell output
```

## Dependencies
- Sprint 1-6: All previous sprints complete

## Notes
- Document exact expected output
- Create automated test if possible
- Note any differences from expected
