# Contribution Guidelines

This document contains guidelines for contributing code to libecoli. It has to
be followed in order for your patches to be approved and applied.

## Patch the code

Anyone can contribute to libecoli. First you need to clone the repository and
build the project:

```sh
git clone https://github.com/rjarry/libecoli
cd libecoli
make
```

Patch the code. Write some tests. Ensure that your code is properly formatted
with `clang-format`. Ensure that everything builds and works as expected. Ensure
that you did not break anything.

- If applicable, update unit tests.
- If adding a new feature, please consider adding new tests.
- Do not forget to update the docs, if applicable.
- Run the linters using `make lint`.
- Run unit tests using `make tests`.

Once you are happy with your work, you can create a commit (or several
commits). Follow these general rules:

- Limit the first line (title) of the commit message to 60 characters.
- Use a short prefix for the commit title for readability with `git log
  --oneline`. Do not use the `fix:` nor `feature:` prefixes. See recent commits
  for inspiration.
- Only use lower case letters for the commit title except when quoting symbols
  or known acronyms.
- Use the body of the commit message to actually explain what your patch does
  and why it is useful. Even if your patch is a one line fix, the description
  is not limited in length and may span over multiple paragraphs. Use proper
  English syntax, grammar and punctuation.
- Address only one issue/topic per commit.
- Describe your changes in the imperative mood, e.g. *"make xyzzy do frotz"*
  instead of *"[This patch] makes xyzzy do frotz"* or *"[I] changed xyzzy to do
  frotz"*, as if you are giving orders to the codebase to change its behaviour.
- If you are fixing a GitHub issue, add an appropriate `Closes: <ISSUE_URL>`
  trailer.
- If you are fixing a regression introduced by another commit, add a `Fixes:
  <sha> ("<title>")` trailer.
- When in doubt, follow the format and layout of the recent existing commits.
- The following trailers are accepted in commits. If you are using multiple
  trailers in a commit, it's preferred to also order them according to this
  list. Note, that the `commit-msg` hook (see below for installing) will
  automatically sort them for you when committing.

  * `Closes: <URL>` closes the referenced issue.
  * `Fixes: <sha> ("<title>")` reference the commit that introduced a regression.
  * `Link:`
  * `Cc:`
  * `Suggested-by:`
  * `Requested-by:`
  * `Reported-by:`
  * `Co-authored-by:`
  * `Assisted-by:` used to acknowledge AI assistants or automated tools that
    helped with the implementation.
  * `Signed-off-by:` compulsory!
  * `Tested-by:` used in review _after_ submission to the mailing list. If
    minimal changes occur between respins, feel free to include that into your
    respin to keep track of previous reviews.
  * `Reviewed-by:` used in review _after_ submission. If minimal changes occur
    between respins, feel free to include that into your respin to keep track
    of previous reviews.
  * `Acked-by:` used in review _after_ submission.

