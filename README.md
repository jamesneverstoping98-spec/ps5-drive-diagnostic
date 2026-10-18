# PS5 Drive Diagnostic — development source

> **Development status: simulation/source only. No PS5 console, real optical drive, game, or actual emulator integration has been tested.**
> Uncompiled PS5 source preview. Host C builds failed; no C tests or PS5 builds completed. No installable binary is provided.

Targets: original PS5 and PS5 Slim, using an external USB CD drive. This is a small, single-run diagnostic, not a game player. It issues INQUIRY, TEST UNIT READY, READ TOC and two one-sector READ CD requests at each track's INDEX 01. It writes a JSONL report, closes each device, and exits. It does not copy a disc, eject a tray, change drive speed, or launch an emulator. CAM opens device handles read/write because the pass-through interface requires it; the SCSI commands themselves request no media writes.

**Delivered status:** source and build recipes, plus C host-mock tests and the attempted-build log. No PS5 binary or installable package has been built or tested. The CAM code is adapted from verified SwanStationPS5 source, not proof this app will work on a console. The Windows development environment had neither WSL nor a PS5 SDK; the included build recipes require a Linux/WSL build host. Original PS5 and Slim remain untested.

## Files

- `src/main.c`: one-shot drive scan, session/summary logging.
- `src/probe.c`: strict TOC parsing, sampling, short-transfer checks, repeat CRC32.
- `src/transport_cam.c`: PS5 CAM transport, adapted from pinned SwanStationPS5; non-PS5 compilation guard.
- `tests/`: C TOC and mock-transport fixtures.
- `tools/test_host.py`: compiles the actual C probe and validates its JSONL output using simulated responses.
- `results/host-tests.json`: attempted host build and its failure log. Mock JSONL files will be produced only after a successful host build; those sessions are explicitly tagged `host_mock`.
- `vendor/PROVENANCE.md` and `LICENSE`: upstream attribution and GPL licensing.

## Host checks

Python 3.10+ and a C11 compiler are required:

```sh
python3 tools/test_host.py
# Or, with Zig available:
python3 tools/test_host.py --cc zig cc
```

No PS5 SDK is needed for these tests. They cover mixed data/audio samples, unit attention retry, absent drive, empty tray, disconnect, truncated TOC, permanent read error, short transfer, unstable repeat data, and TOC bounds/order validation. CRC32 is a repeat-consistency diagnostic, not an authenticity check or game compatibility claim. The mock never loads the PS5 CAM branch.

## Build an ELF payload first

Use Linux/WSL, with the ps5-payload-dev SDK v0.42 extracted. This version is the one referenced by the pinned upstream's setup script:
https://github.com/ps5-payload-dev/sdk/releases/tag/v0.42

```sh
export PS5_PAYLOAD_SDK=/absolute/path/to/ps5-payload-sdk
bash tools/build-payload.sh
```

Expected output: `dist/PS5DriveDiagnostic.elf`. This is a build recipe, not a verified build result. The build recipes explicitly select `__PROSPERO__`; compiling this transport without it intentionally fails. SDK CAM headers, symbols, loader behavior and filesystem permissions must be validated on the actual build/console environment.

On an already jailbroken console with a working ELF loader, the payload is intended to be sent through that loader. There is no jailbreak or privilege-escalation implementation in this project. Do not run SwanStationPS5's disc daemon or other optical-drive pollers concurrently. Their command streams can consume each other's media-change notices.

The default output is `/data/PS5DriveDiagnostic/report.jsonl`; running again replaces it. Save each report before rerunning. If `/data` is inaccessible, the app exits 2 and reports the failure to stderr. The current payload has no screen UI or notifications. It exits after the scan. Retrieve the report using your existing console file-access method. The app does not upload anything.

## Build a home-screen homebrew app

Use a **dedicated checkout** of the verified source so diagnostic staging and its build outputs stay separate from your emulator work:

