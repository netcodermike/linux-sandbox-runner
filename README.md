# Linux Sandbox Runner

[![Build and verify](https://github.com/netcodermike/linux-sandbox-runner/actions/workflows/verify.yml/badge.svg)](https://github.com/netcodermike/linux-sandbox-runner/actions/workflows/verify.yml)


Explore Linux isolation by running a static executable in fresh namespaces with an empty tmpfs root, no external network, and resource limits.

**Language:** C11 / GCC or Clang  
**Platform:** Linux only; unprivileged user namespaces required  
**Status:** First-release learning project. See implemented scope and limitations below; this is not a production-completeness claim.

## Features

- User, mount, network, UTS, IPC, and PID namespaces.
- Private mount propagation and a 32 MiB tmpfs root.
- Only the selected executable is exposed as a read-only bind mount.
- CPU, address-space, file-size, descriptor, and core-dump limits plus a wall timeout.
- Optional cgroup v2 aggregate memory, PID, and CPU limits.

## Requirements and quick start

Clone the repository, enter its folder, and install the language/platform tools listed above. Commands run from the repository root. No credentials are required except for Backup Vault's explicitly generated secrets.

```sh
git clone https://github.com/netcodermike/linux-sandbox-runner.git
cd linux-sandbox-runner
make
cc -static -O2 examples/probe.c -o probe
./sandbox ./probe
# Optional, with an existing user-delegated cgroup v2 leaf:
SANDBOX_CGROUP=/sys/fs/cgroup/YOUR_DELEGATED_LEAF ./sandbox ./probe
```

## Verification

```sh
make && cc -static -O2 examples/probe.c -o probe && ./sandbox ./probe
```

Run the static probe in a disposable Linux VM. It should report PID 1, fail to open host files, and fail external networking. Build a static program that loops forever and confirm timeout/CPU termination. A simple nonzero exit code should propagate. With an explicitly delegated test cgroup, verify the configured controller files and cleanup the empty leaf yourself after the process exits.

See [validation notes](docs/VALIDATION.md) for what was actually executed during authoring. A committed CI workflow is a future check until its run is observed successful.

## Architecture and data

`sandbox.c` forks a supervisor, creates a user namespace and UID/GID mapping, creates isolation namespaces, mounts a new root, binds the selected binary read-only, then forks PID 1 inside the new PID namespace. The child chroots, applies rlimits and `PR_SET_NO_NEW_PRIVS`, closes inherited descriptors except stdin/stdout/stderr, clears the environment, and execs the static binary. The supervisor waits and returns its exit status or 128+signal, killing it after approximately five seconds. The outer process removes the empty host temporary directory after the mount namespace disappears.

`examples/probe.c` checks namespace PID, host `/etc/passwd` invisibility, and external network failure. `SANDBOX_CGROUP`, if supplied, must name an existing writable cgroup v2 leaf delegated to your user. The runner sets `memory.max=134217728`, `pids.max=16`, and `cpu.max=50000 100000`, then attaches the namespaced supervisor. Use a distinct empty leaf per invocation; the runner does not create or delete the delegated directory.

## Limitations and next steps

Educational isolation experiment, not a hardened boundary for hostile programs. No seccomp, syscall allowlist, complete capability drop, device policy, or audited escape resistance. Without a delegated cgroup, rlimits apply per process and do not bound aggregate descendant memory/process counts. Standard streams remain connected to the caller. Only static native executables are supported: no shared libraries, scripts, procfs, host data mounts, or DNS setup. Policies such as AppArmor may block unprivileged namespaces. Do not work around that by running this as host root.

## Troubleshooting

`Operation not permitted` at unshare means user namespaces are disabled/restricted on the host. Use a disposable Linux VM configured to allow them. `execve` ENOENT often means the binary is dynamically linked and its loader is absent; rebuild with `-static`. A cgroup permission error means the specified leaf was not delegated or its controllers are unavailable. A signal exit can reflect a configured resource limit. CI compiles this project but does not claim namespace execution succeeds on every runner.

## Development

Read the source entry point described above, make a focused change, run the verification command, and exercise the relevant manual scenario. [Contributing](CONTRIBUTING.md) explains bug reports and pull requests; [security notes](SECURITY.md) describe the trust assumptions. Runtime/generated files are ignored. No telemetry or paid API dependencies are included.

## License

[MIT](LICENSE), copyright 2026 netcodermike.
