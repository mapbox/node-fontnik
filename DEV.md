### Tagging and publishing binaries via Github Actions

On each commit that passes through GitHub actions workflow, the binaries are generated for `linux-x64` and `darwin-x64`. These binaries can be downloaded when publishing the npm package.

Running `npm publish` uses the binaries present in the `prebuilds` directory. When the module is installed with `npm install`, a pre-built binary in the `prebuilds` directory is used if there's one that's suitable for the OS and architecture of the machine. Otherwise, the binary is built from the source when installing.

Typical workflow:

```
git checkout master

# increment version number
# https://docs.npmjs.com/cli/version
npm version major | minor | patch

# amend commit to include "[publish binary]"
git commit --amend
"x.y.z" -> "x.y.z [publish binary]"

# push commit and tag to remote
git push
git push --tags

# make a sandwich, check travis console for build successes
# test published binary (should install from remote)
npm install && npm test

# Make sure that the GHA workflow is successfull. Download the artifacts
npm run download-binaries

# publish to npm
npm publish
```

> Note: gh CLI is required in order to download binaries from GH workflow runs. Follow the instruction in the [link](https://github.com/cli/cli#installation) to install the CLI. Run `gh auth login` and follow the instructions to authenticate before downloading binaries. 