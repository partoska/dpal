# Security Policy

## Supported Versions

Only the latest release receives security fixes.

## Reporting a Vulnerability

Please **do not** open a public GitHub issue for security vulnerabilities.

Use [GitHub's private vulnerability reporting](https://github.com/partoska/dpal/security/advisories/new) to submit a report. You can expect:

- Acknowledgement within **168 hours**.
- A fix or mitigation plan within **90 days**, depending on severity.
- Credit in the release notes if you would like it.

## Scope

This tool runs entirely on your local machine and manages processes via SysV message queues. If you find an issue related to privilege escalation, unsafe signal handling, IPC exposure, or insecure process lifecycle management, please report it.
