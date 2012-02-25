typedef struct glistent	*GLIST_ENT;
typedef struct glist	*GLIST;

typedef struct glist	*SLIST;

struct glistent
  {
    GLIST_ENT	next;
    void	*data;
  };
struct glist
  {
    GLIST_ENT	list, *last;
    int		size;
  };

static GLIST
glist_new(size_t size)
{
  GLIST	list;

  list		= alloc(sizeof *list);
  list->list	= 0;
  list->last	= &list->list;
  list->size	= size;
  return list;
}

static GLIST_ENT
glist_add(GLIST list)
{
  GLIST_ENT	ent;

  ent		= alloc(sizeof *ent);
  ent->next	= 0;
  ent->data	= 0;
  *list->last	= ent;
  list->last	= &ent->next;
  if (list->size)
    ent->data	= alloc(list->size);
  return ent;
}

static GLIST_ENT
glist_get(GLIST list)
{
  GLIST_ENT	ent;

  xDP(("glist_get(%p)", list));
  if (!list || (ent=list->list)==0)
    return 0;
  if ((list->list=ent->next)==0)
    list->last	= &list->list;
  return ent;
}

static void
glist_free(GLIST_ENT ent)
{
  xDP(("glist_free(%p)", ent));
  if (ent->data)
    free(ent->data);
  ent->next	= 0;
  ent->data	= 0;
  free(ent);
}

static void *
glist_fetchfree(GLIST_ENT ent)
{
  void	*data;

  data		= ent->data;
  ent->data	= 0;
  glist_free(ent);
  return data;
}

static SLIST
slist_new(void)
{
  return (SLIST)glist_new((size_t)0);
}

static void
slist_add(SLIST list, const char *s)
{
  GLIST_ENT	ent;

  ent		= glist_add((GLIST)list);
  ent->data	= str_alloc(s);
}

static SLIST
slist_init(int argc, char **argv)
{
  int	i;
  SLIST	list;

  list	= slist_new();
  for (i=0; i<argc; i++)
    slist_add(list, argv[i]);
  return list;
}

static const char *
slist_get(SLIST list)
{
  GLIST_ENT	ent;

  xDP(("slist_get(%p)", list));
  if ((ent=glist_get((GLIST)list))==0)
    return 0;
  return glist_fetchfree(ent);
}

static void
slist_free(const char *s)
{
  free((char *)s);
}
