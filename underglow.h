// Per-layer spinning underglow.
//
// Ownership: this module owns every RGB call in the firmware. keymap.c only tells
// it (a) to start, (b) to tick, and (c) when a layer key is physically held.
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Call from keyboard_post_init_user() on BOTH halves.
void underglow_init(void);

// Call from housekeeping_task_user() on BOTH halves. No-ops on the slave, which
// is driven by the split RPC instead.
void underglow_task(void);

// Report the physical up/down of a layer key, so the colour changes on press
// instead of waiting for the tap dance to resolve. Call from process_record_user().
void underglow_hold_hint(uint8_t layer, bool held);
