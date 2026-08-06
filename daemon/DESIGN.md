# Task 2 — companion daemon (agent-aware macropad)

Design spec for the host daemon that makes the standalone agent macropad
*agent-aware*: the underglow shows the most-urgent Claude Code state at a glance,
and the fleet keys (`Jump` / `◄Agent` / `Agent►` / `New`) drive real multiplexer
actions instead of dumb keystrokes. Builds on Task 1 (the pad's Task-1 seams:
`set_status_color()`, the fleet keys as custom keycodes, and the `RAW_ENABLE`
note in `macropad/rules.mk`).

No BLE, no extra hardware — a host daemon bridges Claude Code to the pad over USB
Raw HID.

## Target environment (this setup)

- **OS:** Windows 11, native (not WSL).
- **Shell:** Claude Code CLI runs in `zsh` under Git Bash.
- **Multiplexer:** tmux, running inside one Windows Terminal window. Sessions are
  tmux windows/panes; each runs its own `claude`.
- **Daemon:** Python, native Windows (the pad's USB is visible to Windows directly).

The daemon and firmware are written to be portable, but the fleet-key actions and
autostart are Windows/tmux-specific and live behind config.

## Architecture

```
 Claude Code sessions (each in its own tmux pane, in Windows Terminal)
   │  each hook event → reporter.py  (session_id, cwd, $TMUX_PANE, event, ts)
   ▼  localhost TCP 127.0.0.1:PORT  (one JSON line per event)
 agentpad_daemon.py  (native Windows, autostart at logon)
   │  • per-session state; aggregate status: waiting > running > idle > none
   │  • tracks WHICH session is waiting (for Jump)
   │  Raw HID (hidapi)   ▲ fleet-key byte
   ▼ status byte         │
 macropad  ── 6 LEDs = aggregate status ── fleet keys → daemon → tmux / Win32
```

## Raw HID protocol (32-byte reports)

QMK Raw HID uses fixed 32-byte reports on usage page `0xFF60`, usage `0x61`. On
Windows, hidapi prepends a report-id byte (`0x00`); the daemon writes 33 bytes,
the firmware sees 32.

- **Host → pad — status:** `data = [0x01, status, 0…]`
  `status`: `0` none/off · `1` idle (dim) · `2` thinking (amber breathe) ·
  `3` waiting (green) · `4` error (red). Sent on change **and** as a keepalive
  every ~2 s.
- **Pad → host — fleet key:** `data = [0x10, key, 0…]`
  `key`: `0` Jump · `1` ◄Agent · `2` Agent► · `3` New.
- **Staleness:** the firmware tracks time since the last status report. No status
  for `STALE_MS` (~5 s) → it reverts to the Task-1 local colour (layer / OS-mode),
  so the pad still works standalone when the daemon is off.

## Firmware changes (`macropad/`)

- `rules.mk`: `RAW_ENABLE = yes`.
  - **Flash-budget risk:** the Task-1 pad is at 91 % (2514 bytes free). Raw HID
    adds ~1–2 KB. Verify it fits by compiling; if tight, trim an unused feature
    (`MOUSEKEY` / `EXTRAKEY`).
- New module `status.c/.h` (keeps `keymap.c` glue-only, matching Task 1):
  - `raw_hid_receive(data, len)` — validate `data[0]==0x01`, store `data[1]` +
    a timestamp.
  - `status_task()` (called from `housekeeping_task_user`) — if stale, hand the
    colour back to the Task-1 policy (`underglow_for_layer`); else render the
    status, including the amber **breathe** animation for "thinking". Applies
    colour through the existing `set_status_color()` seam.
  - Precedence: while `Fn` is held (`_CTL` active) the pad still shows the control
    hue, so you always know you're in the control layer; on `_BASE` a fresh daemon
    status wins, falling back to the OS-mode hue when stale/absent.
- Fleet keys: `CC_JUMP`, `CC_AGENT_PREV`, `CC_AGENT_NEXT`, `CC_NEW` change from
  tmux `SEND_STRING` placeholders to custom keycodes that `raw_hid_send()` the
  `[0x10, key]` event. Everything else (Esc/Enter, slash macros, OS-layout
  compensation, `_CTL` controls) is unchanged.

## Daemon (`daemon/agentpad_daemon.py`)

Long-running user process. Responsibilities:

- **Device:** open the pad by VID/PID + raw usage page (`0xFF60/0x61`), all
  config-overridable. Reconnect if unplugged.
- **State:** per-session `{status, cwd, tmux_pane, last_seen}` keyed by
  `session_id`; expire sessions on `SessionEnd` or after an idle TTL.
- **Aggregate:** derive one status `waiting > running > idle > none`; remember the
  waiting session's pane for `Jump`.
- **Push:** send status on change + keepalive every ~2 s.
- **Fleet keys:** read pad events and act (all config-driven):
  - **Jump** → `tmux select-window -t <waiting pane>` (+ `switch-client`), then
    best-effort foreground the terminal (`SetForegroundWindow` on
    `WindowsTerminal.exe` via pywin32).
  - **◄Agent / Agent►** → cycle tmux windows (previous / next).
  - **New** → configurable command; default `tmux new-window claude`, with an
    optional git-worktree template.

## Hooks + reporter

- `reporter.py` — tiny, dependency-free. Reads the hook JSON on stdin, adds
  `$TMUX_PANE` from the environment, sends one JSON line
  (`{event, session_id, cwd, tmux_pane, ts}`) to the daemon over localhost TCP.
- `claude-settings.snippet.json` — hook registration to merge into
  `~/.claude/settings.json`. Event → status mapping:

  | Hook event                        | Meaning        |
  |-----------------------------------|----------------|
  | `SessionStart`                    | idle           |
  | `UserPromptSubmit` / `PreToolUse` | running        |
  | `Notification`                    | **waiting**    |
  | `PostToolUse`                     | running        |
  | `Stop` / `SubagentStop`           | idle           |
  | `SessionEnd`                      | none (gone)    |

  > **To confirm before finalizing:** exact hook event names + payload fields
  > (`session_id`, `cwd`) against the installed Claude Code version.

## Repo additions

```
daemon/
  ├── agentpad_daemon.py          # the service
  ├── reporter.py                 # Claude Code hook reporter
  ├── claude-settings.snippet.json# hook registration to merge
  ├── config.example.toml         # VID/PID, port, timeouts, new-session cmd, foreground
  ├── requirements.txt            # hidapi, pywin32, (tomli if <3.11)
  ├── README.md                   # install (autostart + hooks + HID), config, caveats
  └── tests/                      # light unit tests for the aggregation logic
```

## CI

Add a light `daemon-lint` job to `.github/workflows/build.yml`: set up Python,
`pip install ruff`, `ruff check daemon/`. No binary to build; **not** a dependency
of the release job.

## Decisions (defaults, all overridable)

1. **IPC = localhost TCP** (`127.0.0.1:PORT`). Low latency, no file rotation;
   loopback binds don't trigger the Windows firewall prompt. (Alt considered:
   NDJSON state file the daemon tails.)
2. **Autostart = Task Scheduler at logon** — a user process, no admin. (A Windows
   Service fights HID access and session-0 isolation.)
3. **`New` = `tmux new-window claude`** by default, with a git-worktree variant in
   config (the spec's fuller version), since the worktree flow is personal.

## Risks / to-confirm

- **Flash budget** after `RAW_ENABLE` — verify the `.hex` still fits ATmega32U4;
  trim a feature if needed.
- **Hook event names/payload** — confirm against the installed Claude Code version.
- **HID permissions** — Windows opens vendor HID without special entitlements;
  verify the daemon can claim the raw interface while the pad also works as a
  keyboard.
- **Jump when the terminal is backgrounded** — `SetForegroundWindow` is
  best-effort under Windows focus-stealing rules; document the caveat.

## Acceptance (from SPEC 2.7)

- [ ] Starting a Claude Code session lights the pad idle; a permission prompt turns
      it to the waiting colour; completion returns it to idle.
- [ ] With multiple sessions, the pad shows the most-urgent; `Jump` focuses the one
      that's waiting.
- [ ] Pad works standalone (Task-1 static colour) when the daemon isn't running.
- [ ] Daemon README covers install, hook registration, HID permissions, and config.
