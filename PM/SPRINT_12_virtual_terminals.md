# Sprint 12: Virtual Terminals

**Epic:** ./project/EPIC.md  
**Phase:** Phase 4: Extended Features  
**Status:** BACKLOG  
**Priority:** P3 (Low)  
**Estimated Duration:** 3 days  

## Goal
Implement virtual terminal switching (Alt+F1-F6).

## Background
Standard OS feature: multiple virtual terminals accessible via keyboard.

## Tasks

### 12.1 Terminal data structures (Day 1)
```c
#define NUM_VT 6

typedef struct {
    char buffer[4096];
    int cursor;
    int scroll;
    task_t *foreground_task;
} virtual_terminal_t;

static virtual_terminal_t terminals[NUM_VT];
static int current_vt = 0;
```

### 12.2 Keyboard handling (Day 2)
```c
void handle_keyboard(void) {
    char c = keyboard_getchar();

    // Check for Alt+Fn
    if (alt_pressed && (c >= KEY_F1 && c <= KEY_F6)) {
        switch_vt(c - KEY_F1);
        return;
    }

    // Send to current terminal
    terminals[current_vt].buffer[terminals[current_vt].cursor++] = c;
}
```

### 12.3 Terminal switching (Day 2)
```c
void switch_vt(int vt_num) {
    // Save current terminal state
    save_vt_state(current_vt);

    // Load new terminal state
    load_vt_state(vt_num);

    current_vt = vt_num;
}
```

### 12.4 Render terminal (Day 3)
```c
void render_terminal(int vt_num) {
    // Clear screen
    vga_clear();

    // Draw buffer content
    for (int i = 0; i < terminals[vt_num].cursor; i++) {
        vga_putchar(terminals[vt_num].buffer[i]);
    }
}
```

## Deliverables
- Alt+F1-F6 switches terminals
- Each terminal has independent shell
- Screen updates on switch

## Acceptance Criteria
```bash
# Alt+F1 shows terminal 1
# Alt+F2 shows terminal 2
# Each terminal has separate state
```

## Dependencies
- Sprint 7: Basic shell working
