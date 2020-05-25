# Work with fork

## Delete every local changes and reset to upstream/master

```
git checkout master
git pull upstream master
git reset --hard upstream/master
git push origin master --force
```