There is a great reference for commit messages in the
[Linux kernel documentation](https://www.kernel.org/doc/html/latest/process/submitting-patches.html#describe-your-changes).

IMPORTANT: you must sign-off your work using `git commit --signoff`. Follow the
[Linux kernel developer's certificate of origin][linux-signoff] for more
details. All contributions are made under the
[BSD-3-Clause](https://spdx.org/licenses/BSD-3-Clause.html) license. Please use
your real name and not a pseudonym. Here is an example:

    Signed-off-by: Robin Jarry <rjarry@redhat.com>

Once you are happy with your commits, you can verify that they are correct with
the following command:

```console
$ make check-commits
ok    [commit 1/2] 'worker: allow starting threads outside of current process affinity'
ok    [commit 2/2] 'main: allow creating ports when only a single cpu is available'
2/2 valid patches
```

## Create a pull request

[Fork the project](https://github.com/rjarry/libecoli/fork) if you haven't
already done so. Configure your clone to point at your fork and keep a reference
on the upstream repository. You can also take the opportunity to configure git
to use SSH for pushing and https:// for pulling.

```console
$ git remote remove origin
$ git remote add upstream https://github.com/rjarry/libecoli
$ git remote add origin https://github.com/foobar/libecoli
$ git fetch --all
Fetching origin
From https://github.com/foobar/libecoli
 * [new branch]                main       -> origin/main
Fetching upstream
From https://github.com/rjarry/libecoli
 * [new branch]                main       -> upstream/main
$ git config url.git@github.com:.pushinsteadof https://github.com/
```

Create a local branch named after the topic of your patch(es) and push it on
your fork.

```console
$ git checkout -b completion
$ git push origin completion
Enumerating objects: 11, done.
Counting objects: 100% (11/11), done.
Delta compression using up to 8 threads
Compressing objects: 100% (6/6), done.
Writing objects: 100% (6/6), 771 bytes | 771.00 KiB/s, done.
Total 6 (delta 5), reused 0 (delta 0), pack-reused 0 (from 0)
remote: Resolving deltas: 100% (5/5), completed with 5 local objects.
remote:
remote: Create a pull request for 'completion' on GitHub by visiting:
remote:      https://github.com/foobar/libecoli/pull/new/completion
remote:
To github.com:foobar/libecoli
 * [new branch]                completion -> completion
```

Before your pull request can be applied, it needs to be reviewed and approved
by others. They will indicate their approval by replying to your patch with
a [Tested-by, Reviewed-by or Acked-by][linux-review] (see also: [the git
wiki][git-trailers]) trailer. For example:

    Acked-by: Robin Jarry <rjarry@redhat.com>

There is no "chain of command" in libecoli. Anyone that feels comfortable enough
to "ack" or "review" a patch should express their opinion freely with an
official `Acked-by` or `Reviewed-by` trailer. If you only tested that a patch
works as expected but did not conduct a proper code review, you can indicate it
with a `Tested-by` trailer.

You can follow the review process on GitHub [web
UI](https://github.com/rjarry/libecoli/pulls).

Wait for feedback. Address comments and amend your original commit(s).
Then you should push to refresh your branch which will update the pull request:

```console
$ git commit --amend
$ git rebase -i upstream/main
$ git push --force origin completion
Enumerating objects: 23, done.
Counting objects: 100% (23/23), done.
Delta compression using up to 8 threads
Compressing objects: 100% (9/9), done.
Writing objects: 100% (12/12), 1.04 KiB | 1.04 MiB/s, done.
Total 12 (delta 7), reused 0 (delta 0), pack-reused 0 (from 0)
remote: Resolving deltas: 100% (7/7), completed with 7 local objects.
To github.com:foobar/libecoli
 + 0bde1bea3491...e66d7e8ef74e completion -> completion (forced update)
```

Be polite, patient and address *all* of the reviewers' remarks. If you disagree
with something, feel free to discuss it.

Once your patch has been reviewed and approved (and if the maintainer is OK
with it), the pull request will be applied.

## Code Style

Please refer only to the quoted sections when guidelines are sourced from
outside documents as some rules of the source material may conflict with other
rules set out in this document.

When updating an existing file, respect the existing coding style unless there
is a good reason not to do so.

### C code

All C code follows the rules specified in the [`.clang-format`](/.clang-format)
file. The code can be automatically formatted by running `make format`.

If `make lint` accepts your code, it is most likely properly formatted.

### Commenting

Use standard C block comments:

```c

/*
 * This is a block comment.
 * Please use this format instead of C99 line comments.
 */
int foo(void);
```

Do NOT use C99 line comments:

```c
// This is a C99 line comment.
// Please use block comments instead.
int foo(void);
```

### Editor modelines

> Some editors can interpret configuration information embedded in source
> files, indicated with special markers. For example, emacs interprets lines
> marked like this:
>
>     -*- mode: c -*-
>
> Or like this:
>
>     /*
>     Local Variables:
>     compile-command: "gcc -DMAGIC_DEBUG_FLAG foo.c"
>     End:
>     */
>
> Vim interprets markers that look like this:
>
>     /* vim:set sw=8 noet */
>
> Do not include any of these in source files. People have their own personal
> editor configurations, and your source files should not override them. This
> includes markers for indentation and mode configuration. People may use
> their own custom mode, or may have some other magic method for making
> indentation work correctly.
> — [Linux kernel coding style][linux-coding-style]

In the same way, files specific to only your workflow (for example the `.vscode`
directory) are not desired. If a script might be useful to other contributors,
it can be sent as a separate patch that adds it to the `devtools` directory.
Since it is not editor-specific, an [`.editorconfig`](/.editorconfig) file is
available in the repository.

[git-send-email-tutorial]: https://git-send-email.io/
[git-trailers]: https://git.wiki.kernel.org/index.php/CommitMessageConventions
[linux-coding-style]: https://www.kernel.org/doc/html/v5.19-rc8/process/coding-style.html
[linux-review]: https://www.kernel.org/doc/html/latest/process/submitting-patches.html#using-reported-by-tested-by-reviewed-by-suggested-by-and-fixes
[linux-signoff]: https://www.kernel.org/doc/html/latest/process/submitting-patches.html#sign-your-work-the-developer-s-certificate-of-origin
