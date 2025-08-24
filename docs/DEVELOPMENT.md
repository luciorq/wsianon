# `wsianon` Development Notes

This file is used to track the initial experimentation with the build tools.

## Building Python Package

New module code written in `src/wsianon` directory.

### Syncing C library from remote

```bash
# TODO: @luciorq `macos-support` has already been merged in
# + the main (master) branch.
# Initial clone: `git clone -b macos-support https://github.com/luciorq/wsi-anon ./temp/`;

git clone -b macos-support https://github.com/luciorq/wsi-anon ./temp/

rm -rf ./include/*;
cp -r temp/src/* ./include/;
cp temp/LICENSE ./include/;
rm -rf ./temp;
```

### Test data

Test data is acquired from the [Openslide project](https://openslide.cs.cmu.edu/download/openslide-testdata/).

---

<!--

## First try using Hatch

> NOTE: Not working properly, since you still needs to compile the C library
using `setuptools`. Development will move to setuptools entirely.

Using [Hatch](https://hatch.pypa.io/latest/) for Package Management.

### Create hatch env

```bash
conda create -n hatch-env -y hatch;
alias hatch='conda run -n hatch-env hatch';
hatch new --init;
```

### Create Python development env with conda

Test environment: `pytest`, `openslide`, and `tiffslide` are using for testing,
they are not required for building or using the package.

```bash
# Test environment
conda create -n wsianon-env -y python pytest tiffslide openslide openslide-python;
# TODO: @luciorq Not working as expected
conda activate wsianon-env;
```

### Copy source code from Upstream

Using personal fork with modification.

```bash
git clone -b macos-support https://github.com/luciorq/wsi-anon ./temp/;
\mkdir -p ./src/wsianon;
\cp -r ./temp/wrapper/python/* ./src/wsianon/;
\mkdir -p ./include/;
# \mkdir -p ./lib/;
cp -r temp/src/* ./include/;
```

```bash
# Setting package version
touch ./src/wsianon/__about__.py
version_string="$(\cat temp/CHANGELOG.md | \grep '^##' | \head -1 | \sed 's|.* ||g')";
\builtin echo -ne "__version__ = \"${version_string}\"\n" > ./src/wsianon/__about__.py
# Check if hatch is able to parse version
hatch version; # "${version_string}";
```

#### Compile C library

```bash
# TODO: @luciorq temporary testing
# (cd temp/ && make shared-lib) && cp temp/bin/libwsianon* ./;
```

```bash
\cp -r
```

#### Check if module works

```bash
conda run -n wsianon-env python -m pip install -e .;

conda run -n wsianon-env python -c 'from wsianon import wsianon';
```

#### Testing with real files

```bash
conda run -n wsianon-env python -c 'from wsianon import wsianon; wsianon.anonymize_wsi("../wsi-tools/data/input/CMU-1-Small-Region.svs", new_label_name = "wsianon_test")';

conda run -n wsianon-env \
  python -c \
  'from wsianon import wsianon; wsianon.get_wsi_data("../wsi-tools/data/input/CMU-1-Small-Region.svs")';
```

Testing with python

```python
import openslide
import tiffslide

from wsianon import wsianon

wsi_path = [
  "../wsi-tools/data/input/CMU-1-Small-Region.svs",
  "../wsi-tools/data/input/wsianon_test.svs"
]

wsianon.anonymize_wsi(wsi_path[0], new_label_name = "wsianon_test")

slide = tiffslide.TiffSlide(wsi_path[0])
associated_images = slide.associated_images

slide = tiffslide.TiffSlide(wsi_path[1])
associated_images = slide.associated_images

```

#### Clean project

```bash
\rm -rf ./temp;
\rm -rf  ../wsi-tools/data/input/wsianon_test*;
```

---

## Using setuptools python package

```bash
# TODO: @luciorq Deprecation note on this method
# + See <https://blog.ganssle.io/articles/2021/10/setup-py-deprecated.html> for details.
conda run -n wsianon-env python setup.py build;
conda run -n wsianon-env python setup.py install;
```

-->

---

## Adding features to Upstream project on GitLab

Adding suport for MacOS in the Upstream [EMPAIA WSI Anon](https://gitlab.com/empaia/integration/wsi-anon) project on GitLab.

Fork to <https://gitlab.com/luciorq/wsi-anon>

```bash
git clone https://gitlab.com/luciorq/wsi-anon ~/projects/forks/wsi-anon;
cd ~/projects/forks/wsi-anon;
git branch;
git checkout -b <BRANCH_NAME>;
```

Make necessary edits to the branch.

Check if everything is working with `make`.

```bash
make
```

Commit and push.

```bash
git add -p <FILES_TO_ADD>
git commit -m "...";

git push origin <BRANCH_NAME>
```

This will push the changes to your personal fork,
you still need to create the Pull Request in the original repository.

## Testing WSI Anon Python Wrapper

```bash
conda create -n wsi-anon-env -c conda-forge -c bioconda openslide python pytest tiffslide openslide-python


cp bin/libwsianon.dylib ./
conda run -n wsi-anon-env python -m pytest ./wrapper/python/test
```

## Add additional remote on GitHub

```bash
git remote -v;
git remote add origin-gh https://github.com/luciorq/wsi-anon;
git remote -v;
git push origin-gh --all;

gh repo view --web;
```

---

## Install docs and test tools

Trying pixi with pip for installing deps;

```bash
pixi add python pip;
pixi run python -m pip install -e ".[examples,docs,tests]";
pixi run python -m pytest -vvv;
```

After accepting Pull Request

```bash
git remote prune --dry-run origin
git remote prune origin
alias git-list-untracked='git fetch --prune && git branch -r | awk "{print \$1}" | grep -E -v -f /dev/fd/0 <(git branch -vv | grep origin) | awk "{print \$1}"'
git-list-untracked
alias git-remove-untracked='git fetch --prune && git branch -r | awk "{print \$1}" | grep -E -v -f /dev/fd/0 <(git branch -vv | grep origin) | awk "{print \$1}" | xargs git branch -d'
git-remove-untracked
```

### Fix tests using openslide and tiffslide

```bash
pixi add ruff;
pixi run python -m pip install -e ".[examples,docs,tests]";
pixi run ruff check;
pixi run python -m pytest -vvv;
```

---

## Updating After Merge and Update of Upstream

Working again on this project for the URSSI 2025 Summer School.

Date: 2025-08-18.

```bash
# cd ~/projects/wsianon

git -C ~/projects/forks/wsi-anon/ status
git -C ~/projects/forks/wsi-anon/ remote -v
# origin  https://gitlab.com/luciorq/wsi-anon
# origin-gh       https://github.com/luciorq/wsi-anon
# upstream        https://gitlab.com/empaia/integration/wsi-anon

# Go to <https://gitlab.com/luciorq/wsi-anon> and make sure to sync fork
git -C ~/projects/forks/wsi-anon/ fetch
git -C ~/projects/forks/wsi-anon/ branch

git -C ~/projects/forks/wsi-anon/ pull

git -C ~/projects/forks/wsi-anon/ log \
  --graph \
  --pretty=format:"%C(magenta)%h%Creset -%C(red)%d%Creset %s %C(dim green)(%cr) %C(cyan)<%an>%Creset" \
  --abbrev-commit;

# Prepare build project
ls -lah ./include;
rm -rf ./include/*;
ls -lah ~/projects/forks/wsi-anon/src/;
cp -r ~/projects/forks/wsi-anon/src/* ./include/;
ls -lah ~/projects/forks/wsi-anon/;
cp ~/projects/forks/wsi-anon/LICENSE ./include/;
rm -rf ./temp;
```

```bash
# pixi run python -m pip install -e ".[examples,docs,tests]";
# pixi run ruff check;
# pixi run python -m pytest -vvv;
```

```bash
pixi init;
pixi add python pip;
pixi run python -m pip install --group=all -e ".[cli]";
```

Using `uv`

```bash
# uv pip install --extra cli .;
uv pip install --group=all -e ".[cli]";
uv tool run ruff check;
uv tool run rumdl check;
uv tool run pytest;
```
