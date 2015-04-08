### Making a release + publishing binaries via Travis

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
