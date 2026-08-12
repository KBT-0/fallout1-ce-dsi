# NETWORK_ENVIRONMENT — Run 1

## Result

Shell/container outbound networking is blocked at DNS resolution level. It is **not Git-specific**.

The web research channel was able to inspect public GitHub sources, but ordinary shell processes could not resolve external hosts. Therefore source audit could continue through public web retrieval, while local `git clone` and package/toolchain acquisition remained blocked.

## Commands executed

```bash
set -o pipefail
{
  echo '### git clone test'
  git clone --depth 1 https://github.com/alexbatalov/fallout1-ce.git /tmp/f1ce-nettest 2>&1 || true

  echo '### curl github test'
  curl -I --max-time 10 https://github.com 2>&1 || true

  echo '### curl raw.githubusercontent.com test'
  curl -I --max-time 10 https://raw.githubusercontent.com 2>&1 || true

  echo '### curl example.com test'
  curl -I --max-time 10 https://example.com 2>&1 || true
}
```

## Exact relevant output

```text
### git clone test
Cloning into '/tmp/f1ce-nettest'...
fatal: unable to access 'https://github.com/alexbatalov/fallout1-ce.git/': Could not resolve host: github.com

### curl github test
curl: (6) Could not resolve host: github.com

### curl raw.githubusercontent.com test
curl: (6) Could not resolve host: raw.githubusercontent.com

### curl example.com test
curl: (6) Could not resolve host: example.com
```

This establishes that the restriction is general outbound DNS/network access in the shell environment, not a Git-only failure.

## Tool availability

```text
git version 2.47.3
arm-none-eabi-g++: NOT_FOUND
arm-none-eabi-size: NOT_FOUND
melonDS: NOT_FOUND
DEVKITPRO=UNSET
DEVKITARM=UNSET
BLOCKSDS=UNSET
```

## Consequence

- Source/code audit: partially unblocked via public web retrieval.
- Local source-wide grep: still unavailable here.
- ARM9 compile/link/size: blocked locally.
- melonDS preflight: blocked locally.
- CI is the preferred unblock path.
