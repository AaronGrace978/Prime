#!/usr/bin/env bash
# Build and package Prime for universal Linux x86_64 (Steam Deck / desktop).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="${PRIME_VERSION:-$(git -C "$ROOT" describe --tags --always 2>/dev/null | sed 's/^v//')}"
VERSION="${VERSION:-0.3.0}"
ARCH="x86_64"
STAGING="$ROOT/dist/Prime-${VERSION}-linux-${ARCH}"
TARBALL="$ROOT/dist/Prime-${VERSION}-linux-${ARCH}.tar.gz"

echo "◈ Prime ${VERSION} — linux ${ARCH} release build"
echo ""

# --- compile ---------------------------------------------------------------
BUILD="$ROOT/build-release"
# Prefer g++ for portable linux tarballs (static libstdc++ linking)
export CC="${CC:-gcc}"
export CXX="${CXX:-g++}"
cmake -S "$ROOT" -B "$BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=x86-64" \
  -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc"

cmake --build "$BUILD" -j"$(nproc)"

# smoke test
ctest --test-dir "$BUILD" --output-on-failure

# --- stage -----------------------------------------------------------------
rm -rf "$STAGING"
mkdir -p "$STAGING"/{bin,share/prime,share/applications,share/icons/hicolor/256x256/apps}

cp "$BUILD/tools/primegraph/primegraph" "$STAGING/bin/"
cp "$BUILD/tools/primeforge/primeforge" "$STAGING/bin/"
cp "$BUILD/tools/primetex/primetex"     "$STAGING/bin/"
cp "$BUILD/tools/primeplay/primeplay"   "$STAGING/bin/"
chmod +x "$STAGING/bin/"*

# studio + assets
mkdir -p "$STAGING/share/prime/studio"
cp -r "$ROOT/studio/server" "$ROOT/studio/web" "$STAGING/share/prime/studio/"
cp "$ROOT/studio/requirements.txt" "$STAGING/share/prime/studio/"
cp -r "$ROOT/assets" "$STAGING/share/prime/"

# launchers
cp "$ROOT/release/bin/prime-studio" "$STAGING/bin/"
cp "$ROOT/release/bin/prime-deck"   "$STAGING/bin/"
cp "$ROOT/release/install.sh"       "$STAGING/"
cp "$ROOT/release/README-STEAMDECK.md" "$STAGING/"
cp "$ROOT/release/Prime.desktop"    "$STAGING/share/applications/"
cp "$ROOT/release/prime.png"        "$STAGING/share/icons/hicolor/256x256/apps/" 2>/dev/null || true

# version file
cat > "$STAGING/VERSION" <<EOF
Prime ${VERSION}
linux-${ARCH}
built $(date -u +%Y-%m-%dT%H:%MZ)
EOF

# --- tarball ---------------------------------------------------------------
mkdir -p "$ROOT/dist"
tar -C "$ROOT/dist" -czf "$TARBALL" "Prime-${VERSION}-linux-${ARCH}"

echo ""
echo "✓ Package: $TARBALL"
echo "  Size: $(du -h "$TARBALL" | cut -f1)"
echo ""
echo "Steam Deck: extract anywhere, then run ./install.sh"
