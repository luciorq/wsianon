#!/usr/bin/env just
# shellcheck shell=bash

package_name := 'wsianon'

# github_org := 'luciorq'

@default:
  just --choose

@install:
  #!/usr/bin/env -vS bash -i
  \builtin set -euxo pipefail;
  conda create -n wsianon-env -y --override-channels -c conda-forge python pytest;
  conda run -n wsianon-env python -m pip install -e "{{ justfile_directory() }}";

@remove: clean
  #!/usr/bin/env -vS bash -i
  \builtin set -euxo pipefail;
  conda run -n wsianon-env python -m pip uninstall -y wsianon;

@clean:
  #!/usr/bin/env -vS bash -i
  \builtin set -euxo pipefail;
  \rm -rf ./build/;
  \rm -rf ./src/{{ package_name }}.egg-info;
  \rm -rf ./src/{{ package_name }}/__pycache__;
  \rm -f ./src/{{ package_name }}/lib{{ package_name }}*.so;

@test-apps:
  #!/usr/bin/env -vS bash -i
  \builtin set -euxo pipefail;
  conda run -n wsianon-env python -m pytest "{{ justfile_directory() }}"/tests/*.py;