#!/usr/bin/env bash
TOOLS=$(cd "$(dirname "$0")" && pwd)
WORK=${CEV_WORK:-/tmp/c-evidence-work}
REPO=${CEV_REPO:-$(dirname "$(git -C "$TOOLS" rev-parse --path-format=absolute --git-common-dir)")}
BUILD=${CEV_BUILD:-$WORK/build}
DEVSRC=${CEV_DEVSRC:-$WORK/dev-source}
# usage: devbuild.sh slug Observer  (adds observer to dev CMakeLists once, builds)
set -e
slug=$1; obs=$2; src=$DEVSRC
if ! grep -q "add_executable($obs " $src/CMakeLists.txt; then
cat >> $src/CMakeLists.txt <<EOT
add_executable($obs "$WORK/$slug/scenario.c" \$<TARGET_OBJECTS:OurCoreLib> \$<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories($obs PRIVATE src src/tests)
target_compile_definitions($obs PRIVATE ALLOW_BORG)
target_link_libraries($obs PRIVATE \${ANGBAND_CORE_LINK_LIBRARIES})
EOT
fi
# re-configure so a newly added observer target exists (otherwise the build is a silent no-op)
cmake -S $src -B $BUILD >/dev/null
out=$(flock /tmp/angband-validation.lock cmake --build $BUILD --parallel 4 --target $obs 2>&1) || { echo "$out" | tail -40; exit 1; }
echo "$out" | grep -E "error|warning: unused|Error" | head -30 || true
[ -x "$BUILD/$obs" ] || [ -x "$(find $BUILD -maxdepth 2 -name $obs -type f | head -1)" ] || { echo "devbuild: observer $obs was not built" >&2; exit 1; }
