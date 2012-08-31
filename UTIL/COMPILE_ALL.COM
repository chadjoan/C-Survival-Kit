$ SET NOVERIFY
$ !
$ WRITE SYS$OUTPUT "Building object library OBJLIB:C_UTIL.OLB"
$ !
$ LIBRARY/CREATE=(GLO:600,BLO:500) OBJLIB:C_UTIL.OLB
$ PURGE OBJLIB:C_UTIL.OLB
$
$! @cutil:compile lang_util
$ @cutil:compile str_util
$ @cutil:compile err_util
$ @cutil:compile util_exceptions_generated
$ @cutil:compile sockn
$
