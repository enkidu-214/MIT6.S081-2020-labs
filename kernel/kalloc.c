// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
//引用计数的记录数组
int refcount[(PHYSTOP-KERNBASE) / PGSIZE];
#define PAIDX(a) (((a) - KERNBASE) / PGSIZE)

struct spinlock reflock;
struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  //初始化引用计数
  initlock(&reflock, "ref");
  memset(refcount,0,sizeof(refcount));
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  uint64 refidx=PAIDX((uint64)pa);
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  acquire(&reflock);
  refcount[refidx]--;
  
  if(refcount[refidx]<0)
    refcount[refidx]=0;

  if(refcount[refidx]==0){
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&reflock);  
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r){
    acquire(&reflock);
    refcount[PAIDX((uint64)r)]=1;
    release(&reflock); 
    memset((char*)r, 5, PGSIZE); // fill with junk
  }

  return (void*)r;
}

int refincrease(uint64 pa)
{
  acquire(&reflock);
  refcount[PAIDX(pa)]++;
  release(&reflock);
  return refcount[PAIDX(pa)];
}

int getref(uint64 pa){
  return refcount[PAIDX(pa)];
}