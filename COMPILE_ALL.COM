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
$ @'THIS_DIR'compile survival_kit/macro                                "''P1'"
$ @'THIS_DIR'compile survival_kit/memory                               "''P1'"
$ @'THIS_DIR'compile survival_kit/misc                                 "''P1'"
$ @'THIS_DIR'compile survival_kit/inheritance_table                    "''P1'"
$ @'THIS_DIR'compile survival_kit/cstr                                 "''P1'"
$ @'THIS_DIR'compile survival_kit/stack_builtins                       "''P1'"
$ @'THIS_DIR'compile survival_kit/fstack_builtins                      "''P1'"
$ @'THIS_DIR'compile survival_kit/feature_emulation/setjmp/jmp_stack   "''P1'"
$ @'THIS_DIR'compile survival_kit/feature_emulation/setjmp/jmp_fstack  "''P1'"
$ @'THIS_DIR'compile survival_kit/feature_emulation/exception          "''P1'"
$ @'THIS_DIR'compile survival_kit/feature_emulation/thread_context     "''P1'"
$ @'THIS_DIR'compile survival_kit/feature_emulation/frame_info         "''P1'"
$ @'THIS_DIR'compile survival_kit/feature_emulation/stack_trace        "''P1'"
$ @'THIS_DIR'compile survival_kit/feature_emulation/unittest           "''P1'"
$ @'THIS_DIR'compile survival_kit/feature_emulation/init               "''P1'"
$ @'THIS_DIR'compile survival_kit/bag                                  "''P1'"
$ @'THIS_DIR'compile survival_kit/signal_handling                      "''P1'"
$ @'THIS_DIR'compile survival_kit/flags                                "''P1'"
$ @'THIS_DIR'compile survival_kit/math                                 "''P1'"
$ @'THIS_DIR'compile survival_kit/datetime                             "''P1'"
$ @'THIS_DIR'compile survival_kit/string                               "''P1'"
$ @'THIS_DIR'compile survival_kit/trie                                 "''P1'"
$ @'THIS_DIR'compile survival_kit/regex                                "''P1'"
$ @'THIS_DIR'compile survival_kit/path                                 "''P1'"
$ @'THIS_DIR'compile survival_kit/parsing/peg                          "''P1'"
$ @'THIS_DIR'compile survival_kit/array_builtins                       "''P1'"
$ @'THIS_DIR'compile survival_kit/streams/stream                       "''P1'"
$ @'THIS_DIR'compile survival_kit/streams/text_stream                  "''P1'"
$ @'THIS_DIR'compile survival_kit/streams/file_stream                  "''P1'"
$ @'THIS_DIR'compile survival_kit/streams/pfile_stream                 "''P1'"
$ @'THIS_DIR'compile survival_kit/streams/tcp_stream                   "''P1'"
$ @'THIS_DIR'compile survival_kit/streams/ind_stream                   "''P1'"
$ @'THIS_DIR'compile survival_kit/streams/empty_stream                 "''P1'"
$ @'THIS_DIR'compile survival_kit/streams/init                         "''P1'"
$ @'THIS_DIR'compile survival_kit/init                                 "''P1'"
$ @'THIS_DIR'compile tools_src/unittests                               "''P1'"
$
