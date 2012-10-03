$!COMPILE.COM:
$! P1 = Source file path relative to project root in unix style.
$! P2 = Optional compiler flags.
$!
$!SUB_VMS2UNIX
$! P1 = input VMS path
$! P2 = output UNIX path
$ SUB_VMS2UNIX: SUBROUTINE
$     VMS_PATH = P1
$     VMS_NODE = F$PARSE("''VMS_PATH'",,,"NODE")
$     VMS_DEV  = F$PARSE("''VMS_PATH'",,,"DEVICE")
$     VMS_DIR  = F$PARSE("''VMS_PATH'",,,"DIRECTORY")
$     VMS_NAME = F$PARSE("''VMS_PATH'",,,"NAME")
$     VMS_EXT  = F$PARSE("''VMS_PATH'",,,"TYPE")
$     VMS_VER  = F$PARSE("''VMS_PATH'",,,"VERSION")
$!    ! TODO: no idea how to handle nodes.
$!
$     UNIX_PATH = ""
$     IF VMS_NODE .NES. ""
$     THEN
$         UNIX_PATH = "''VMS_NODE'"
$     ENDIF
$!
$     IF VMS_DEV .NES. ""
$     THEN
$!        ! Chop off the ":" character that comes with the device.
$         RAW_DEV = F$EXTRACT(0, F$LENGTH(VMS_DEV)-1, VMS_DEV)
$         IF RAW_DEV .NES. "" ! Paranoia.
$         THEN
$             UNIX_PATH = "''UNIX_PATH'/''RAW_DEV'"
$         ENDIF
$     ENDIF
$!
$     IF VMS_DIR .NES. ""
$     THEN
$         ! Chop off the "[]" or "<>" characters on directories.
$         RAW_DIR = F$EXTRACT(1, F$LENGTH(VMS_DIR)-2, VMS_DIR)
$!
$         ! Convert . to /
$       VMS_DOT_CONVERT:
$         TARGET_POS = F$LOCATE(".", RAW_DIR)
$         IF TARGET_POS .NE. F$LENGTH(RAW_DIR)
$         THEN
$             RAW_DIR[TARGET_POS,1] := "/"
$             GOTO VMS_DOT_CONVERT
$         ENDIF
$!
$         IF RAW_DIR .NES. "" ! More paranoia.
$         THEN
$             UNIX_PATH = "''UNIX_PATH'/''RAW_DIR'"
$         ENDIF
$     ENDIF
$!
$     IF VMS_NAME .NES. ""
$     THEN
$         UNIX_PATH = "''UNIX_PATH'/''VMS_NAME'"
$     ENDIF
$!
$     IF VMS_EXT .NES. "" .AND. VMS_EXT .NES. "."
$     THEN
$!        ! Note: it will already have the dot on it.
$         UNIX_PATH = UNIX_PATH + VMS_EXT
$     ENDIF
$!
$     IF VMS_VER .NES. "" .AND. VMS_VER .NES. ";"
$     THEN
$!        ! Note: it will already have the ; on it.
$         UNIX_PATH = UNIX_PATH + VMS_VER
$     ENDIF
$!
$     IF F$EXTRACT(0,1,UNIX_PATH) .NES. "/"
$     THEN
$         UNIX_PATH = "/''UNIX_PATH'"
$     ENDIF
$!
$     'P2' == UNIX_PATH
$     EXIT
$ ENDSUBROUTINE
$!
$! P1 = input path (ex: "foo/bar/baz")
$! P2 = output munged path (ex: "foo_bar_baz")
$ SUB_MUNGE_PATH: SUBROUTINE
$     MUNGE_PATH = P1
$   MUNGE_LOOP:
$     MUNGE_SLASH_POS = F$LOCATE("/",MUNGE_PATH)
$     IF MUNGE_SLASH_POS .NE. F$LENGTH(MUNGE_PATH)
$     THEN
$         MUNGE_PATH[MUNGE_SLASH_POS,1] := "_"
$         GOTO MUNGE_LOOP
$     ENDIF
$     'P2' == MUNGE_PATH
$     EXIT
$ ENDSUBROUTINE
$!
$! P1 = input relative unix FILE path (ex: "foo/bar/baz.c")
$! P2 = output incomplete VMS path (ex: "foo.bar]baz.c")
$ SUB_PARTIAL_VMSIFY: SUBROUTINE
$     WIP_PATH = P1
$!
$     LAST_SLASH_POS = F$LENGTH(WIP_PATH)-1
$   FIND_LAST_SLASH:
$     IF LAST_SLASH_POS .GE. 0
$     THEN
$         THIS_CHARACTER = F$EXTRACT(LAST_SLASH_POS, 1, WIP_PATH)
$         IF THIS_CHARACTER .EQS. "/"
$         THEN
$             WIP_PATH[LAST_SLASH_POS,1] := "]"
$         ELSE
$             LAST_SLASH_POS = LAST_SLASH_POS - 1
$             GOTO FIND_LAST_SLASH
$         ENDIF
$     ENDIF
$!
$   REPLACE_SLASHES:
$     NEXT_SLASH_POS = F$LOCATE("/", WIP_PATH)
$     IF NEXT_SLASH_POS .NE. F$LENGTH(WIP_PATH)
$     THEN
$         WIP_PATH[NEXT_SLASH_POS,1] := "."
$         GOTO REPLACE_SLASHES
$     ENDIF
$!
$     'P2' == WIP_PATH
$     EXIT
$ ENDSUBROUTINE
$!
$! This argument should be in relative unix format:  path/to/src.c
$ CFILE_UNIX_REL_PATH = P1
$!
$ THIS_COM_FILE = F$ENVIRONMENT("PROCEDURE")
$ COM_DIR  = F$PARSE("''THIS_COM_FILE'",,,"DIRECTORY")
$ COM_DEV  = F$PARSE("''THIS_COM_FILE'",,,"DEVICE")
$ PROJECT_VMS_PATH = "''COM_DEV'" + "''COM_DIR'"
$ CALL SUB_VMS2UNIX 'PROJECT_VMS_PATH' PROJECT_UNIX_PATH
$!
$! Grab the first part of the relative path:
$!   survival_kit/feature_emulation/unittest.c -> CFILE_UNIX_DIR = "survival_kit"
$!   tools_src/unittests.c -> CFILE_UNIX_DIR = "tools_src.c"
$! This information is used to determine whether to build executable binaries
$! or library files.
$ CFILE_SLASH_LOC = F$LOCATE("/",CFILE_UNIX_REL_PATH)
$ CFILE_UNIX_DIR = F$EXTRACT(0, CFILE_SLASH_LOC, CFILE_UNIX_REL_PATH)
$ CFILE_UNIX_CAP_DIR = F$EDIT(CFILE_UNIX_DIR, "UPCASE")
$!
$! Also grab the latter part of the relative path.
$!   survival_kit/feature_emulation/unittest.c -> CFILE_UNIX_SUBPATH = "feature_emulation/unittest.c"
$!   tools_src/unittests.c -> CFILE_UNIX_SUBPATH = "unittests.c"
$ CFILE_UNIX_SUBPATH = F$EXTRACT(CFILE_SLASH_LOC+1, F$LENGTH(CFILE_UNIX_REL_PATH), CFILE_UNIX_REL_PATH)
$!
$! Now munge it.
$! The result can be used in temporary files.
$! Ex: "survival_kit/feature_emulation/unittest.c" -> "feature_emulation_unittest.obj"
$ CALL SUB_MUNGE_PATH 'CFILE_UNIX_SUBPATH' CFILE_MUNGED_NAME
$!
$! We'll also need the absolute VMS path to the C source file.
$ CALL SUB_PARTIAL_VMSIFY 'CFILE_UNIX_REL_PATH' CFILE_VMS_PARTIAL_PATH
$ NAKED_DIR = F$EXTRACT(1, F$LENGTH(COM_DIR)-2, COM_DIR)
$ CFILE_VMS_PATH = "''COM_DEV'[''NAKED_DIR'.''CFILE_VMS_PARTIAL_PATH'"
$!
$! Now that we finally have all of the paths we need, it's compilin' time.
$ WRITE SYS$OUTPUT "Compiling ''CFILE_VMS_PATH'"
$!
$ CC /OBJ=SYS$SCRATCH:'CFILE_MUNGED_NAME.OBJ -
    /LIS=SYS$SCRATCH:'CFILE_MUNGED_NAME.LIS - 
    /NESTED_INCLUDE_DIR=NONE -
    /INCLUDE_DIR=("''PROJECT_UNIX_PATH'/") -
    'CFILE_VMS_PATH -
	'P2
