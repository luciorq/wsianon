#!/usr/bin/env just
# shellcheck shell=bash

package_name := 'wsianon'

@default:
  just --choose

@install:
  #!/usr/bin/env bash
  \builtin set -euxo pipefail;
  pixi install;
  pixi run python -m pip install -e "{{ justfile_directory() }}[examples,docs,tests]";

@remove: clean
  #!/usr/bin/env bash
  \builtin set -euxo pipefail;
  pixi run python -m pip uninstall -y wsianon;

@clean:
  #!/usr/bin/env bash
  \builtin set -euxo pipefail;
  \rm -rf ./build/;
  \rm -rf ./src/{{ package_name }}.egg-info;
  \rm -rf ./src/{{ package_name }}/__pycache__;
  \rm -f ./src/{{ package_name }}/lib{{ package_name }}*.so;

@test:
  #!/usr/bin/env bash
  \builtin set -euxo pipefail;
  pixi run python -m wsianon --help;
  pixi run wsianon --help;
  pixi run ruff check;
  pixi run python -m pytest -vvv;
