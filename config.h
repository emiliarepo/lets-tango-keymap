#pragma once

// Custom split RPC channel used by the spinning underglow animation (see keymap.c).
// The master computes hue/phase and syncs it to the slave so both halves rotate together.
#define SPLIT_TRANSACTION_IDS_USER RGB_SYNC