```sh
git clone https://github.com/darkxex/SwanStationPS5.git swan-diagnostic-build
cd swan-diagnostic-build
git checkout b995783c93b1e28560a23d67335e5ee2e257e2db
cd ..
export SWANSTATION_SOURCE="$PWD/swan-diagnostic-build"
bash /absolute/path/to/ps5-drive-diagnostic/tools/build-app.sh folder
# Supported by that upstream build pipeline: ffpkg or ffpfsc instead of folder.
```

Follow that repository's Linux build prerequisites (clang-18, lld-18, LLVM, make, ninja, Python, etc.). Its build scripts fetch required dependencies. This recipe directly invokes the upstream app builder with diagnostic sources and no emulator/Vulkan archives. It stages separate metadata and reuses upstream presentation assets for the development build; the title is `PS5 Drive Diagnostic`. Local homebrew title ID `PPSA98519` is only a placeholder, not a registered or collision-checked identifier. Change it if already used in your environment.

Expected output is the upstream `dist/PPSA98519` app folder, ZIP, or requested package format. This route is unbuilt and untested here. Use only the installation method supported by your existing homebrew loader; the project does not claim universal firmware/package support. The app runs once and exits; a black or brief screen is expected because it has no renderer. A proper on-screen UI is a later milestone after drive communication is confirmed.

## Interpreting reports

Each line is one JSON object. `session` declares backend and scope; `open` records enumeration; `command` records opcode, stage, requested/actual bytes, result, packed sense and OS errno; `inquiry` provides hex-encoded vendor/product/revision bytes; `toc`, `track`, and `sample` record media layout and sample checks; `summary` records counts. Identity hex decodes as ASCII bytes 8–35 of standard INQUIRY. Sense is `(key << 16) | (ASC << 8) | ASCQ`. Zero sense on an OS failure does not mean a good sector. OS errno is meaningful for OS failures, not device status.

Exit codes: 0 means every opened candidate passed its narrow sample check; 1 means no candidate opened or at least one probe failed; 2 means the report could not be written. Even exit 0 does not certify an entire disc or USB drive model. Tracks are read twice, not retried on arbitrary errors; only readiness unit-attention errors receive up to three attempts. Each command requests a 5-second OS timeout. Total runtime depends on the number of drives/tracks and the kernel honoring those timeouts.

The scan tries cd units 0–7 and relies on CAM to map them to pass devices. It does not identify USB bus topology, so an opened candidate is not proof it is external USB. It rejects non-CD peripheral types, malformed/incomplete TOCs, and TOCs not starting at track 1. It does not support negative-LBA/pregap reads, multisession interpretation, subchannels, authentication, or full-disc checks. Permission denial and unsupported kernel paths must be distinguished from an absent drive using the recorded errors. A PS5's internal drive does not gain CD support from this program.

For a real test, record original vs Slim, console firmware, jailbreak/loader version, SDK build revision, external drive model/firmware and USB bridge, media identity and disc type alongside the report. These values are deliberately not guessed; `model` is `unrecorded`. First test detection with no disc, then a known readable data CD, then mixed-mode media. Stop if the console becomes unresponsive. Collect logs before changing software or hardware so results can be compared.

## Next milestone

A successful console log with INQUIRY, valid TOC, full-length reads and matching repeated CRCs would establish basic communication for that exact setup. Only then add screen controls, insertion/removal polling, longer reads, cancellation and SwanStationPS5 integration. No emulator API or game execution is called here.

## Validation recorded for this development snapshot

The C host test runner was attempted with Zig 0.13.0 and 0.15.2, but both failed with Windows sandbox file-access errors before producing an executable. TinyCC 0.9.27 was also attempted but Windows rejected its process with error 623 (DLL relocation). No C tests passed or ran in this session. `results/host-tests.json` preserves the Zig 0.15.2 failure; `results/build-status.json` summarizes all build limitations. The earlier Python virtual kit results remain separate and do not validate this C diagnostic. The source is a development snapshot requiring compilation and runtime validation.

## Credits and license

Created by James, with software development and documentation assisted by OpenAI Codex. Development release; no hardware or game compatibility has been established. Distributed under GPL-3.0-or-later; see LICENSE.
