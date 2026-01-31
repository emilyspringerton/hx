# HX correctness checklist and suggested tests

This checklist defines correctness for HX. A build should be considered **incomplete** if any item fails.

## Scope & assumptions

- Target environment: localhost development on Linux-like systems.
- Security is intentionally minimal in V1; prioritize correctness and predictability.
- Tests assume a browser with standard clipboard behavior (no custom keybindings).

## 1. Build & startup correctness

**Build**
- Compiles with `gcc -std=c99 -Wall -Wextra -O2`.
- No warnings ignored.
- Clean rebuild works: `make clean && make`.

**Startup**
- Server starts in <100ms on a typical dev box.
- Port is configurable (flag or environment variable).
- Startup failure messages are human-readable.
- Server exits cleanly on `SIGINT`.

## 2. HTTP layer correctness

- `GET /` serves `index.html` with the correct `Content-Type`.
- Browser loads the page without console errors.
- Nonexistent paths return `404`.
- HTTP requests do not block the event loop.

**Failure handling**
- Malformed HTTP requests do not crash the server.

## 3. WebSocket correctness (critical)

**Handshake**
- RFC 6455 handshake implemented correctly.
- `Sec-WebSocket-Accept` is computed (not hardcoded).
- Invalid handshakes are rejected cleanly.

**Framing**
- Client → server frames are masked and correctly unmasked.
- Server → client frames are unmasked.
- Text frames handled correctly.
- Close control frames handled gracefully.

**Stability**
- One misbehaving client cannot crash the server.
- Client disconnect cleans up resources.

## 4. PTY + shell correctness (most important)

**Shell lifecycle**
- One PTY + shell per WebSocket client.
- Shell launched with correct argv/env.
- Shell exits when client disconnects.
- Zombie processes are reaped.

**I/O semantics**
- Browser input reaches PTY stdin exactly once.
- No dropped or duplicated characters.
- Output streams incrementally (not buffered until newline).

**Interactive behavior**
- `ls`, `pwd`, `cd` behave correctly.
- `top`, `vi`, `less` run (even if ugly).
- `Ctrl+C` **in the shell input** sends `SIGINT` to the child.
- `Ctrl+C` **with selected text in the browser** copies text (no `SIGINT`).

## 5. Browser UX correctness

**Input**
- Typing feels immediate (no lag).
- Paste works normally (`Ctrl+V`, right-click).
- Large paste (>1k chars) does not freeze UI.

**Output**
- Output appears in order.
- No missing lines.
- No character corruption.
- Scroll follows output unless the user scrolls up.

**Selection**
- Text selection behaves like normal web text.
- Copy produces expected clipboard contents.
- Selection does not steal focus or input.
- `Ctrl+C` is only intercepted by the app when input is focused and no text is selected.
- Clipboard shortcuts (`Ctrl+C`, `Ctrl+V`, `Ctrl+X`) must follow browser defaults when selection is active.

## 6. Resource & failure correctness

**Memory**
- No unbounded buffers.
- No growth per command.
- Multiple connect/disconnect cycles remain stable.

**File descriptors**
- PTYs closed on disconnect.
- No FD leaks after 100+ reconnects.

**Failure handling**
- Network drop does not hang the shell.
- Shell crash does not crash the server.
- Server continues serving new clients.

## 7. Cognitive correctness

These are first-class requirements.

- No surprise keybindings.
- No invisible modes.
- No “smart” behavior that changes expectations.
- Error messages explain what and why.
- Nothing happens silently.
- If a keybinding deviates from browser defaults, it is documented.

If a human has to ask “why did that happen?”, correctness failed.

---

# Suggested tests

These tests are manual + scripted. They target the failure modes that cause friction.

## A. Smoke tests (5 minutes)

**Boot**
```sh
make && ./hx
```

**Open browser**
- Page loads.
- No console errors.

**Basic command**
```sh
echo hello
```
- Output appears immediately.

**Exit**
```sh
exit
```
- Shell closes and client disconnects cleanly.

## B. Clipboard sanity tests (high value)

Paste this into HX (multiple times):
```sh
git status
git log --oneline -5
```

**Expected:**
- No missing characters.
- No duplicated lines.
- No terminal lockup.

## C. Interactive program tests

Run each inside HX:
```sh
top
less README.md
vi test.txt
```

**Expected:**
- Input works.
- Screen updates (even if ugly).
- `Ctrl+C` exits the program.

## D. Signal tests

Run:
```sh
sleep 100
```

**Expected:**
- With focus in the input and no selection, `Ctrl+C` sends `SIGINT` to the shell.
- With output text selected, `Ctrl+C` copies text without interrupting the shell.

## E. Focus & selection tests

- Click inside the input and type `echo focus`.
- Click in the output area and drag-select multiple lines.
- Press `Ctrl+C` once with the selection active.
- Press `Ctrl+V` with the selection active and confirm paste goes to the browser input only after refocusing it.

**Expected:**
- Input focus allows typing without lost characters.
- Selection does not steal input permanently (clicking input restores typing).
- `Ctrl+C` with a selection copies text and does not send `SIGINT`.
- Clipboard shortcuts with selection active do not trigger shell input.

## F. Stress tests

**Output flood**
```sh
yes | head -n 5000
```

**Expected:**
- No crash.
- Browser remains responsive.
- Output completes.

**Rapid connect/disconnect**
- Refresh the page 20 times quickly.

**Expected:**
- No zombie shells.
- No FD leak.
- Server remains stable.

## G. Failure injection tests

- Kill the shell process manually.
- Disconnect the network (dev tools → offline).
- Send malformed WebSocket frames (if possible).

**Expected:**
- Server survives.
- Client gets a clean disconnect.
- No undefined behavior.

## H. Regression tests (later automation targets)
Automate once the baseline is stable:
- WebSocket handshake test.
- PTY lifecycle test.
- Multi-client concurrency.
- Memory leak checks (valgrind).

---

**Rule of thumb:** if HX ever feels confusing, unpredictable, or clever, it has failed its primary mission.
