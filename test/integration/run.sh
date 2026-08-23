#!/usr/bin/env bash
#
# Runs the GUI integration tests from the repository root, whatever directory it is called from.
#
#   test/integration/run.sh                       # everything
#   test/integration/run.sh --filter 'tournament-*'
#   test/integration/run.sh --config release
#
# On Linux the tests need a display: the GUI really opens a window, which is the point -- the
# suite drives the same application a person would be watching. Without one, xvfb-run provides a
# virtual display, and that is what CI uses.

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
cd "${repo_root}"

runner=(python3 test/integration/test_runner.py "$@")

if [[ "$(uname -s)" == "Linux" && -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
	if command -v xvfb-run >/dev/null 2>&1; then
		echo "No display found -- running under xvfb-run."
		exec xvfb-run -a "${runner[@]}"
	fi
	echo "No display and no xvfb-run: the GUI cannot open a window." >&2
	echo "Install xvfb (apt install xvfb) or run from a desktop session." >&2
	exit 2
fi

exec "${runner[@]}"
