### Tagging and publishing binaries via Travis

On each commit that passes through Travis, the [`travis_publish.sh`](https://github.com/mapbox/node-fontnik/blob/master/deps/travis_publish.sh) script checks for the string `[publish binary]` in the commit message, and if present, runs `node-pre-gyp publish` to publish a binary.

Running `npm publish` doesn't actually upload a binary anywhere, but instead just pushes up your local code with an updated version number in `package.json`. When your module is installed with `npm install`, `node-pre-gyp` will use this version number to search for a published binary, falling back to a source compile if no matching binary is found.

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

# publish to npm
npm publish
```
