# Work with fork

## Delete every local changes and reset to upstream/master

```
git checkout master
git pull upstream master
git reset --hard upstream/master
git push origin master --force
```

## Create pull_request for pull request
```
git checkout master
git checkout -b pull_request
```

## Squash merge for pull request

```
git checkout pull_request
git pull upstream master
git reset --hard upstream/master
git merge --squash --no-commit WORKING_BRANCH
git status
git commit -am "PR TITLE"
git pull origin pull_request
```
