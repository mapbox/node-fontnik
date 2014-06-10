### Making a release + publishing binaries via travis

Typical flow:

    git checkout master

    # increment version number
    vim package.json

    git commit package.json -m "0.m.n"
    git tag v0.m.n
    git push
    git push --tags

    # empty commit with '[publish binary]' in message
    # tells travis to create a new binary for version in package.json
    git commit --allow-empty -m "linux [publish binary]"
    git push origin master

    # switch to travis-osx branch which can build on OS X
    # and do the same
    git checkout travis-osx
    git merge master
    git commit --allow-empty -m "darwin [publish binary]"
    git push origin travis-osx

    # make a sandwich, check travis console for build successes
    npm publish
