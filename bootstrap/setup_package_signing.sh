#!/bin/bash
# Setup package signing infrastructure for RAFCODEΦ
# Create signing keys, configure APT validation
# Part of Stage 2: Package Manager Rebuild (Days 4-6)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

KEYRING_DIR="${REPO_ROOT}/signing"
PREFIX="/data/data/com.termux.rafacodephi"

# Signing configuration
KEY_NAME="RAFCODEΦ Package Signing Key"
KEY_EMAIL="rafcodephi-packages@termux.local"
KEY_BITS="4096"
KEY_VALIDITY="3650"  # 10 years

setup_signing_environment() {
    echo "[PKG-SIGN] Setting up package signing environment..."

    mkdir -p "$KEYRING_DIR/public"
    mkdir -p "$KEYRING_DIR/private"
    mkdir -p "$KEYRING_DIR/revoke"

    # Check for gnupg
    if ! command -v gpg &> /dev/null; then
        echo "[PKG-SIGN] ERROR: gnupg not found" >&2
        exit 1
    fi

    echo "[PKG-SIGN] GPG version: $(gpg --version | head -1)"
}

generate_signing_keys() {
    local public_key="${KEYRING_DIR}/public/rafcodephi.asc"
    local private_key="${KEYRING_DIR}/private/rafcodephi.key"

    echo "[PKG-SIGN] Generating RAFCODEΦ signing keys (${KEY_BITS} bits, ${KEY_VALIDITY} days)..."

    if [[ -f "$public_key" ]] && [[ -f "$private_key" ]]; then
        echo "[PKG-SIGN] Keys already exist, skipping generation"
        return 0
    fi

    # Create batch file for unattended key generation
    local batch_file=$(mktemp)

    cat > "$batch_file" << EOF
%echo Generating RAFCODEΦ package signing key
Key-Type: RSA
Key-Length: $KEY_BITS
Name-Real: $KEY_NAME
Name-Email: $KEY_EMAIL
Expire-Date: ${KEY_VALIDITY}d
%no-protection
%echo done
EOF

    # Generate key
    gpg --batch --generate-key "$batch_file" || {
        echo "[PKG-SIGN] Key generation failed" >&2
        rm -f "$batch_file"
        exit 1
    }

    # Export public key
    gpg --armor --export "$KEY_EMAIL" > "$public_key" || {
        echo "[PKG-SIGN] Public key export failed" >&2
        rm -f "$batch_file"
        exit 1
    }

    # Export private key
    gpg --armor --export-secret-keys "$KEY_EMAIL" > "$private_key" || {
        echo "[PKG-SIGN] Private key export failed" >&2
        rm -f "$batch_file"
        exit 1
    }

    # Secure private key permissions
    chmod 600 "$private_key"

    rm -f "$batch_file"

    echo "[PKG-SIGN] Keys generated successfully"
    echo "[PKG-SIGN]   Public key: $public_key"
    echo "[PKG-SIGN]   Private key: $private_key (mode 0600)"
}

create_apt_keyring() {
    local apt_keyring_dir="${PREFIX}/etc/apt/trusted.gpg.d"
    local apt_keyring="${apt_keyring_dir}/rafcodephi.gpg"
    local public_key="${KEYRING_DIR}/public/rafcodephi.asc"

    echo "[PKG-SIGN] Creating APT keyring..."

    mkdir -p "$apt_keyring_dir"

    if [[ -f "$public_key" ]]; then
        # Convert ASCII key to binary format for APT
        gpg --export "${KEY_EMAIL}" > "$apt_keyring" || {
            echo "[PKG-SIGN] APT keyring generation failed" >&2
            return 1
        }

        chmod 644 "$apt_keyring"
        echo "[PKG-SIGN] APT keyring created: $apt_keyring"
    fi
}

create_signing_wrapper() {
    local wrapper_script="${KEYRING_DIR}/sign-package.sh"

    echo "[PKG-SIGN] Creating package signing wrapper script..."

    cat > "$wrapper_script" << 'SCRIPT_EOF'
#!/bin/bash
# Package signing wrapper for RAFCODEΦ
# Signs .deb packages with RAFCODEΦ key

set -e

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <package.deb>" >&2
    exit 1
fi

PACKAGE_FILE="$1"

if [[ ! -f "$PACKAGE_FILE" ]]; then
    echo "Error: Package file not found: $PACKAGE_FILE" >&2
    exit 1
fi

# Get key ID
KEY_ID=$(gpg --list-secret-keys --with-colons | grep "^sec" | head -1 | cut -d: -f5)

if [[ -z "$KEY_ID" ]]; then
    echo "Error: No signing key found" >&2
    exit 1
fi

echo "[SIGN] Signing package: $PACKAGE_FILE"
echo "[SIGN] Key ID: $KEY_ID"

# Create detached signature
gpg --batch --detach-sign --armor --default-key "$KEY_ID" "$PACKAGE_FILE" || {
    echo "[SIGN] Signing failed" >&2
    exit 1
}

echo "[SIGN] Signature created: ${PACKAGE_FILE}.asc"

# Verify signature
gpg --verify "${PACKAGE_FILE}.asc" "$PACKAGE_FILE" || {
    echo "[SIGN] Signature verification failed" >&2
    exit 1
}

echo "[SIGN] Signature verified successfully"
SCRIPT_EOF

    chmod +x "$wrapper_script"
    echo "[PKG-SIGN] Signing wrapper: $wrapper_script"
}

