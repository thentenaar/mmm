/* Typedefs */
-T mode_t -T off_t -T ssize_t
-T sigjmp_buf -T jmp_buf -T sig_atomic_t
-T db_session -T sqlite3 -T MYSQL -T PGconn
-T MYSQL_ROW -T git_strarray -T git_diff_options
-T git_diff_find_options -T git_diff_file -T git_diff_delta
-T git_diff -T git_oid -T git_repository -T git_object
-T git_revwalk -T git_commit -T git_tree

/**
 * -kr: K&R Style
 * -l74: Line length: 74
 * -i8:  Indent 8 columns (1 tab)
 * -il0: Labels aligned to column 1
 * -ts8: Tab size = 8 columns
 * -ut: Indent with tabs
 * -as: Align with spaces
 * -slc: Allow single-line conditionals
 * -cdw: Cuddle while up to do
 * -ncs: No spaces between a cast and the object being casted
 * -cp1: Put comments 1 column to the right of #else and #endif
 * -c1:  Put comments 1 column to the right of code
 * -cd1: Put comments 1 column to the right of declarations
 * -ntac: Don't tab-align comments
 */
-kr -l74 -i8 -il0 -ts8 -ut -as -slc -cdw -ncs -cp1 -c1 -cd1 -ntac
