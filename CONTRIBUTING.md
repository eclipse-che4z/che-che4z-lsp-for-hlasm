# Contributing to the Eclipse Che4z LSP for HLASM project

Thanks for your interest in this project.

## Eclipse Che4z

This project is a part of [Eclipse Che4z](https://projects.eclipse.org/projects/ecd.che.che4z) which provides components/extensions for Eclipse Che to facilitate mainframe application development.

## Eclipse Contributor Agreement

Before your contribution can be accepted by the project team, you must electronically sign the Eclipse Contributor Agreement (ECA).

* http://www.eclipse.org/legal/ECA.php

Commits that are provided by non-committers must have a Signed-off-by field in the footer indicating that the author is aware of the terms by which the contribution has been provided to the project. The non-committer must additionally have an Eclipse Foundation account and must have a signed Eclipse Contributor Agreement (ECA) on file.

Make sure that the user email you specified on your local git is the same as on the Github account.

For more information, please see the Eclipse Committer Handbook:
https://www.eclipse.org/projects/handbook/#resources-commit

The signoff is easily achieved using the `--signoff` option of the `git commit` command (provided that the git credentials are properly configured):
```
git commit --signoff
```

## Setting up the Development Environment

The project uses CMake targeting all major platforms (Windows, Linux, MacOS). The full build instructions including project prerequisities can be found in the project [wiki](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/Build-instructions).

## Start contributing

Before contributing to the project, fork the repository and clone it, or add it as a new remote if you have already cloned the original repository. All the commits should be pushed to the fork in order to open pull requests from it.

We encourage every potential contributor to read the project documetation available on the project [wiki](https://github.com/eclipse/che-che4z-lsp-for-hlasm/wiki/). It describes the architecture of the project, decomposes it into software components and explains their relationships.

All the activity on the project should begin with defining a new issue that describes the required changes.

## Contribution acceptance

Each pull request is automatically verified in the following environments (in a GitHub Actions pipeline):

- Windows build with the MSVC compiler
- Ubuntu 18.04 build with the GNU GCC 8 compiler using the libstdc++ standard library
- Ubuntu 18.04 build with the Clang 8 compiler using the libc++ standard library and address sanitizer
- Alpine Linux 3.10 build with GNU GCC 8 compiler
- MacOS 10.15 build with LLVM CLang 8 (not Apple Clang) using the libc++ standard

Further, all C++ sources must be formatted with clang-format. The required configuration is available in the [.clang-format](https://github.com/eclipse/che-che4z-lsp-for-hlasm/blob/development/.clang-format) file in the root of the repository.

The project is also scanned on [Sonarcloud](https://sonarcloud.io/dashboard?id=eclipse_che-che4z-lsp-for-hlasm). However, because of the current architecture of GitHub Actions and Sonarcloud (see e.g. this [issue](https://jira.sonarsource.com/browse/MMF-1371)), the scan cannot be run from forked repositories automatically. Until a fix is found, it will be scanned manually by a member of the project team.

## Contact

Contact the project developers on [slack](https://join.slack.com/t/che4z/shared_invite/enQtNzk0MzA4NDMzOTIwLWIzMjEwMjJlOGMxNmMyNzQ1NWZlMzkxNmQ3M2VkYWNjMmE0MGQ0MjIyZmY3MTdhZThkZDg3NGNhY2FmZTEwNzQ).
