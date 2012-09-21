#ifndef SKIT_STREAM_INCLUDED
#define SKIT_STREAM_INCLUDED

#define SKIT_STREAM_VTABLE_SIZE 5
typedef enum skit_stream_type skit_stream_type;
enum skit_stream_type
{
	SKIT_CLASS_STREAM,
	SKIT_CLASS_TEXT_STREAM_CLASS,
	SKIT_CLASS_FILE_STREAM_CLASS,
	SKIT_CLASS_PFILE_STREAM_CLASS, 
	ASTD_CLASS_RMS_STREAM_CLASS,
};

typedef struct skit_stream_vtable_t skit_stream_vtable_t;
struct skit_stream_vtable_t
{
	/* Basic stream operations. */
	void       (*init)        (skit_stream*);
	skit_slice (*read_line)   (skit_stream*);
	skit_slice (*read_bytes)  (skit_stream*);
	void       (*write_line)  (skit_stream*,skit_slice);
	void       (*write_bytes) (skit_stream*,skit_slice);
	void       (*flush)       (skit_stream*);
	void       (*rewind)      (skit_stream*);
	skit_slice (*slurp)       (skit_stream*);
	skit_slice (*to_slice)    (skit_stream*);
	void       (*dump)        (skit_stream*,skit_stream*);
	
	/* File stream operations. */
	void       (*open)        (skit_file_stream*,skit_slice,const char*);
	void       (*close)       (skit_file_stream*);
};

skit_stream_vtable_t skit_stream_vtables[SKIT_STREAM_VTABLE_SIZE];

static void skit_stream_func_not_impl(skit_stream* stream)
{
	sTHROW(SKIT_NOT_IMPLEMENTED,
		"Attempt to call a virtual function on an "
		"instance of the abstract class skit_stream (or skit_file_stream).");
}

static skit_slice skit_stream_read_not_impl(skit_stream* stream)
{
	skit_stream_func_not_impl(stream);
	return skit_slice_null();
}

static void skit_stream_write_not_impl(skit_stream *stream, skit_slice slice)
{
	skit_stream_func_not_impl(stream);
}

static void skit_stream_dump_not_impl(skit_stream* stream, skit_stream* output)
{
	skit_stream_func_not_impl(stream);
}


static void skit_file_stream_open_not_impl(skit_file_stream* stream, skit_slice fname, const char *mode)
{
	skit_stream_func_not_impl((skit_stream*)stream);
}


static void skit_file_stream_close_not_impl(skit_file_stream* stream)
{
	skit_stream_func_not_impl((skit_stream*)stream);
}

void skit_stream_vtable_init(skit_stream_vtable_t *table)
{
	table->init          = &skit_stream_func_not_impl;
	table->read_line     = &skit_stream_read_not_impl;
	table->read_bytes    = &skit_stream_read_not_impl;
	table->write_line    = &skit_stream_write_not_impl;
	table->write_bytes   = &skit_stream_write_not_impl;
	table->flush         = &skit_stream_func_not_impl;
	table->rewind        = &skit_stream_func_not_impl;
	table->slurp         = &skit_stream_read_not_impl;
	table->to_slice      = &skit_stream_read_not_impl;
	table->dump          = &skit_stream_dump_not_impl;
	table->open          = &skit_file_stream_open_not_impl;
	table->close         = &skit_file_stream_close_not_impl;
}


void skit_text_stream_vtable_init(skit_stream_vtable_t *table)
{
	skit_stream_vtable_init(table);
	table->init          = &skit_text_stream_init;
	table->read_line     = &skit_text_stream_read_line;
	table->read_bytes    = &skit_text_stream_read_bytes;
	table->write_line    = &skit_text_stream_write_line;
	table->write_bytes   = &skit_text_stream_write_bytes;
	table->flush         = &skit_text_stream_flush;
	table->rewind        = &skit_text_stream_rewind;
	table->slurp         = &skit_text_stream_slurp;
	table->to_slice      = &skit_text_stream_to_slice;
	table->dump          = &skit_text_stream_dump;
}

void skit_file_stream_vtable_init(skit_stream_vtable_t *table)
{
	skit_stream_vtable_init(table);
}



void skit_stream_static_init()
{
	skit_stream_vtable_init      (&skit_stream_vtables[SKIT_CLASS_STREAM]);
	skit_text_stream_vtable_init (&skit_stream_vtables[SKIT_CLASS_TEXT_STREAM]);
	skit_file_stream_vtable_init (&skit_stream_vtables[SKIT_CLASS_FILE_STREAM]);
	skit_pfile_stream_vtable_init(&skit_stream_vtables[SKIT_CLASS_PFILE_STREAM]);
	astd_rms_stream_vtable_init  (&astd_stream_vtables[SKIT_CLASS_RMS_STREAM]);
}

typedef struct skit_stream_metadata skit_stream_metadata;
struct skit_stream_metadata
{
	skit_stream_type stream_type;
	/* Other stuff can go here if it has to. */
};

union skit_stream
{
	skit_stream_metadata meta;
	skit_text_stream     as_text_stream;
	skit_file_stream     as_file_stream;
	astd_rms_stream      as_rms_stream;
};

union skit_file_stream
{
	
};

void skit_init_stream(skit_stream *stream)
{
	stream->meta.stream_type = SKIT_STREAM_CLASS;
}

typedef struct skit_text_stream skit_text_stream;
struct skit_text_stream
{
	skit_stream_metadata meta;
	skit_loaf   buffer;
	skit_slice  text;
	char        indent_char;
	short       indent_level;
};


#include <fcntl.h>
typedef struct skit_file_stream skit_file_stream;
struct skit_file_stream
{
	skit_stream_metadata meta;
	skit_loaf name;
	skit_loaf err_msg_buf;
	int file_descriptor;
};

void skit_file_stream_open(skit_file_stream *stream, skit_slice fname, const char *mode)
{
}

skit_slice skit_stream_read_line(skit_stream *stream)
{
	switch( stream->meta.stream_type )
	{
		case SKIT_STREAM_CLASS:
			sTHROW(SKIT_NOT_IMPLEMENTED,
				"Attempt to call skit_stream_read_line on an "
				"instance of the abstract class skit_stream.");
			break;
		
		case SKIT_STREAM_TEXT_STREAM_CLASS:
			return skit_text_stream_read_line(stream);
		
		case SKIT_STREAM_FILE_STREAM_CLASS:
			return skit_file_stream_read_line(stream);
		
		case ASTD_RMS_STREAM_CLASS:
			return astd_rms_stream_read_line(stream);
	}
}

skit_slice skit_text_stream_read_line(
skit_slice skit_file_stream_read_line(

#endif
