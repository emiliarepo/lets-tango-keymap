# Agentpad daemon

Companion daemon for the standalone agent macropad (Task 2). It bridges Claude
Code — running as one or more sessions in tmux panes — to the pad's Raw HID
interface: Claude Code hook events stream into the daemon over localhost TCP,
the daemon aggregates them into a single most-urgent status and pushes it to
the pad's underglow, and the pad's four fleet keys (`Jump` / `◄Agent` /
`Agent►` / `New`) come back over the same HID link to drive tmux and Windows
window-focus actions.

Data flow (from `daemon/DESIGN.md`):

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

## Install

Run everything below from the repo root (`daemon/` is a Python package —
`agentpad_daemon.py` and `reporter.py` both do `from daemon import ...`, so
they need the repo root on `sys.path`, not `daemon/` itself).

1. **Dependencies:**
   ```
   pip install -r daemon/requirements.txt
   ```
   Installs `hidapi`, `pywin32` (Windows only), and `psutil`. These are
   isolated to the I/O modules (`hid_link.py`, `actions.py`) — the pure-logic
   modules (`core.py`, `aggregator.py`, `protocol.py`) import none of them and
   test without any hardware.

2. **Reporter:** copy the reporter to a stable, absolute path outside the repo
   so hook commands keep working regardless of where the repo lives:
   ```
   mkdir "%USERPROFILE%\.claude\agentpad"
   copy daemon\reporter.py "%USERPROFILE%\.claude\agentpad\reporter.py"
   ```
   `reporter.py` is dependency-free stdlib — it doesn't need the `daemon`
   package or the venv above, so it works standalone from that location.

3. **Hook registration:** merge `daemon/claude-settings.snippet.json` into
   `~/.claude/settings.json` (create the `hooks` key if you don't already have
   one; if you do, merge the eight event arrays in rather than overwriting).
   The snippet's paths (`~/.claude/agentpad/reporter.py`) are placeholders —
   edit every `command` to the actual path you copied `reporter.py` to in
   step 2, e.g. `C:\Users\<you>\.claude\agentpad\reporter.py`.

4. **Config:**
   ```
   copy daemon\config.example.toml daemon\config.toml
   ```
   Edit `daemon\config.toml` for your setup (see **Config** below). Keep it
   at an absolute path you can reference from the Task Scheduler command in
   the next section.

## Autostart

The daemon is a long-running **user** process — no admin rights and no
Windows Service (a service fights HID access and session-0 isolation; see
`DESIGN.md`). Register it with Task Scheduler to start at logon, using
`pythonw` so it runs with no console window:

```
schtasks /create /tn "AgentpadDaemon" /sc onlogon /rl limited /f /tr "cmd /c \"cd /d C:\git\lets-tango-keymap && pythonw -m daemon.agentpad_daemon config.toml\""
```

Notes on this exact command:

- `agentpad_daemon.main()` takes the config path as a **plain positional
  argument** — `python -m daemon.agentpad_daemon <path-to-config.toml>` — it
  does **not** parse a `--config` flag. Passing `--config config.toml` makes
  `argv[0]` be the literal string `--config`, and `Config.load("--config")`
  then fails with `FileNotFoundError: '--config'` (verified locally). Always
  pass the config path directly, with nothing in front of it.
- It must be launched as a module (`-m daemon.agentpad_daemon`), not as a bare
  script (`python agentpad_daemon.py`) — the latter fails immediately with
  `ModuleNotFoundError: No module named 'daemon'` because `agentpad_daemon.py`
  imports its sibling modules as `daemon.xxx`, which only resolves when the
  repo root (the parent of `daemon/`) is on `sys.path` (verified locally).
  The `cd /d ... &&` in the command above is what puts the repo root there.
- `/rl limited` = run with the logged-on user's normal (non-admin) rights.
- Adjust the repo path (`C:\git\lets-tango-keymap`) and the config filename
  if `config.toml` isn't in the repo root.
- `pythonw.exe` ships alongside `python.exe` in the same install directory;
  it must be on `PATH` for the bare `pythonw` above to resolve.

## HID permissions

The pad's Raw HID interface (`usage_page=0xFF60`, `usage=0x61`) is a
vendor-defined HID collection — Windows lets any user-mode process open it
with **no special entitlement, driver signing, or admin rights**, unlike
exclusive-access device classes. Two practical conditions still apply:

- The pad must be **plugged in**. `HidLink.open()` (`daemon/hid_link.py`)
  enumerates by VID/PID + usage page/usage and simply reports "not open" if
  nothing matches — the main loop (`Daemon.tick()`) retries `open()` every
  tick, so plugging the pad in later is picked up automatically with no
  restart needed.
- The pad must **not be exclusively held** by another process — e.g. a second
  daemon instance, or a conflicting driver. If `open_path` fails, hidapi
  raises `OSError`, which `HidLink.open()` catches and treats the same as
  "not connected yet" (it keeps retrying rather than crashing).

