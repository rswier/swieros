// mem.h -- modifed K&R memory allocator  XXX do not use... currently under development

struct alloc_s { struct alloc_s *ptr; uint size; } *allocp;

void free(void *ptr)
{
  struct alloc_s *p, *q;

  p = (struct alloc_s *)ptr - 1;
  for (q = allocp; p <= q || p >= q->ptr; q = q->ptr)
    if (q >= q->ptr && (p > q || p < q->ptr))
      break;
  if (p + p->size == q->ptr) {
    p->size += q->ptr->size;
    p->ptr = q->ptr->ptr;
  } else
    p->ptr = q->ptr;
  if (q + q->size == p) {
    q->size += p->size;
    q->ptr = p->ptr;
  } else
    q->ptr = p;
  allocp = q;
}

void *malloc(uint n)
{
  static struct alloc_s base;
  struct alloc_s *p, *q;
  uint nu;

  n = 1 + (n + sizeof(struct alloc_s) - 1) / sizeof(struct alloc_s);
  if (!(q = allocp)) {
    base.ptr = allocp = q = &base; // XXX do this with sbrk vs base?
    base.size = 0;
  }
  for (p = q->ptr; ; q = p, p = p->ptr) {
    if (p->size >= n) {
      if (p->size == n)
        q->ptr = p->ptr;
      else {
        p->size -= n;
        p += p->size;
        p->size = n;
      }
      allocp = q;
      return (void *)(p + 1);
    }
    if (p == allocp) {
      nu = n < 4096 ? 4096 : n;
      p = (struct alloc_s *) sbrk(nu * sizeof(struct alloc_s));
      if ((int)p == -1) return 0;
      p->size = nu;
      free((void *)(p + 1));
      p = allocp;
    }
  }
}

void *memalign(uint bound, uint n)
{

}

void *realloc(void *ptr, uint n)
{

}

