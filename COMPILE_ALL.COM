$ SET NOVERIFY
$ !
$ WRITE SYS$OUTPUT "Building object library OBJLIB:C_UTIL.OLB"
$ !
$ THIS_COM_FILE = F$ENVIRONMENT("PROCEDURE")
$ THIS_PATH = F$PARSE("''THIS_COM_FILE'",,,"DIRECTORY")
$ THIS_DEV  = F$PARSE("''THIS_COM_FILE'",,,"DEVICE")
$ THIS_DIR  = "''THIS_DEV'" + "''THIS_PATH'"
$ !
$! PORTABILITY: OBJLIB logical and C_UTIL.OLB filename.
$ !
$ LIBRARY/CREATE=(GLO:600,BLO:500) OBJLIB:C_UTIL.OLB
$ PURGE OBJLIB:C_UTIL.OLB
$!
$ @'THIS_DIR'compile stack_builtins
$ @'THIS_DIR'compile str
$ @'THIS_DIR'compile generated_exception_defs
$ @'THIS_DIR'compile features
$ @'THIS_DIR'compile sockn
$
