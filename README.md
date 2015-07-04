Minimal Migration Manager
=========================

[![Travis CI Status](https://secure.travis-ci.org/thentenaar/mmm.svg?branch=master)](https://travis-ci.org/thentenaar/mmm)
[![Coverage Status](https://coveralls.io/repos/thentenaar/mmm/badge.svg?branch=master)](https://coveralls.io/r/thentenaar/mmm)
[![Coverity Status](https://scan.coverity.com/projects/5663/badge.svg)](https://scan.coverity.com/projects/5663)

Synopsis
--------
```
Usage: ./mmm [-h] [-f config_file] command [command options...]

  Options:
    -h              Show this message.
    -f config_file  Configuration file to use.
                    (defaults to ./mmm.conf)

  Commands:
     init [config]       Create a new migration directory and config
                         file unless 'config' is specified, which
                         creates a config file only.
     seed <seed file>    Seed the database with a .sql file.
     head                Get the latest local revision.
     pending             List all migrations yet unapplied.
     migrate             Apply all pending migrations.
     rollback [revision] Unapply all migrations since <revision>
                         which defaults to the current previous
                         revision.
     assimilate          Track an existing database, assuming
                         that all migrations have been applied.
```

Description
-----------

``mmm`` is a simple database schema management tool using plain SQL
files to represent changes in the schema, and one tracking table
to track the current state.

To quickly get up and running, do the following:

1. Create your database
2. Run ``mmm init`` to create the config and migration directory.
3. Edit the generated config.
4. If you have a seed file, run it with ``mmm seed``.
5. Otherwise, run ``mmm assimilate`` to create the state table.
6. Add your migrations to the migration directory.
7. Use ``mmm migrate`` to run them.

Note that your first batch of migrations won't record a previous revision
as there is no previous revision to record, if you create your database
by way of the ``seed`` command. However, ``assimilate`` will record
the local revision.

Dependencies
------------

Required (at least one of these):
  - ``libsqlite3`` for SQLite3 support.
  - ``libpq`` for PostgreSQL support.
  - ``libmysqlclient`` for MySQL support.

Optional:
  - [libgit2](https://libgit2.github.com) for the ``git`` source.
  - [cunit](http://cunit.sourceforge.net) for building the tests.
  - [gcovr](http://gcovr.com) for generating coverage reports.

Installation
------------

``mmm`` can be installed with the typical ``make`` followed by
``make install``. Alternatively, you can also run the automated
tests, and generate a coverage report.

- To run the tests: ``make check``
- To generate a coverage report: ``make coverage``
- To uninstall: ``make uninstall``

*NOTE*: The pkgconfig files for Debian-based distros seem to be
either non-existent or broken for CUnit. If the tests fail to build,
try setting CUNIT_LIBS, like so:  ``CUNIT_LIBS=-lcunit make ...``.

Configuration File
------------------

The configuration file is a typical ini-style configuration file,
See the default file generated by ``mmm init`` for an explanation of
the options, although they should be self-explanatory.

Database Drivers
----------------

Three database drivers are available: ``sqlite3``, ``pgsql`` and
``mysql``, and are usable assuming the aforementioned dependencies
are available.

For ``sqlite3`` only the ``db`` option need be specified, and it should
be the path to the SQLite3 database to use.

For ``pgsql`` and ``mysql`` all other options should also be specified.
To use a UNIX socket with these two, set the ``host`` option to the
path to the socket, and ``port`` to 0. The client character set will
default to UTF-8.

With ``mysql`` defaults will be read from the mysql section of the
config file for any unspecified parameters.

Migration Files
---------------

The migration files are plain SQL files, split into two sections like
so:
```sql
-- [up]
CREATE TABLE test(id INTEGER NOT NULL);

-- [down]
DROP TABLE test;
```

- The ``up`` section contains the SQL queries to run when applying a
migration.
- The ``down`` section contains the SQL queries to run when rolling back
a migration.

They can be specified in either order.

Sources
-------

Two sources are currently supported: ``file`` and ``git``.

The ``file`` source looks at the files in the specified
``migration_path`` in order to determine the order that the
files should be applied. This source assumes that all migrations
begin with a numeric designation, for example: ``1-init.sql``;
and will be arranged in numerical order. This is the default
source. Revisions correspond to the files' designations.

The ``git`` source uses a git repository for determining the order
in which migrations should be applied. With this source, the files
need not have a numeric designation, and they will be applied in
the order in which they were committed to the repository. Revisions
correspond to the actual SHA1 hash for the commit at which the
current aet of migrations was performed.

The ``git`` source requires [libgit2](https://libgit2.github.com).

Limitations
-----------

``mmm`` can only handle files smaller than 64 KB by default. To raise
this limit, increase ``FILEBUFSIZ`` in [src/file.c](src/file.c)
accordingly.

Caveats
-------

``mmm`` uses transactions to ensure that if an error occurs, the
database is returned to a known state. However, not all RDMBS support
transactional DDL (Data Definition Language) statements, and therefore
``mmm`` cannot guarantee that databases on these RDBMS systems will be
in the desired state when rolling back the transaction.

If an error occurs, and your database lacks transactional DDL support,
mmm will emit an error message advising you of the situation. In these
cases, you will need to manually verify that your schema is correct.

In case of upgrading the schema, ``mmm`` will automatically roll back
any applied migrations in the current batch when an error occurs if the
RDBMS lacks transactional DDL support.

MySQL
-----

MySQL is supported, although you should be aware that any DDL commands
will cause an implicit commit. Thus, when writing migrations, you should use ``IF [NOT] EXISTS`` on your ``CREATE`` and ``DROP`` statements to
ensure that migrations can still run if, for example, and added table
could not be successfully dropped on rollback.