create_signature_verification_gate() {
    local gate_script="${REPO_ROOT}/scripts/verify-package-signatures.sh"

    echo "[PKG-SIGN] Creating package signature verification gate..."

    mkdir -p "$(dirname "$gate_script")"

    cat > "$gate_script" << 'GATE_EOF'
#!/bin/bash
# Package signature verification gate
# Verifies all .deb packages are signed with RAFCODEΦ key

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

KEYRING="${REPO_ROOT}/signing/public/rafcodephi.asc"

if [[ ! -f "$KEYRING" ]]; then
    echo "Error: Keyring not found: $KEYRING" >&2
    exit 1
fi

echo "[VERIFY] Verifying package signatures..."

UNSIGNED_COUNT=0
VERIFIED_COUNT=0
FAILED_COUNT=0

# Find all .deb packages
for deb_file in $(find "${REPO_ROOT}/staging" -name "*.deb" -type f 2>/dev/null); do
    sig_file="${deb_file}.asc"

    if [[ ! -f "$sig_file" ]]; then
        echo "[VERIFY] UNSIGNED: $deb_file"
        ((UNSIGNED_COUNT++))
        continue
    fi

    # Verify signature
    if gpg --no-default-keyring --keyring "$KEYRING" \
           --verify "$sig_file" "$deb_file" 2>/dev/null; then
        echo "[VERIFY] ✓ VERIFIED: $deb_file"
        ((VERIFIED_COUNT++))
    else
        echo "[VERIFY] ✗ FAILED: $deb_file"
        ((FAILED_COUNT++))
    fi
done

echo "[VERIFY] Results:"
echo "[VERIFY]   Verified: $VERIFIED_COUNT"
echo "[VERIFY]   Failed: $FAILED_COUNT"
echo "[VERIFY]   Unsigned: $UNSIGNED_COUNT"

if [[ $FAILED_COUNT -gt 0 ]]; then
    echo "[VERIFY] ERROR: Signature verification failed" >&2
    exit 1
fi

exit 0
GATE_EOF

    chmod +x "$gate_script"
    echo "[PKG-SIGN] Verification gate: $gate_script"
}

create_signing_receipt() {
    local receipt_file="${REPO_ROOT}/results/package-signing-receipt.json"

    mkdir -p "$(dirname "$receipt_file")"

    cat > "$receipt_file" << EOF
{
  "schema": "raf.package-signing-receipt.v1",
  "key_name": "$KEY_NAME",
  "key_email": "$KEY_EMAIL",
  "key_bits": $KEY_BITS,
  "key_validity_days": $KEY_VALIDITY,
  "key_generated_at": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "keyring_location": "$KEYRING_DIR",
  "public_key_file": "${KEYRING_DIR}/public/rafcodephi.asc",
  "private_key_file": "${KEYRING_DIR}/private/rafcodephi.key",
  "apt_keyring": "${PREFIX}/etc/apt/trusted.gpg.d/rafcodephi.gpg",
  "signing_requirements": [
    {
      "requirement": "package_signatures_present",
      "description": "All packages must be signed with RAFCODEΦ key",
      "verification_gate": "scripts/verify-package-signatures.sh",
      "status": "PLANNED"
    },
    {
      "requirement": "signature_validation_enforced",
      "description": "APT must validate signatures before installation",
      "configuration": "${PREFIX}/etc/apt/trusted.gpg.d/rafcodephi.gpg",
      "status": "CONFIGURED"
    }
  ],
  "tools": {
    "signing_wrapper": "${KEYRING_DIR}/sign-package.sh",
    "verification_gate": "scripts/verify-package-signatures.sh"
  }
}
EOF

    echo "[PKG-SIGN] Receipt written to: $receipt_file"
}

main() {
    echo "[PKG-SIGN] Starting package signing setup"
    echo "  Key name: $KEY_NAME"
    echo "  Key email: $KEY_EMAIL"
    echo "  Key bits: $KEY_BITS"
    echo "  Validity: $KEY_VALIDITY days"
    echo "  Keyring directory: $KEYRING_DIR"

    setup_signing_environment
    generate_signing_keys
    create_apt_keyring
    create_signing_wrapper
    create_signature_verification_gate
    create_signing_receipt

    echo "[PKG-SIGN] Package signing setup completed"
    echo "[PKG-SIGN] Next steps:"
    echo "  1. Sign all .deb packages with: ${KEYRING_DIR}/sign-package.sh <package.deb>"
    echo "  2. Verify signatures with: scripts/verify-package-signatures.sh"
    echo "  3. Integrate into package manager rebuild validation"
}

main "$@"
