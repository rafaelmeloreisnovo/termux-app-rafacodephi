# Bootstrap contract coverage — 2026-10-24

Base/rollback: `20d80c6925cf0cfe328e72afde4f60363e43e6e1`.
Authority: local Android producer CI only, user-authorized draft hotfix.
Governance=DRAFT_NO_PROMOTION; data=PUBLIC_SOURCE; privacy=NO_DEVICE_DATA;
security=FAIL_CLOSED_TEST_IDENTITY. Mapa route=TOKEN_VAZIO; no state promotion.

Observed gap: importer was absent from PR/push filters; push also omitted bootstrap
prepare, profile tool, installer and START HERE workflow, although those affect or
are checked by this contract. Preserve all existing events/paths, add missing inputs.
Checkout PR head explicitly and verify it with actions/rafcode/verify-source pinned
to `7c03495ebe1d28a11ea419799c4f61427250244f` (producer draft PR #17).

Local evidence: existing 11 beta policy tests and 4 startup/settings contracts PASS
before and after; YAML parsed; importer/prepare/profile/installer coverage verified
for both push and pull_request; shell syntax PASS. No App Java, installation logic,
bootstrap bytes, signature, release workflow or upstream license modified.
Bash/Git shared action is hosted CI infrastructure, not freestanding; no Node added.

CI exact head NOT_YET_OBSERVED; APK/physical Android/pkg transaction NOT_RUN.
Rollback: revert this scoped CI commit, retain baseline; no history/branch deletion.
F_ok=missing triggers covered and source identity checked;
F_gap=remote CI and real bootstrap/APK/device chain;
F_next=exact-head contract run then artifact-bound Android validation.
claim_allowed=false; release_allowed=false.