$!
$! PORTABILITY: Converting the list file using CMD:STREAM_LF requires STREAM_LF to exist.
$ CONVERT /FDL=CMD:STREAM_LF SYS$SCRATCH:'CFILE_MUNGED_NAME.LIS CLIST:'CFILE_MUNGED_NAME.LIS
$ DEL SYS$SCRATCH:'CFILE_MUNGED_NAME.LIS;*
$!
$! Compilation of library files.
$ IF CFILE_UNIX_CAP_DIR .EQS. "SURVIVAL_KIT"
$ THEN
$     ! PORTABILITY: OBJLIB must be defined for this to work.
$     LIB/REP OBJLIB:C_UTIL.OLB SYS$SCRATCH:'CFILE_MUNGED_NAME.OBJ
$     !
$ ELSE
$     IF CFILE_UNIX_CAP_DIR .EQS. "TOOLS_SRC"
$     THEN
$         ! PORTABILITY: OBJLIB must be defined for this to work.
$         LINK SYS$SCRATCH:'CFILE_MUNGED_NAME.OBJ,OBJLIB:C_UTIL/LIB /NOMAP/EXE=BIN:'CFILE_MUNGED_NAME.EXE /THREADS_ENABLE
$         !
$         SKIT_'CFILE_MUNGED_NAME :== $bin:'CFILE_MUNGED_NAME.EXE
$     ELSE
$         WRITE SYS$OUTPUT "Error: ''CFILE_UNIX_CAP_DIR' is not an a subdirectory that"
$         WRITE SYS$OUTPUT "       the build script knows what to do with."
$     ENDIF
$ ENDIF
$!
$ DEL SYS$SCRATCH:'CFILE_MUNGED_NAME.OBJ;*
$!
