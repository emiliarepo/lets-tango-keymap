#pragma once
#include <stdint.h>
#include <stdbool.h>
void status_init(void);        // keyboard_post_init_user (after underglow init)
void status_tick(void);        // housekeeping_task_user
bool status_is_active(void);   // fresh daemon status is driving the underglow?
void status_render(void);      // paint current status colour (called by the arbiter)
