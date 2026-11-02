#!/bin/bash
# Builds the macOS installer: one .pkg that installs the VST3, the Audio Unit and the Standalone app.
#
#   packaging/macos/build-pkg.sh <artefacts-dir> <version> <out-dir>
#
# <artefacts-dir> is build/Zither_artefacts/Release. Written for the stock macOS bash 3.2.
set -euo pipefail

if [ $# -ne 3 ]; then
    echo "usage: $0 <artefacts-dir> <version> <out-dir>" >&2
    exit 2
fi

artefacts="$1"
version="$2"
out="$3"
here="$(cd "$(dirname "$0")" && pwd)"
product="Zither Aquatica"
identifier="com.github.kaveets24.zitheraquatica"

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# component <name> <bundle> <install location> [scripts dir]
component()
{
    local name="$1" bundle="$2" location="$3" scripts="${4:-}"
    local root="$work/$name-root"
    local staged="$root/$(basename "$bundle")"

    mkdir -p "$root"
    ditto "$bundle" "$staged"

    # Apple Silicon refuses to load code without a valid signature, so unsigned releases still carry an ad-hoc one,
    # sealed over the final bundle contents. Developer ID signing and notarization replace this line later.
    codesign --force --deep --sign - "$staged"

    # Every format shares one bundle ID, and by default Installer "relocates" a bundle to wherever it last saw that ID
    # (the Standalone app, or a dev build). Plugins must land exactly in the Plug-Ins folders, so turn that off.
    pkgbuild --analyze --root "$root" "$work/$name.plist" > /dev/null
    plutil -replace 0.BundleIsRelocatable -bool NO "$work/$name.plist"

    local args=(--root "$root" --component-plist "$work/$name.plist" --identifier "$identifier.$name"
                --version "$version" --install-location "$location")
    if [ -n "$scripts" ]; then
        args+=(--scripts "$scripts")
    fi
    pkgbuild "${args[@]}" "$work/packages/$name.pkg" > /dev/null
}

mkdir -p "$work/packages" "$work/resources"
cp "$here/../../LICENSE" "$work/resources/LICENSE.txt"

component vst3 "$artefacts/VST3/$product.vst3"      /Library/Audio/Plug-Ins/VST3
component au   "$artefacts/AU/$product.component"   /Library/Audio/Plug-Ins/Components "$here/scripts/au"
component app  "$artefacts/Standalone/$product.app" /Applications

mkdir -p "$out"
installer="$out/$product $version.pkg"
productbuild --distribution "$here/distribution.xml" --package-path "$work/packages" \
    --resources "$work/resources" "$installer" > /dev/null
echo "$installer"
