# Validation record

Authoring host: Windows, November 6, 2026. Requirements: C11 / GCC or Clang; Linux only; unprivileged user namespaces required.

The README verification command is `make && cc -static -O2 examples/probe.c -o probe && ./sandbox ./probe`. The companion GitHub Actions workflow describes reproducible build/test checks. Merely including that workflow does not mean it ran successfully. Host-specific verification results are recorded before publication.

Manual acceptance steps are in README.md. GUI behavior, hardware sensors, permissions, audio output, and OS isolation need verification on their target platform; compilation alone cannot establish these properties.

## Authoring result

Target toolchain/platform unavailable on the Windows authoring host. Source and build configuration were reviewed, and a target-platform GitHub Actions build/check workflow is provided. No successful native build or runtime result is claimed yet.

## Observed GitHub Actions result

Passed on November 6, 2026: [build/check run](https://github.com/netcodermike/linux-sandbox-runner/actions/runs/42196136745).

Verified source commit: `012f0caf4e9d47bfb3a629ac0a7e85d205fa947f`.

Strict-warning C compilation of the sandbox and static probe. Namespace execution was not exercised by CI.

This record may be committed after the linked source-validation run; the linked SHA identifies exactly what was checked. Native GUI/hardware acceptance remains manual as described in README.md.
