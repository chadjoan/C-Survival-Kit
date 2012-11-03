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
$ @'THIS_DIR'compile survival_kit/macro
$ @'THIS_DIR'compile survival_kit/memory
$ @'THIS_DIR'compile survival_kit/misc
$ @'THIS_DIR'compile survival_kit/cstr
$ @'THIS_DIR'compile survival_kit/stack_builtins
$ @'THIS_DIR'compile survival_kit/fstack_builtins
$ @'THIS_DIR'compile survival_kit/feature_emulation/setjmp/jmp_stack
$ @'THIS_DIR'compile survival_kit/feature_emulation/setjmp/jmp_fstack
$ @'THIS_DIR'compile survival_kit/feature_emulation/exceptions
$ @'THIS_DIR'compile survival_kit/feature_emulation/funcs
$ @'THIS_DIR'compile survival_kit/feature_emulation/types
$ @'THIS_DIR'compile survival_kit/feature_emulation/unittest
$ @'THIS_DIR'compile survival_kit/signal_handling
$ @'THIS_DIR'compile survival_kit/math
$ @'THIS_DIR'compile survival_kit/string
$ @'THIS_DIR'compile survival_kit/array_builtins
$ @'THIS_DIR'compile survival_kit/streams/stream
$ @'THIS_DIR'compile survival_kit/streams/text_stream
$ @'THIS_DIR'compile survival_kit/streams/file_stream
$ @'THIS_DIR'compile survival_kit/streams/pfile_stream
$ @'THIS_DIR'compile survival_kit/streams/init
$ @'THIS_DIR'compile survival_kit/sockn
$ @'THIS_DIR'compile survival_kit/init
$ @'THIS_DIR'compile tools_src/unittests
$