# Bootstrap ZIP hash registry V1

A different ZIP hash is not necessarily corruption. It is also not proof of safety. The wizard requires two separate confirmations for a locally selected variant and pins the observed BLAKE3 and SHA-256 in a private receipt. Automatic remote loading remains pinned.

The repository registry begins with zero approved entries: no invented hashes. A human-reviewed update may add a provenance-bound record after verifying source, license and exact binary identity. Candidate receipts must not automatically promote hashes to trusted status.

The core C freestanding pieces stay independent of Android. Android UI, filesystem operations, ZIP compression and transport are OS-backed adapters, not bare metal.

Physical ARM32/AArch64 and executable shell/pkg gates remain TOKEN_VAZIO until observed.
