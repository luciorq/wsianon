#!/usr/bin/env just
# shellcheck shell=bash

package_name := 'wsianon'

# github_org := 'luciorq'

@default:
  just --choose

@install:
  #!/usr/bin/env -vS bash -i
  conda run -n wsianon-env python -m pip install .;

@remove: clean
  #!/usr/bin/env -vS bash -i
  conda run -n wsianon-env python -m pip uninstall -y wsianon;

@clean:
  #!/usr/bin/env -vS bash -i
  \builtin set -euxo pipefail;
  \rm -rf ./build/;
  \rm -rf ./src/{{ package_name }}.egg-info;

#@echo:
#  #!/usr/bin/env -vS bash -i
#  \builtin set -euxo pipefail;
#  \builtin echo -ne "Hello, {{ arch() }} - {{ justfile() }}.\\n";