# Excluded Codec Source Publication Review

Review date: 2026-09-25.

## Decision

Public redistribution rights for the previously bundled G.722.1 and G.722.2 /
AMR-WB reference code have not been established. Both plugin directories have
been removed from the current tree. G.722 and H.261 were also removed because
permission for portions of their source has not been established. `.gitignore`
excludes all four directories from future additions. The earlier commits that
contain those sources are retained only in a private maintenance archive. This
public source repository starts with a new history containing the reviewed tree.

## H.261

Several VIC-derived files lack a redistribution grant in their headers.
`p64encoder.cxx` and `p64encoder.h` carry Indranet copyright notices without
permission terms. A license on other H.261 files or on the plugin wrapper does
not establish rights for these files. Confirm rights with the relevant holders
before restoring this plugin to a public source tree.

## G.722

The G.722 encoder, decoder, and header credit Carnegie Mellon University
Speech Group code by Chengxiang Lu and Alex Hauptmann. The source files state
GPL/LGPL terms from Steve Underwood, but do not provide a separate permission
grant for the CMU-derived portions. Confirm the CMU rights before restoring
this plugin to a public source tree.

## G.722.1

`G.722.1/G722-1/` identifies ITU-T G.722.1 fixed-point Release 2.1 and carries
Polycom's "All rights reserved" notice. The ITU-T Release 2.1 archive contains
the same notice; its Readme directs inquiries about distribution of updated
software to ITU. Download availability is not a grant to republish the source.
The ITU-T G.191 license cited by basic operators applies to those files, not
to all Polycom code in the directory. The permissive notice in
`G.722.1/G7221Codec.cxx` applies to that wrapper file only.

Before including this code in a public source repository, establish the exact
redistribution and modification terms with the current copyright holder and
ITU. Patent rights and royalty terms must be checked separately.

Primary source: https://www.itu.int/rec/T-REC-G.722.1-200505-I/en

## G.722.2 / AMR-WB

Files under `G.722.2/AMR-WB/` identify themselves as the 3GPP AMR-WB
floating-point speech codec. The corresponding specification is 3GPP TS
26.204, published as ETSI TS 126 204. TS 26.173 is the fixed-point version;
the older wrapper description was inaccurate. The reference files contain no
redistribution license notice. The wrapper's Nimajin permission applies to
the wrapper file only.

The ETSI publication carries a written-permission copyright notice, and 3GPP
directs copyright authorization inquiries to ETSI Legal Service. Confirm
that authorization covers publication, modification, and redistribution of
the exact source package. Codec patent rights require a separate review.

Primary sources:

- https://www.etsi.org/deliver/etsi_ts/126200_126299/126204/06.00.00_60/ts_126204v060000p.pdf
- https://www.etsi.org/deliver/etsi_ts/126100_126199/126173/06.00.00_60/ts_126173v060000p.pdf
- https://www.3gpp.org/about-us/legal-matters
