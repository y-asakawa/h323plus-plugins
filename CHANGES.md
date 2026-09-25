# Change Log

This file records material local changes to the H.323 Plus, OpenH323, OPAL,
and third-party codec sources maintained in this repository. Historical
upstream change logs remain in individual source files.

## 2026

### September 2026

- Verified H.263 macOS build and four-frame encode/decode round trips for
  QCIF, CIF, and 4CIF using 1200-byte RFC 2190 RTP packets. Fixed modern
  FFmpeg loading, codec lifetime, encoder output size, rate control setup,
  RTP packetization and reassembly, and option reinitialization. Removed the
  mislabeled H.263-720 entry, which actually declared CIF dimensions.
- Removed H.263 Visual Studio project files; kept its macOS Makefiles and
  codec source.
- Excluded H.261 from the current source tree because some VIC-derived and
  Indranet files lack confirmed redistribution permission.
- Excluded G.722 from the current source set because the redistribution terms
  for the CMU-derived portions could not be established.
- Removed unused historical FFmpeg headers and obsolete build instructions
  from the H.263 plugin source tree.
- Fixed the standalone H.263 Makefiles to find a sibling H.323 Plus checkout;
  verified its macOS build after removing the obsolete headers.
- Removed H.264 Windows-only pipe, helper, and x264 loader files; kept the
  Unix helper path used by macOS builds.
- Removed obsolete Visual Studio 2008 project files, the H.264 solution,
  and its outdated build instructions.
- Verified the H.264 plugin build and an encode/decode round trip on macOS;
  replaced a private FFmpeg decoder symbol with its public API, corrected the
  static-link build defaults, and removed temporary encoder diagnostics.
- Corrected the AMR-WB reference-code attribution to floating-point 3GPP TS
  26.204 and removed an unverified G.722.1 royalty statement.
- Recorded the source redistribution review and excluded the G.722.1 and
  G.722.2 / AMR-WB plugin directories in `.gitignore`.
- Removed both plugins from the maintenance repository's current tree; their
  earlier commits remain only in the private maintenance archive.

### June 2026

- Added public-repository README, security policy, multi-license guidance,
  third-party notices, standard license texts, and dependency review records.
- Removed machine-specific paths and generated host-version details from
  checked-in build files and maintenance notes.
- Added CodeQL and Dependabot GitHub configuration.
- Updated `H.263-ffmpeg/Makefile`, `H.263-ffmpeg/Makefile.in`, and
  `H.263-ffmpeg/h263ffmpeg.cxx` to discover current FFmpeg compiler and linker
  flags and handle current FFmpeg APIs.
- Updated `H.264/Makefile*` and `H.264/gpl/Makefile*` to propagate architecture
  and linker flags, including Apple Silicon CI support.
- Corrected multiplication promotion in `H.261-vic/vic/p64.cxx` as reported by
  CodeQL.
- Consolidated shared video helper files under `common/`.

### February 2026

- Imported the G.722 plugin source for local maintenance.
- Added H.264 static-link work, helper scripts, maintenance notes, and H.323
  Plus core-side patch material.

### January 2026

- Added H.264 encoder preview callback support.

## 2025

### December 2025

- Imported the initial H.323 Plus video plugin source set for local FFmpeg and
  macOS maintenance.

## Source origins

The maintained sources derive from H.323 Plus, OpenH323, OPAL, FFmpeg, and
other contributors identified in source-file
headers and
[`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md).

This log does not replace per-file copyright, license, or historical change
notices.
