### Publishing releases

CI builds native binaries for three platforms on every push to `main` and on pull requests:

- `darwin-arm64`
- `linux-x64`
- `linux-arm64`

Binaries are written to `prebuilds/<platform>-<arch>/fontnik.node` by CMake (Release builds) and uploaded as workflow artifacts.

Requires Node.js 20 or later (`engines` in `package.json`).

### Release workflow

Publishing uses npm [OIDC trusted publishing](https://mapbox.atlassian.net/wiki/spaces/CLOUDPLAT/pages/2894397444/PSA+setting+up+npm+release+workflow+for+public+packages) via [`.github/workflows/npm-release.yml`](.github/workflows/npm-release.yml). No npm token is required.

1. Bump the version in `package.json` and update `CHANGELOG.md`
2. Open a PR, get it reviewed, and merge to `main`
3. Go to **Actions → NPM release → Run workflow**
4. Approve the `npm-release` environment gate when prompted

The workflow builds and tests on all three platform runners, verifies the packed tarball, publishes to npm, and creates a GitHub release with auto-generated notes.

To test the pipeline without publishing, run the workflow with **dry-run** enabled.

Publishing happens only in CI; there is no local publish path. `prepublishOnly` runs `check-binaries` so an accidental local `npm publish` fails unless all three prebuilds are present.

The repo must have a GitHub Environment named `npm-release` configured before the first release (see the PSA linked above).

### Local development

```
brew install cmake ninja boost zlib   # macOS
npm ci --ignore-scripts
npm run rebuild
npm test
```

When a matching prebuild exists in `prebuilds/`, `npm install` skips compilation. Otherwise `scripts/install-native.js` runs `npm run rebuild` (unless `CI=true`).
