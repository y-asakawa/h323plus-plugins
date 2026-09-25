# Third-Party Notices

This repository combines code from several projects and copyright holders.
There is no repository-wide single license. Source-file headers are
authoritative and must be retained.

This document is an attribution and license index, not legal advice and not a
replacement for the license text in a source file.

## Source notice preservation review

The source tree and private maintenance archive's source-import commits were
reviewed on 2026-06-24.
The obsolete `H.263-ffmpeg/ReadMe.txt` and Visual Studio 2008 instructions
were removed from the current source set. No imported `COPYING`, `LICENSE`, or
`NOTICE` file was found to have been removed from the repository history.

This history review does not prove that every upstream distribution file was
included in the original import. Source-file headers and the component-specific
review requirements below therefore remain authoritative.

## Standard license texts

The following standard texts are included for licenses referenced by source
headers:

| License | Text |
| --- | --- |
| Mozilla Public License 1.0 | [`LICENSES/MPL-1.0.txt`](LICENSES/MPL-1.0.txt) |
| Mozilla Public License 1.1 | [`LICENSES/MPL-1.1.txt`](LICENSES/MPL-1.1.txt) |
| GNU General Public License 2.0 or later | [`LICENSES/GPL-2.0-or-later.txt`](LICENSES/GPL-2.0-or-later.txt) |
| GNU Lesser General Public License 2.1 or later | [`LICENSES/LGPL-2.1-or-later.txt`](LICENSES/LGPL-2.1-or-later.txt) |

## H.323 Plus, OpenH323, and OPAL plugin code

Much of the plugin integration, makefiles, and shared support code originated
in H.323 Plus, OpenH323, or OPAL. Most such files identify MPL-1.0 in their
headers. Some newer H.323 Plus material identifies MPL-1.1 with a GPL
alternative.

Relevant locations include:

- `H.264/`
- `H.263-ffmpeg/`
- `common/`
- `H.264/core_patch/`

Copyright notices for Equivalence Pty. Ltd., Post Increment, Salyens, March
Networks Corporation, Matthias Schneider, Spranto Australia Pty Ltd., ISVO
(Asia) Pte. Ltd., Vox Lucida Pty. Ltd., and other contributors are preserved in
the corresponding files.

## PTLib and H.323 Plus

PTLib and H.323 Plus are build and runtime dependencies. Their source is not
vendored as a complete distribution here. Any binary distributor must record
the exact revisions used and comply with their applicable licenses.

- PTLib: https://github.com/willamowius/ptlib
- H.323 Plus: https://github.com/willamowius/h323plus

The files under `H.264/core_patch/` are patch material copied from or intended
for a matching H.323 Plus source tree. Their own headers remain authoritative.

## FFmpeg

`H.263-ffmpeg/` and `H.264/` use FFmpeg libraries for video processing. H.264
contains dynamic and static linking support. Historical FFmpeg API headers
formerly under `H.263-ffmpeg/ffmpeg/` were removed from the current tree.

FFmpeg is generally available under LGPL-2.1-or-later. Enabling GPL components
changes the effective license of the resulting FFmpeg build to a GPL license.
If `--enable-version3` is also enabled, GPL version 3-or-later may apply.

The exact license must be determined from the source revision, configure
options, and the output of `ffmpeg -L` for the specific build. Do not infer the
license of a binary from this repository alone.

Official licensing guidance: https://ffmpeg.org/legal.html

## x264

The H.264 plugin uses x264. Helper source under `H.264/gpl/` states
GPL-2.0-or-later. x264 itself is commonly distributed under GPL terms and may
also be available under separate commercial terms from its copyright holders.

A binary release must record the x264 revision, its selected license, build
configuration, and whether it is linked statically, dynamically, or loaded at
runtime.

## Excluded H.261 source

The H.261 plugin was excluded from this source set. Some VIC-derived files have
no redistribution grant in their headers, and the Indranet encoder files have
copyright notices without permission terms. Confirm the applicable rights
before restoring the plugin.

## Excluded G.722 source

The G.722 plugin was excluded from this source set. Its codec files include
CMU-derived code, and redistribution rights for that portion have not been
established. Do not restore the files without confirming those rights.

## Binary distribution

Before distributing a compiled plugin:

1. Complete [`docs/DEPENDENCY-LICENSE-REVIEW.md`](docs/DEPENDENCY-LICENSE-REVIEW.md).
2. Identify every linked or bundled library and its exact version.
3. Preserve all required notices and provide applicable license texts.
4. Provide corresponding source and relinking materials where required.
5. Confirm codec patent, royalty, export, and field-of-use obligations.
