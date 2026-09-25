# H.323 Plus H.264 and H.263 video plugins

H.323 Plus / PTLib plugin API for external audio and video codec plugins.

This repository is a source distribution and maintenance workspace for codec
plugins used with H.323 Plus based applications. It contains original upstream
plugin code plus local maintenance changes, especially around macOS builds and
the H.264/x264/FFmpeg plugin.

H.261, G.722.1, G.722.2 / AMR-WB, and G.722 were excluded because redistribution
rights for portions of their source could not be established. See
[`docs/SOURCE-PUBLICATION-REVIEW.md`](docs/SOURCE-PUBLICATION-REVIEW.md).
The earlier maintenance history containing those sources remains private;
this source repository begins with a new, reviewed Git history.

## Included plugins

| Directory | Plugin | Notes |
| --- | --- | --- |
| `H.263-ffmpeg/` | H.263 video | FFmpeg/libavcodec based H.263 plugin. |
| `H.264/` | H.264 video | x264/FFmpeg based H.264 plugin, including static-link work notes. |
| `common/` | shared video helpers | Shared tracing, dynamic loading, RTP, and compatibility helpers. |

## Repository policy

The repository is intended to contain source code, build scripts, project files,
and maintenance notes only.

Do not commit generated build output such as:

- `obj/`
- `*.o`
- `*.dylib`
- `*.so`
- `*.dll`
- local editor/workspace files
- OS metadata such as `.DS_Store`

Generated plugin binaries must not be committed to the source tree.

If binary distribution has been reviewed and all applicable license, source
availability, notice, patent, and royalty requirements have been satisfied,
the resulting binaries may be published through GitHub Releases or another
appropriate distribution channel.

## Build requirements

The exact build environment depends on the plugin.

Common requirements:

- H.323 Plus headers and libraries
- PTLib headers and libraries
- a C/C++ toolchain supported by the original H.323 Plus plugin build system

Additional requirements:

- `H.264/`: FFmpeg/libavcodec/libavutil and x264, or the static-link workflow
  documented in `H.264/H264_STATIC_LINK_REWORK_20260227.txt`
- `H.263-ffmpeg/`: FFmpeg/libavcodec/libavformat/libavutil compatible with the plugin source

The checked-in `Makefile` files reflect the last local generated state. For
portable changes, update the matching `Makefile.in` where present.

For a standalone H.264 build, place H323Plus beside this repository or set
`OPENH323DIR` to its source directory. On macOS, the H.264 Makefile uses the static
FFmpeg archives under `$OPENH323DIR/third_party/static_ffmpeg/install` and
x264's static archive when available; otherwise it uses libraries found by
`pkg-config`. Check the resulting dependencies with `otool -L` on macOS.

## Typical local build

Build each plugin from its directory:

```sh
cd H.263-ffmpeg
make

cd ../H.264
make
```

The generated plugin files are ignored by git. Install them into the plugin
directory expected by your H.323 Plus / PTLib runtime.

## H.264 maintenance notes

The H.264 plugin contains local work for reducing dynamic Homebrew dependency
coupling on macOS. See:

- `H.264/H264_STATIC_LINK_REWORK_20260227.txt`
- `H.264/CORE_PATCH_BUNDLE_NOTE_20260227.txt`
- `H.264/core_patch/`

`H.264/core_patch/` contains H.323 Plus core-side files that were bundled with
the H.264 maintenance work. Treat them as patch material for a matching H.323
Plus source tree, not as standalone plugin code.

## Security

See `SECURITY.md` for vulnerability reporting and supported usage notes.

## License

This repository is a multi-license source distribution. There is no single
license that applies to every file. The license header in each source file is
authoritative.

Many H.323 Plus, OpenH323, and OPAL plugin files identify MPL-1.0 in their
source headers. Other files identify MPL-1.1, GPL-2.0-or-later, BSD-style,
or project-specific terms. External dependencies have their own terms.
Files without a clear
license notice must not be assigned a repository-wide default license.

Complete standard license texts and additional licensing information are
available in:

- [`LICENSE`](LICENSE) - repository-level multi-license guidance
- [`LICENSES/`](LICENSES/) - standard license texts used by included files
- [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md) - component and
  attribution index
- [`CHANGES.md`](CHANGES.md) - local modification history
- [`docs/DEPENDENCY-LICENSE-REVIEW.md`](docs/DEPENDENCY-LICENSE-REVIEW.md) -
  dependency and binary-release review record

The H.264 and H.263 plugins may link to FFmpeg, and the H.264 plugin may also
link to x264. The effective distribution requirements depend on the exact
FFmpeg/x264 builds, configure options, and static or dynamic linking method.

Publishing this source repository does not by itself authorize redistribution
of compiled plugins. Before publishing `.dylib`, `.so`, `.dll`, or other
binaries, complete the dependency review and satisfy all applicable source,
relinking, notice, patent, and royalty obligations.

## Related projects

- H.323 Plus: https://github.com/willamowius/h323plus
- PTLib: https://github.com/willamowius/ptlib
