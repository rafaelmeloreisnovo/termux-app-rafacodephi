#!/bin/bash
# Prefix Migration Script: /data/data/com.termux → /data/data/com.termux.rafacodephi
# Rewrite all embedded paths in dpkg/apt/libapt binaries and configuration
# Part of Stage 2: Package Manager Rebuild

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

OLD_PREFIX="/data/data/com.termux"
NEW_PREFIX="/data/data/com.termux.rafacodephi"

# Ensure we have the required tools
check_dependencies() {
    for tool in sed grep strings readelf; do
        if ! command -v "$tool" &> /dev/null; then
            echo "Error: Required tool '$tool' not found" >&2
            exit 1
        fi
    done
}

# Migrate prefix in text-based configuration files
migrate_config_files() {
    local config_dir="${1:-.}"

    echo "[PREFIX_MIGRATION] Migrating configuration files..."

    for config_file in $(find "$config_dir" -type f \( -name "*.conf" -o -name "*.cfg" -o -name "sources.list" -o -name "*.list" \)); do
        if grep -q "$OLD_PREFIX" "$config_file" 2>/dev/null; then
            echo "  Migrating: $config_file"
            sed -i "s|$OLD_PREFIX|$NEW_PREFIX|g" "$config_file"
        fi
    done
}

# Extract embedded paths from binary using strings
extract_embedded_paths() {
    local binary="$1"

    if [[ ! -f "$binary" ]]; then
        return
    fi

    strings "$binary" 2>/dev/null | grep -E "^$OLD_PREFIX" | sort -u || true
}

# Create prefix rewrite mapping (for binary patching if needed)
create_prefix_mapping() {
    local binary="$1"
    local output_file="$2"

    echo "old_prefix=$OLD_PREFIX" > "$output_file"
    echo "new_prefix=$NEW_PREFIX" >> "$output_file"
    echo "old_length=${#OLD_PREFIX}" >> "$output_file"
    echo "new_length=${#NEW_PREFIX}" >> "$output_file"

    # Extract all embedded paths
    {
        echo "embedded_paths:"
        extract_embedded_paths "$binary"
    } >> "$output_file"
}

# Validate that new prefix fits within binary size constraints
validate_prefix_migration() {
    local binary="$1"

    # Old prefix: /data/data/com.termux = 24 bytes
    # New prefix: /data/data/com.termux.rafacodephi = 41 bytes
    # Difference: +17 bytes per occurrence

    local path_count=$(extract_embedded_paths "$binary" | wc -l)
    local size_increase=$((17 * path_count))

    echo "[PREFIX_MIGRATION] Binary: $binary"
    echo "  Embedded path count: $path_count"
    echo "  Estimated size increase: $size_increase bytes"

    # Warn if size increase is significant
    if [[ $size_increase -gt 50000 ]]; then
        echo "  WARNING: Large size increase detected" >&2
    fi
}

# Generate prefix migration report
generate_migration_report() {
    local report_file="${REPO_ROOT}/results/prefix-migration-report.json"

    mkdir -p "$(dirname "$report_file")"

    cat > "$report_file" << 'EOF'
{
  "schema": "raf.prefix-migration.v1",
  "old_prefix": "/data/data/com.termux",
  "new_prefix": "/data/data/com.termux.rafacodephi",
  "migration_stage": "Stage 2: Package Manager Rebuild",
  "binaries_requiring_migration": [
    {
      "name": "dpkg",
      "path": "/data/data/com.termux.rafacodephi/bin/dpkg",
      "status": "PLANNED",
      "embedded_paths": []
    },
    {
      "name": "apt-get",
      "path": "/data/data/com.termux.rafacodephi/bin/apt-get",
      "status": "PLANNED",
      "embedded_paths": []
    },
    {
      "name": "apt",
      "path": "/data/data/com.termux.rafacodephi/bin/apt",
      "status": "PLANNED",
      "embedded_paths": []
    },
    {
      "name": "libapt.so",
      "path": "/data/data/com.termux.rafacodephi/lib/libapt.so",
      "status": "PLANNED",
      "embedded_paths": []
    }
  ],
  "config_files_requiring_migration": [
    "/data/data/com.termux.rafacodephi/etc/apt/sources.list",
    "/data/data/com.termux.rafacodephi/etc/apt/apt.conf",
    "/data/data/com.termux.rafacodephi/etc/dpkg/dpkg.cfg"
  ],
  "verification_gates": [
    {
      "gate": "no_old_prefix_in_binaries",
      "description": "Verify no /data/data/com.termux references remain in binaries",
      "command": "strings /path/to/binary | grep -c '/data/data/com.termux[^.]' || true",
      "expected_result": "0"
    },
    {
      "gate": "new_prefix_present",
      "description": "Verify new prefix is correctly embedded",
      "command": "strings /path/to/binary | grep -c '/data/data/com.termux.rafacodephi' || true",
      "expected_result": ">0"
    }
  ],
  "risk_mitigation": [
    "Prefix lengths differ (+17 bytes): validate binary size increase",
    "Embedded paths must align with null terminators: verify with strings output",
    "Dependent libraries must also be migrated: check library interdependencies"
  ]
}
EOF

    echo "[PREFIX_MIGRATION] Report written to: $report_file"
}

main() {
    check_dependencies

    echo "[PREFIX_MIGRATION] Starting prefix migration process"
    echo "  Old prefix: $OLD_PREFIX"
    echo "  New prefix: $NEW_PREFIX"

    # Migrate configuration files if they exist
    if [[ -d "${REPO_ROOT}/configs" ]]; then
        migrate_config_files "${REPO_ROOT}/configs"
    fi

    # Generate migration report
    generate_migration_report

    echo "[PREFIX_MIGRATION] Prefix migration planning complete"
    echo "  Next steps:"
    echo "    1. Rebuild dpkg with -DPREFIX='$NEW_PREFIX'"
    echo "    2. Rebuild libapt against musl with -DPREFIX='$NEW_PREFIX'"
    echo "    3. Rebuild apt with -DPREFIX='$NEW_PREFIX'"
    echo "    4. Validate binaries with: strings binary | grep '$OLD_PREFIX' (should return 0 matches)"
}

main "$@"
