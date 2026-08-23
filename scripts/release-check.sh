#!/usr/bin/env bash
#
# The check that goes in front of a release: build, then all three test layers, in the order
# that finds the cheapest failures first. The first stage that fails ends the run.
#
#   scripts/release-check.sh                  # debug build
#   scripts/release-check.sh --config release
#   scripts/release-check.sh --skip-gui       # leave out the on-screen GUI suite
#
# Every stage that starts the GUI does so with a configuration directory of its own, so a release
# check never touches, and never depends on, the configuration you work with.

set -uo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

config="default"
skip_gui=0

while [[ $# -gt 0 ]]; do
	case "$1" in
		--config) config="$2"; shift 2 ;;
		--skip-gui) skip_gui=1; shift ;;
		*) echo "Unknown option: $1" >&2; exit 2 ;;
	esac
done

scratch="$(mktemp -d)"
trap 'rm -rf "${scratch}"' EXIT

stage=0
failed=""

run_stage() {
	local title="$1"; shift
	stage=$((stage + 1))
	echo
	echo "=============================================="
	echo "  ${stage}. ${title}"
	echo "=============================================="
	if "$@"; then
		echo "-- ${title}: ok"
		return 0
	fi
	echo "-- ${title}: FAILED" >&2
	failed="${title}"
	return 1
}

run_stage "Build (${config})" cmake --build --preset "${config}" || { echo; echo "Release check failed at: ${failed}" >&2; exit 1; }

run_stage "Unit tests" "./build/${config}/unit-tests" || { echo; echo "Release check failed at: ${failed}" >&2; exit 1; }

run_stage "Integration tests (HTTP)" test/integration/run.sh --config "${config}" \
	|| { echo; echo "Release check failed at: ${failed}" >&2; exit 1; }

if [[ "${skip_gui}" == "0" ]]; then
	# The on-screen suite last: it is the slowest and, today, the least steady of the three -- a
	# full run does not always report the same number of tests. Treated as a gate all the same,
	# with --skip-gui as the deliberate way past it rather than a quiet exclusion.
	run_stage "GUI tests (ImGui Test Engine)" env \
		QAPLA_AUTO_RUN_TESTS=1 "./build/${config}/qapla" --config-dir="${scratch}/gui-tests" \
		|| { echo; echo "Release check failed at: ${failed}" >&2; exit 1; }
fi

echo
echo "=============================================="
echo "  All stages passed. Ready to release."
echo "=============================================="
