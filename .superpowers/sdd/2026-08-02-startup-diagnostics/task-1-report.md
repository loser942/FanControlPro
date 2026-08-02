# Task 1 Report: Read-Only Startup Self-Check Policy

## Status

Completed and committed. The policy is header-only C++17, has no dependency on
`Core`, EC APIs, or fan-write paths, and returns monitoring-only results until
three consecutive valid CPU/GPU temperature samples have been recorded.

## Commit

`e347caf Add startup self-check policy`

## TDD Evidence

### Red

Test written first: `diagnostics/startup_self_check_policy_test.cpp`.

Command:

```powershell
& $env:COMSPEC /d /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul && cl /nologo /std:c++17 /utf-8 /EHsc diagnostics\startup_self_check_policy_test.cpp /Fe:diagnostics\startup_self_check_policy_test.exe'
```

Result: failed as expected with `fatal error C1083`, because
`../StartupSelfCheckPolicy.h` did not exist.

### Green

Command:

```powershell
& $env:COMSPEC /d /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul && cl /nologo /std:c++17 /utf-8 /EHsc diagnostics\startup_self_check_policy_test.cpp /Fe:diagnostics\startup_self_check_policy_test.exe && diagnostics\startup_self_check_policy_test.exe'
```

Result: exit code 0. The independently compiled C++17 diagnostic covers:

- Three valid CPU/GPU temperature samples unlock takeover only on sample three.
- An invalid temperature locks the policy in monitoring-only mode.
- Missing RPM/GPU availability produces warnings without blocking faults.
- Core initialization failure produces a blocking monitoring-only result.

## Changes

- Added `StartupSelfCheckPolicy.h` with immutable `StartupCheckResult` fields:
  blocking faults, warnings, consecutive-valid-sample count, takeover decision,
  and Chinese status message.
- Added `StartupState::SelfChecking`, which remains non-takeover-capable under
  the existing state predicates.
- Registered the policy header in `FanControlPro.vcxproj`; existing project
  configurations remain only `Debug|Win32` and `Release|Win32`.
- Added the standalone C++17 diagnostic test.

## Concerns

- `Debug|Win32` and `Release|Win32` application builds could not run in this
  environment. Both stop before compilation with MSB8041 because the installed
  Visual Studio Build Tools lack the MFC libraries required by the project.
- The test compiler creates `diagnostics/startup_self_check_policy_test.exe`.
  It is untracked and was deliberately excluded from the commit, along with all
  existing untracked binaries and `.superpowers` content.

## Fix Round 1 Requested

Reviewer finding: GPU unavailable must remain a warning even if GPU temperature is unreadable; add a three-sample regression test proving takeover becomes allowed.

## Fix Round 1

### Regression

Added a three-round regression to `diagnostics/startup_self_check_policy_test.cpp`:

- GPU is unavailable and reports a temperature of `0` on every round.
- CPU temperatures are valid on all three rounds.
- Each result has no blocking fault and exactly the `GPU 不可用` warning.
- The first two rounds remain monitoring-only; the third round permits takeover.

### Red

Command:

```powershell
& $env:COMSPEC /d /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul && cl /nologo /std:c++17 /utf-8 /EHsc diagnostics\startup_self_check_policy_test.cpp /Fe:diagnostics\startup_self_check_policy_test.exe && diagnostics\startup_self_check_policy_test.exe'
```

Result: failed as expected with `Assertion failed:
unavailableGpuFirst.blockingFaults.empty()` at
`diagnostics\\startup_self_check_policy_test.cpp`, line 41. This proved that
the existing policy incorrectly treated a zero GPU temperature as a blocking
fault even when GPU availability was false.

### Fix

Updated `StartupSelfCheckPolicy::Evaluate` so CPU temperature remains required
on every sample, while GPU temperature is required only when `gpuAvailable` is
true. GPU unavailability remains represented exclusively by the pre-existing
warning. No EC write path was changed.

### Green

Focused command:

```powershell
& $env:COMSPEC /d /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul && cl /nologo /std:c++17 /utf-8 /EHsc diagnostics\startup_self_check_policy_test.cpp /Fe:diagnostics\startup_self_check_policy_test.exe && diagnostics\startup_self_check_policy_test.exe'
```

Result: exit code 0; `startup_self_check_policy_test.cpp` compiled and ran
without assertion failures.

Full diagnostics command:

```powershell
& $env:COMSPEC /d /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul && for %f in (diagnostics\*_test.cpp) do @echo [diagnostic] %f & cl /nologo /std:c++17 /utf-8 /EHsc %f /Fe:%~dpnf.exe && %~dpnf.exe || exit /b 1'
```

Result: exit code 0. The following diagnostics all compiled and passed:
`autorun_command_policy_test`, `config_persistence_test`,
`control_verification_state_test`, `dialog_layout_policy_test`,
`sensor_health_policy_test`, `startup_self_check_policy_test`,
`startup_state_test`, and `temperature_alert_policy_test`.