The pad continues to work as a normal keyboard (its other HID interfaces) at
the same time — Raw HID is an additional interface, not a replacement.

## Config

All keys are optional; `Config.load(None)` (or any key omitted from the file)
falls back to the default shown. See `daemon/config.example.toml` for a ready
template.

| Key (`[device]` sub-table where noted) | Default | Meaning |
|---|---|---|
| `port` | `8787` | Localhost TCP port the daemon listens on for reporter events. **Must match `AGENTPAD_PORT`** — see caveat below. |
| `keepalive_s` | `2.0` | Resend the current status to the pad at least this often, even with no change (so the pad's staleness fallback never fires while the daemon is alive). |
| `stale_s` | `5.0` | Mirrors the firmware's local staleness threshold (documented in `DESIGN.md`'s `STALE_MS`) — how long the pad waits without a status update before reverting to the Task-1 static colour. Present in `Config` and loaded from `config.toml`, but currently informational: the Python daemon itself doesn't read `cfg.stale_s` in its own logic (the actual fallback timer lives in firmware); keep it in sync with the firmware constant if you change one. |
| `session_ttl_s` | `3600.0` | How long (seconds) an inactive session is kept in the aggregator before being expired/dropped, independent of an explicit `SessionEnd`. Not present in `config.example.toml` — add it explicitly if you want a non-default TTL. |
| `foreground` | `true` | Whether `Jump` also tries to bring the terminal window to the foreground (`SetForegroundWindow`), not just switch the tmux pane. |
| `terminal_exe` | `"WindowsTerminal.exe"` | Process name (`psutil`) `foreground()` looks for when raising the terminal window. |
| `new_window_argv` | `["new-window", "claude"]` | Argv appended to `tmux` for the `New` fleet key, e.g. `tmux new-window claude`. |
| `device.vid` | `0x1209` | Pad USB vendor ID. |
| `device.pid` | `0xBEE5` | Pad USB product ID. |
| `device.usage_page` | `0xFF60` | Raw HID usage page to match when enumerating. |
| `device.usage` | `0x61` | Raw HID usage to match when enumerating. |

## Firmware side

The pad must be running the Task-2 firmware build (`RAW_ENABLE = yes` in
`macropad/rules.mk`, plus the `status.c`/`.h` module and the fleet-key
keycodes described in `daemon/DESIGN.md`) — the Task-1-only build has no Raw
HID interface for the daemon to open. With that build flashed, the underglow
renders the aggregate status the daemon pushes:

- **idle** — dim
- **running** — amber, breathing
- **waiting** — green
- **error** — red

If the daemon is stale (no status report for `stale_s`, or not running at
all) the pad falls back to the static Task-1 colour (layer / OS-mode hue), so
it still works standalone with the daemon off.

## Caveats

- **Hook event names + payload — confirmed, not just assumed.** The eight
  event names in `claude-settings.snippet.json` (`SessionStart`,
  `UserPromptSubmit`, `PreToolUse`, `PostToolUse`, `Notification`, `Stop`,
  `SubagentStop`, `SessionEnd`) and the stdin contract `reporter.py` depends
  on (`session_id` and `cwd` present on every hook's stdin JSON) were checked
  against the current Claude Code hooks documentation during implementation
  and matched exactly — this is confirmed, not a to-do.
- **`AGENTPAD_PORT` is the single source of truth for the port, and reporter
  and daemon each read it from a different place.** `reporter.py` reads the
  `AGENTPAD_PORT` environment variable (default `8787`) at the time each hook
  fires; the daemon reads `port` from `config.toml` (also default `8787`).
  Nothing keeps these in sync automatically — if you change one, change the
  other (and make sure `AGENTPAD_PORT` is set in whatever environment your
  Claude Code hooks actually run in, not just your interactive shell).
  Additionally, `reporter.py` does `int(env.get("AGENTPAD_PORT", ...))` with
  no validation: a **non-numeric `AGENTPAD_PORT` raises `ValueError`
  internally, which the reporter's blanket error handling swallows** — the
  hook event is silently dropped and the reporter still exits 0, so a typo
  here fails with no visible error anywhere.
- **`Jump`'s foreground step is best-effort.** `actions.foreground()` calls
  `SetForegroundWindow`, which Windows' focus-stealing prevention can simply
  ignore depending on what currently has focus — the tmux pane switch always
  happens, but the window may not actually come to the front.
- **If you have multiple terminal windows open, `foreground()` picks the
  first match** it finds while enumerating top-level windows by process name
  (`terminal_exe`), not necessarily the one with the waiting session — with
  a single Windows Terminal window (the documented setup) this is moot.
- **tmux is required for all fleet-key actions**, not just `Jump` —
  `actions.run()` shells out to `tmux` unconditionally and does not catch a
  missing executable, so if `tmux` isn't on `PATH` the daemon's main loop will
  raise on the next fleet-key press rather than failing silently.
