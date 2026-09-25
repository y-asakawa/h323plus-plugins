# Dependency and License Review

This repository contains source code. A successful build does not
establish that a compiled plugin may be redistributed.

The H.261, G.722.1, G.722.2 / AMR-WB, and G.722 sources were removed from the
current tree. Their earlier commits remain in a private maintenance archive;
this public repository does not contain that history. See
[`SOURCE-PUBLICATION-REVIEW.md`](SOURCE-PUBLICATION-REVIEW.md).

Complete a copy of the release record below for every binary release and keep
it with the release artifacts.

## Current source review

| Component | Repository content or dependency | Review status |
| --- | --- | --- |
| H.323 Plus / OpenH323 / OPAL code | MPL-1.0 and some MPL-1.1/GPL alternatives in file headers | Indexed |
| H.264 helper code | GPL-2.0-or-later and MPL/GPL alternatives in file headers | Indexed |
| FFmpeg | External dependency | Per-build review required |
| x264 | External dependency used by H.264 | Per-build review required |
| PTLib and H.323 Plus | External build/runtime dependencies | Per-build review required |

The H.264 static build links GPL-licensed helper code and x264 with files
carrying MPL-1.0 notices. A working local build does not establish that the
resulting combined binary may be redistributed. Obtain a specific license
compatibility review before any H.264 binary release.

## Release record

### Release identity

- Repository commit:
- Release tag:
- Build date:
- Builder/CI run:
- Target operating system:
- Target architecture:
- Artifact names and SHA-256 hashes:

### Toolchain

- Compiler and version:
- Linker and version:
- SDK/sysroot:
- PTLib repository and commit:
- H.323 Plus repository and commit:

### FFmpeg

- FFmpeg version and commit:
- Source URL:
- `ffmpeg -version` output attached: yes/no
- `ffmpeg -L` output attached: yes/no
- `ffmpeg -buildconf` output attached: yes/no
- Configure options:
- Effective license (LGPL/GPL/other):
- `--enable-gpl` enabled: yes/no
- `--enable-version3` enabled: yes/no
- Static, dynamic, or runtime loading:
- Corresponding source location:
- Local patches:

FFmpeg's official guidance states that optional GPL components can change the
effective license of the FFmpeg build, and `--enable-version3` can make
GPL-3.0-or-later applicable. Determine the result from the actual configure
options and `ffmpeg -L`. Review: https://ffmpeg.org/legal.html

### x264

- x264 version and commit:
- Source URL:
- Build configuration:
- GPL or separately licensed build:
- Static, dynamic, or runtime loading:
- Corresponding source location:
- Local patches:

### Binary inspection

Record direct and transitive dependencies from the completed artifact.

macOS:

```sh
file path/to/plugin.dylib
otool -L path/to/plugin.dylib
codesign -dvv path/to/plugin.dylib
shasum -a 256 path/to/plugin.dylib
```

Linux:

```sh
file path/to/plugin.so
readelf -d path/to/plugin.so
ldd path/to/plugin.so
sha256sum path/to/plugin.so
```

Windows:

```powershell
Get-FileHash path\to\plugin.dll -Algorithm SHA256
dumpbin /DEPENDENTS path\to\plugin.dll
```

### Distribution checklist

- [ ] Every compiled source file has been mapped to its license.
- [ ] Direct and transitive binary dependencies have been recorded.
- [ ] Required license texts and copyright notices accompany the release.
- [ ] Corresponding source is available where GPL/LGPL terms require it.
- [ ] LGPL relinking or replacement requirements have been addressed.
- [ ] Build scripts, configure options, and local patches are available.
- [ ] FFmpeg and x264 effective licenses have been confirmed from actual builds.
- [ ] Export, trademark, and field-of-use restrictions have been reviewed.

## Decision

- Reviewer:
- Review date:
- Approved artifacts:
- Excluded artifacts:
- Conditions or follow-up actions:

Do not publish a binary when any required item remains unknown.
