#!/usr/bin/env bash
set -euo pipefail

# Build and run the C++ unit-test suite (src/test, Google Test) in a
# container. This is deliberately not part of the normal compose build; run
# this script when you want the tests.
#
# It builds the `unit-tests` stage of apps/docker/Dockerfile, which
# reconfigures with -DBUILD_TESTING=ON, recompiles (backed by the shared
# ccache volume), and runs the suite as the final build step — so a
# successful image build means a green test run.
#
# Note: container layer caching applies. If nothing changed since the last
# run, the test step is a cache hit and reports success without re-executing.
#
# Uses podman if available, docker otherwise; set CONTAINER_ENGINE to force.

cd "$(dirname "$0")/../.."

engine="${CONTAINER_ENGINE:-}"
if [ -z "$engine" ]; then
  if command -v podman >/dev/null 2>&1; then
    engine=podman
  else
    engine=docker
  fi
fi

exec "$engine" build --target unit-tests -f apps/docker/Dockerfile .
