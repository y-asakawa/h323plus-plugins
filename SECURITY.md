# Security Policy

## Supported scope

Security review and fixes are handled for the current `main` branch of this
source repository.

This repository does not provide long-term support for historical snapshots,
locally built binaries, or binaries built against unknown PTLib, H.323 Plus,
FFmpeg, or x264 versions.

## Reporting a vulnerability

Please report suspected vulnerabilities privately.

Preferred method:

1. Use GitHub's private vulnerability reporting or Security Advisory feature for
   this repository.
2. Include the affected plugin, platform, dependency versions, build options,
   reproduction steps, and whether the issue requires a crafted media stream,
   signaling message, or local build input.

If private reporting is not available, open a minimal public issue asking for a
private contact path. Do not include exploit details, crash payloads, or
proof-of-concept inputs in a public issue.

## Handling expectations

Reports will be triaged based on reproducibility, affected code path, and
whether the issue is in this repository or in an upstream dependency.

Issues in FFmpeg, x264, PTLib, H.323 Plus, or codec reference code may need to
be reported upstream as well. This repository may still carry a mitigation or
documentation update when appropriate.

## Security-relevant build guidance

- Build against maintained dependency versions where possible.
- Avoid committing generated binaries, object files, local credentials, private
  keys, or environment files.
- Rebuild plugin binaries after updating FFmpeg, x264, PTLib, or H.323 Plus.
- Treat incoming RTP/H.323 media and signaling as untrusted input.
