# FanControlPro Startup Stability Design

## Goal

Make the local Win32 build start predictably on the target Thunderobot / Clevo system before expanding fan-control features. A startup failure must never take over fan control.

## Evidence

- The signed `ClevoEcInfo.dll` is x86 and exposes every required function.
- The installed `zntport` driver is running.
- A read-only x86 probe returns `InitIo=1`, fan count 2, and valid temperature / duty values.
- The application debug log reaches single-instance mutex acquisition but does not yet identify the following startup boundary.

## Startup Model

1. Create and show the dialog without waiting for optional services.
2. Initialize controls and present a visible startup status.
3. Read configuration and initialize the EC worker after the UI is usable.
4. Run optional startup-task and GPU capability checks outside the critical startup path.
5. Enable fan takeover only after a successful EC initialization and an explicit user action.

## Failure Handling

- Every startup boundary writes an ASCII-safe log entry with relevant Win32 error data.
- Failures appear in the dialog status area; message boxes are not the sole diagnostic channel.
- An EC or worker failure leaves BIOS fan control active, disables takeover controls, and permits a normal application exit.
- GPU frequency locking remains disabled by default and cannot block startup.

## Scope

This iteration changes startup sequencing, observability, and failure containment. It does not change the EC register mapping, install drivers, publish upstream changes, or add new fan-curve features.

## Verification

- Automated tests cover startup-state transitions and prevent a failed EC initialization from enabling takeover.
- The Win32 Release build succeeds.
- A fresh launch produces ordered log entries for each startup phase.
- On the target system, the window appears before optional checks complete and shows a readable status.
- A successful EC probe remains read-only until the user explicitly enables takeover.
