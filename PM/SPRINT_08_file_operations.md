# Sprint 8: File Operations

**Epic:** FeatherOS User Space Shell  
**Phase:** Phase 4  
**Status:** BACKLOG  
**Priority:** P3 (Low)  
**Estimated Duration:** 2 days  

## Goal
Add basic file operations to shell.

## Background
After basic shell works, add ability to read files.

## Tasks

### 8.1 Implement sys_read
- Read from file descriptor
- Connect to VFS

### 8.2 Add cat command support
- Use sys_open/sys_read/sys_close

## Deliverables
- `cat filename` works
- File contents displayed

## Acceptance Criteria
```bash
# Test: cat works
# cat /etc/motd shows file contents
```

## Dependencies
- Sprint 7: Basic shell working

## Notes
- Use VFS for file operations
- Keep it simple initially
