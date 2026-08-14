# Workflow Control Plane V1 — fileset

## Added

- `.github/workflows/00-rafaelia-control-plane.yml`
- `.github/workflows/_reusable-arm32-compat.yml`
- `scripts/ci/workflow_control_plane.py`
- `tests/test_workflow_control_plane_contract.py`
- `docs/WORKFLOW_CONTROL_PLANE.md`
- `docs/USER_RUNBOOK_ARM32_BOOTSTRAP.md`
- `docs/audits/WORKFLOW_CONTROL_PLANE_REFACTOR_20260814_V1.md`
- `data/governance/workflow-control-plane-v1-20260814.json`

## Refactored

- `.github/workflows/compatibility-arm32.yml`
- `.github/workflows/compatibility-arm32-ndk29.yml`
- `.github/workflows/run_tests.yml`
- `.github/workflows/beta-real-bootstrap-contract.yml`
- `.github/workflows/apk-evidence-gate.yml`
- `docs/CI_WORKFLOW_OWNERSHIP.md`

## Invariant

The complete `.github/workflows` tree is discovered by the control-plane scanner. Specialist workflows not yet migrated remain visible and governed rather than silently rewritten.
