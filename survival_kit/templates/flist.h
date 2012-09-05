
Declaration-free way to allocate stack memory and access it:
if (skit_jmp_buf_flist_full(&list))
	skit_jmp_buf_flist_grow(&list,alloca(sizeof(skit_jmp_buf_snode*)));
setjmp(*skit_jmp_buf_flist_push(&list));

typedef struct SKIT_T(flist) SKIT_T(flist);
struct SKIT_T(flist)
{
	SKIT_T(slist) unused;
	SKIT_T(slist) used;
};

void SKIT_T(flist_init)( SKIT_T(flist) *list )
{
	SKIT_T(slist_init)(&(list->unused));
	SKIT_T(slist_init)(&(list->used));
}

int SKIT_T(flist_full)( SKIT_T(flist) *list )
{
	if ( unused.length == 0 )
		return 1;
	else
		return 0;
}

void SKIT_T(flist_grow)( SKIT_T(flist) *list, void *node )
{
	SKIT_T(slist_push)(&(list->unused), (SKIT_T(snode)*)node);
}

void SKIT_T(flist_shrink)( SKIT_T(flist) *list, int num_nodes )
{
	SKIT_T(slist_push)(&(list->unused), (SKIT_T(snode)*)node);
}

SKIT_T_ELEM_TYPE *SKIT_T(flist_push)( SKIT_T(flist) *list )
{
	SKIT_T(snode)* result = SKIT_T(slist_pop)(&(list->unused));
	SKIT_T(slist_push)(&(list->used),result);
	return &result.val;
}

SKIT_T_ELEM_TYPE *SKIT_T(flist_pop)( SKIT_T(flist) *list )
{
	SKIT_T(snode)* result = SKIT_T(slist_pop)(&(list->used));
	SKIT_T(slist_push)(&(list->unused),result);
	return &result.val;
}