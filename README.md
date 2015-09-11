Experimental fork
=================

This is an experimental fork of Taskwarrior. It indends to offer underground
solutions, stuff which was not approved or even submitted upstream. It also
serves as a sandbox for these features to be tested, refined and narrowed.

Features added might not be documented or tested at the quality of the
Taskwarrior project.

Pushing policy
--------------

To ensure that patches can be still easily submitted upstream, this git
repository will not be a good citizen. To keep the the extra changes on
top of current development branch from upstream, this branch will be
regularly rebased. This means you'll need update your copies with

    git pull --force

Extra features
--------------

### Extended support for external scripts via 'task execute'

The implementation of the 'execute' command has been altered to pass two
environment variables to the spawned process:

* $TASK_FILTER - contains command line part which represents a filter
* $TASK_ARGS   - contains command line part which represents non-filtered arguments

This allows you to create neat aliases in the taskrc file:

    # Simple example, with this you can use 'task 4 open' instead of having to do 'task open 4'
    alias.open=execute taskopen $TASK_FILTER

    # More complex example. Note ':' at the end, which eats the arguments which
    # are passed to this command, since they are already passed to the command via
    # $TASK_ARGS
    alias.append=execute for TASK_ID in `task $TASK_FILTER _ids`; do task $TASK_ID mod `task _get $TASK_ID.description` $TASK_ARGS; done; :


Parts of original Taskwarrior readme
------------------------------------

Thank you for taking a look at Taskwarrior!

Taskwarrior is a GTD, todo list, task management, command line utility with a
multitude of features. It is a portable, well supported and very active Open
Source project. Taskwarrior has binary distributions, online documentation,
demonstration movies, and you'll find all the details at:

    http://taskwarrior.org

At the site you'll find online documentation, downloads, news and more.

Your contributions are especially welcome. Whether it comes in the form of
code patches, ideas, discussion, bug reports, encouragement or criticism, your
input is needed.

For support options, take a look at:

    http://taskwarrior.org/support

Please send your code patches to:

    support@taskwarrior.org

Consider joining bug.tasktools.org, answers.tasktools.org and participating in
the future of Taskwarrior.

---

Taskwarrior is released under the MIT license. For details check the LICENSE
file.
