# Building and publishing documentation

From the repository root, using Python 3.10 or newer:

```sh
python3 -m venv .venv-docs
.venv-docs/bin/pip install -r doc/requirements.txt
.venv-docs/bin/python doc/build.py
python3 -m http.server 8082 --directory build-docs
```

Open `/doc/` (English) or `/doc_ru/` (Russian). On Windows activate the virtual
environment and run `python doc/build.py` or `doc\deploy.cmd` for a local build.
Both languages build in strict mode. Use the matching source release, currently `v1.5.261116`; macOS and WASM are
part of `main`. The public website builds both documentation languages from
`main` through the workflow in `intent-garden/wui_site`.

Publish using an already configured AWS CLI profile/environment:

```sh
PYTHON_BIN=.venv-docs/bin/python bash doc/deploy.sh --dry-run
PYTHON_BIN=.venv-docs/bin/python bash doc/deploy.sh
```

Default target: Yandex Object Storage bucket `libwui.org`. Overrides:
`WUI_SITE_BUCKET`, `S3_ENDPOINT_URL`, `AWS_DEFAULT_REGION`, `AWS_BIN`, `PYTHON_BIN`.
No credentials are stored in the scripts. Both languages must build before upload.
Only `doc/` and `doc_ru/` are uploaded; no remote objects are deleted. HTML uses
UTF-8 and all uploads request cache revalidation. Key uploaded files are downloaded
and compared with the build. The website and WASM examples deploy independently.
