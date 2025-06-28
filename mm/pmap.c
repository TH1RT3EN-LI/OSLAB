#include "mmu.h"
#include "pmap.h"
#include "printf.h"
#include "env.h"
#include "error.h"

/* These variables are set by mips_detect_memory() */
u_long maxpa;   /* Maximum physical address */
u_long npage;   /* Amount of memory(in pages) */
u_long basemem; /* Amount of base memory(in bytes) */
u_long extmem;  /* Amount of extended memory(in bytes) */

Pde *boot_pgdir;

struct Page *pages;
static u_long freemem;

static struct Page_list page_free_list; /* Free list of physical pages */

/* Overview:
    Initialize basemem and npage.
    Set basemem to be 64MB, and calculate corresponding npage value.*/
/**
 *   basemem - Base memory size in bytes
 *   maxpa   - Maximum physical address
 *   extmem  - Extended memory size in bytes, but not used in this function, set 0
 *   npage   - Number of physical memory page
 *   PPN(va) - Macro to get the page number from a virtual address

 */
void mips_detect_memory()
{
    /* Step 1: Initialize basemem.
     * (When use real computer, CMOS tells us how many kilobytes there are). */
    basemem = 64 * 1024 * 1024;
    maxpa = basemem;
    extmem = 0;
    // Step 2: Calculate corresponding npage value.
    npage = PPN(maxpa);

    printf("Physical memory: %dK available, ", (int)(maxpa / 1024));
    printf("base = %dK, extended = %dK\n", (int)(basemem / 1024),
           (int)(extmem / 1024));
}

/* Overview:
    Allocate `n` bytes physical memory with alignment `align`, if `clear` is set, clear the
    allocated memory.
    This allocator is used only while setting up virtual memory system.

   Post-Condition:
    If we're out of memory, should panic, else return this address of memory we have allocated.*/
static void *alloc(u_int n, u_int align, int clear)
{
    extern char end[];
    u_long alloced_mem;

    /* Initialize `freemem` if this is the first time. The first virtual address that the
     * linker did *not* assign to any kernel code or global variables. */
    if (freemem == 0)
    {
        freemem = (u_long)end;
    }

    /* Step 1: Round up `freemem` up to be aligned properly */
    freemem = ROUND(freemem, align);

    /* Step 2: Save current value of `freemem` as allocated chunk. */
    alloced_mem = freemem;

    /* Step 3: Increase `freemem` to record allocation. */
    freemem = freemem + n;

    /* Step 4: Clear allocated chunk if parameter `clear` is set. */
    if (clear)
    {
        bzero((void *)alloced_mem, n);
    }

    // We're out of memory, PANIC !!
    if (PADDR(freemem) >= maxpa)
    {
        panic("out of memorty\n");
        return (void *)-E_NO_MEM;
    }

    /* Step 5: return allocated chunk. */
    return (void *)alloced_mem;
}

/* Overview:
    Get the page table entry for virtual address `va` in the given
    page directory `pgdir`.
    If the page table is not exist and the parameter `create` is set to 1,
    then create it.*/
static Pte *boot_pgdir_walk(Pde *pgdir, u_long va, int create)
{
    // remind，PDX is page directory index, PTX is page table index, PTE is page table entry
    Pde *pgdir_entryp = &pgdir[PDX(va)];
    Pte *pgtable;

    if (*pgdir_entryp & PTE_V)
    {
        pgtable = (Pte *)KADDR(PTE_ADDR(*pgdir_entryp));
        return &pgtable[PTX(va)];
    }
    if (create) // 
    {
        pgtable = alloc(BY2PG, BY2PG, 1);
        if (!pgtable)
            return NULL;
        *pgdir_entryp = PADDR(pgtable) | PTE_V;
        return &pgtable[PTX(va)]; 
    }
    return NULL; 
}

/*Overview:
    Map [va, va+size) of virtual address space to physical [pa, pa+size) in the page
    table rooted at pgdir.
    Use permission bits `perm|PTE_V` for the entries.
    Use permission bits `perm` for the entries.

  Pre-Condition:
    Size is a multiple of BY2PG.*/
// pa is the physical address, va is the virtual address, size is the size of the segment, perm is the permission bits!!!
void boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm) 
{
    Pte *pgtable_entry;
    /* Step 1: Ensure size is rounded up to a multiple of BY2PG. */
    size = ROUND(size, BY2PG);

    /* Step 2: Map each page in the range [va, va+size) to [pa, pa+size). */
    u_long offset;
    for (offset = 0; offset < size; offset += BY2PG) {
        pgtable_entry = boot_pgdir_walk(pgdir, va + offset, 1); // Get the page table entry for this virtual address
        if (!pgtable_entry) {
            // I thought should do sth to avoid pgtable_entry is NULL, this is a simple attempt
            panic("boot_map_segment: cannot allocate page table\n");
        }
        // PET_ADDR is a macro defined in mmu.h, which gets the physical address form a page table entry
        *pgtable_entry = PTE_ADDR(pa + offset) | (perm | PTE_V);
    }
}

/* Overview:
    Set up two-level page table.

   Hint:
    You can get more details about `UPAGES` and `UENVS` in include/mmu.h. */
void mips_vm_init()
{
    extern char end[];
    extern int mCONTEXT;
    extern struct Env *envs;

    Pde *pgdir;
    u_int n;

    /* Step 1: Allocate a page for page directory(first level page table). */
    pgdir = alloc(BY2PG, BY2PG, 1);
    printf("to memory %x for struct page directory.\n", freemem);
    mCONTEXT = (int)pgdir;

    boot_pgdir = pgdir;

    /* Step 2: Allocate proper size of physical memory for global array `pages`,
     * for physical memory management. Then, map virtual address `UPAGES` to
     * physical address `pages` allocated before. For consideration of alignment,
     * you should round up the memory size before map. */
    pages = (struct Page *)alloc(npage * sizeof(struct Page), BY2PG, 1);
    printf("to memory %x for struct Pages.\n", freemem);
    n = ROUND(npage * sizeof(struct Page), BY2PG);
    boot_map_segment(pgdir, UPAGES, n, PADDR(pages), PTE_R);

    /* Step 3, Allocate proper size of physical memory for global array `envs`,
     * for process management. Then map the physical address to `UENVS`. */
    envs = (struct Env *)alloc(NENV * sizeof(struct Env), BY2PG, 1);
    n = ROUND(NENV * sizeof(struct Env), BY2PG);
    boot_map_segment(pgdir, UENVS, n, PADDR(envs), PTE_R);

    printf("pmap.c:\t mips vm init success\n");
}

/*Overview:
    Initialize page structure and memory free list.
    The `pages` array has one `struct Page` entry per physical page. Pages
    are reference counted, and free pages are kept on a linked list.
  Hint:
    Use `LIST_INSERT_HEAD` to insert something to list.*/
void page_init(void)
{
    /* Step 1: Initialize page_free_list. */
    LIST_INIT(&page_free_list);

    /* Step 2: Align `freemem` up to multiple of BY2PG. */
    // ROUND is a macro defined in mmu.h, which rounds up the value to the nearest multiple of the second argument.
    // BY2PG is page size, defined in mmu.h, BY for byte, 2 is unclear, and PG is page.
    freemem = ROUND(freemem, BY2PG);

    /* Step 3: Mark all memory below `freemem` as used (set `pp_ref` to 1). */
    // PADDR(freemem) is a macro defined in mmu.h, which converts a virtual address to a physical address.
    // P for physical, ADDR for address.
    u_long minFreePage = PPN(PADDR(freemem)); //
    u_long i;
    for (i = 0; i < minFreePage; i++)
    {
        pages[i].pp_ref = 1; // used
    }

    /* Step 4: Mark the other memory as free. */
    for (i = minFreePage; i < npage; i++)
    {
        pages[i].pp_ref = 0;                                   // free
        LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link); // to free list
    }
}

/*Overview:
    Allocates a physical page from free memory, and clear this page.

  Post-Condition:
    If failed to allocate a new page(out of memory(there's no free page)),
    return -E_NO_MEM.
    Else, set the address of allocated page to *pp, and returned 0.

  Note:
    Does NOT increment the reference count of the page - the caller must do
    these if necessary (either explicitly or via page_insert).

  Hint:
    Use LIST_FIRST and LIST_REMOVE defined in include/queue.h .*/
int page_alloc(struct Page **pp)
{
    struct Page *ppage_temp;

    /* Step 1: Get a page from free memory. If fails, return the error code.*/
    if (LIST_EMPTY(&page_free_list))
    {
        return -E_NO_MEM;
    }

    ppage_temp = LIST_FIRST(&page_free_list);
    LIST_REMOVE(ppage_temp, pp_link);

    /* Step 2: Initialize this page.
     * Hint: use `bzero`. */
    // page2kva is a func defined in pmap.h, kva means kernel virtual address
    // bzero is a function defined in init-checkpoint.c, which clears the memory, start from arg1(addr), set arg2(size) Byte to 0
    bzero(page2kva(ppage_temp), BY2PG);

    *pp = ppage_temp;

    return 0;
}

/*Overview:
    Release a page, mark it as free if it's `pp_ref` reaches 0.
  Hint:
    When to free a page, just insert it to the page_free_list.*/
void page_free(struct Page *pp)
{
    // pp_ref is the reference count of the page, it indicates how many virtual addresses refer to this page
    // why dont call it pp_ref_cnt, it will be more clear

    /* Step 1: If there's still virtual address refers to this page, do nothing. */
    if (pp->pp_ref > 0)
    {
        return;
    }

    /* Step 2: If the `pp_ref` reaches to 0, mark this page as free and return. */
    if (pp->pp_ref == 0)
    {
        LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
        return;
    }
    // ????? who wrote this????? the pp_ref is a u_short, so it can only be 0 or positive
    /* If the value of `pp_ref` less than 0, some error must occurred before,
     * so PANIC !!! */
    panic("cgh:pp->pp_ref is less than zero\n");
}

/*Overview:
    Given `pgdir`, a pointer to a page directory, pgdir_walk returns a pointer
    to the page table entry (with permission PTE_R|PTE_V) for virtual address 'va'.

  Pre-Condition:
    The `pgdir` should be two-level page table structure.

  Post-Condition:
    If we're out of memory, return -E_NO_MEM.
    Else, we get the page table entry successfully, store the value of page table
    entry to *ppte, and return 0, indicating success.

  Hint:
    We use a two-level pointer to store page table entry and return a state code to indicate
    whether this function execute successfully or not.
    This function have something in common with function `boot_pgdir_walk`.*/
int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) // walk means to traverse! stupid
{
    // Pde is u_long, which is a page directory entry, it is a pointer to a page table
    // Pte is u_long, which is a page table entry, it is a pointer to a physical page
    // PDX(va) is a macro defined in mmu.h, which cala the page directory index of the virtual address va
    // entryp' p means pointer, I thought
    // pgtable is a pointer to a page table
    // ppte is a pointer to a page table entry
    Pde *pgdir_entryp = &pgdir[PDX(va)];
    Pte *pgtable = NULL;
    struct Page *ppage = NULL;

    // PTE_V is a macro defined in mmu.h, which indicates the page table entry have valid bit set
    // PTE_ADDR is a macro defined in mmu.h, which gets the physical address from a page table entry
    // KADDR is a macro defined in mmu.h, which converts a physical address to a kernel virtual address
    if (*pgdir_entryp & PTE_V)
    {
        pgtable = (Pte *)KADDR(PTE_ADDR(*pgdir_entryp));
    }
    else if (create)
    {                                        // create is a 0/1flag, if it's 1, need to create a new page table
        if (page_alloc(&ppage) == -E_NO_MEM) // try to allocate
        {
            *ppte = NULL;
            return -E_NO_MEM;
        }
        ppage->pp_ref++;           // more virtual address refer to this page
        pgtable = page2kva(ppage); // get the kernel virtual address of this page
        // page2pa(ppage) is a function defined in pmap.h, which converts a page to a physical address
        *pgdir_entryp = page2pa(ppage) | PTE_V; // set the valid bit
    }
    else
    {
        *ppte = NULL;
        return 0;
    }
    // PTX(va) is a macro defined in mmu.h, which gets the page table index of the virtual address va
    *ppte = &pgtable[PTX(va)];
    return 0;
}

/*Overview:
    Map the physical page 'pp' at virtual address 'va'.
    The permissions (the low 12 bits) of the page table entry should be set to 'perm|PTE_V'.

  Post-Condition:
    Return 0 on success
    Return -E_NO_MEM, if page table couldn't be allocated

  Hint:
    If there is already a page mapped at `va`, call page_remove() to release this mapping.
    The `pp_ref` should be incremented if the insertion succeeds.*/
int
page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm)
{
    u_int PERM;
    Pte *pgtable_entry;
    PERM = perm | PTE_V; // remind: perm is the permission bit

    /* Step 1: Get corresponding page table entry. */
    pgdir_walk(pgdir, va, 0, &pgtable_entry); // after this call, pgtable_entry points to the page table entry for va

    // tlb_invalidate is a function which set the TLB entry of va to invalid
    // pa2page is a function which converts a physical address to a page
    if (pgtable_entry != 0 && (*pgtable_entry & PTE_V) != 0) { // if the page table entry is valid, in other words, pgdir_walk has been called and successfully got the page table entry
        if (pa2page(*pgtable_entry) != pp) { // if the page table entry is valid and the page is not the same as pp(the given one)
            page_remove(pgdir, va); // remove the old mapping
        } else  { // page table entry has already been set to pp, or , the page user want to insert is already mapped to va
            tlb_invalidate(pgdir, va);
            *pgtable_entry = (page2pa(pp) | PERM);  // make sure the authorization bits are set
            return 0;
        }
    }

    /* Step 2: Update TLB. */
    /* hint: use tlb_invalidate function */
    tlb_invalidate(pgdir, va);

    /* Step 3: Do check, re-get page table entry to validate the insertion. */
    /* Step 3.1 Check if the page can be insert, if can’t return -E_NO_MEM */
    if (pgdir_walk(pgdir, va, 1, &pgtable_entry)) { // create a new page table entry if it doesn't exist
        return -E_NO_MEM;
    }
    /* Step 3.2 Insert page and increment the pp_ref */
    *pgtable_entry = PTE_ADDR(page2pa(pp)) | PERM; 
    pp->pp_ref++;

    return 0;
}

/*Overview:
    Look up the Page that virtual address `va` map to.

  Post-Condition:
    Return a pointer to corresponding Page, and store it's page table entry to *ppte.
    If `va` doesn't mapped to any Page, return NULL.*/
struct Page *
page_lookup(Pde *pgdir, u_long va, Pte **ppte)
{
    struct Page *ppage;
    Pte *pte;

    /* Step 1: Get the page table entry. */
    pgdir_walk(pgdir, va, 0, &pte);

    /* Hint: Check if the page table entry doesn't exist or is not valid. */
    if (pte == 0)
    {
        return 0;
    }
    if ((*pte & PTE_V) == 0)
    {
        return 0; // the page is not in memory.
    }

    /* Step 2: Get the corresponding Page struct. */

    /* Hint: Use function `pa2page`, defined in include/pmap.h . */
    ppage = pa2page(*pte);
    if (ppte)
    {
        *ppte = pte;
    }

    return ppage;
}

// Overview:
// 	Decrease the `pp_ref` value of Page `*pp`, if `pp_ref` reaches to 0, free this page.
void page_decref(struct Page *pp)
{
    if (--pp->pp_ref == 0)
    {
        page_free(pp);
    }
}

// Overview:
// 	Unmaps the physical page at virtual address `va`.
void page_remove(Pde *pgdir, u_long va)
{
    Pte *pagetable_entry;
    struct Page *ppage;

    /* Step 1: Get the page table entry, and check if the page table entry is valid. */
    ppage = page_lookup(pgdir, va, &pagetable_entry);

    if (ppage == 0)
    {
        return;
    }

    /* Step 2: Decrease `pp_ref` and decide if it's necessary to free this page. */

    /* Hint: When there's no virtual address mapped to this page, release it. */
    ppage->pp_ref--;
    if (ppage->pp_ref == 0)
    {
        page_free(ppage);
    }

    /* Step 3: Update TLB. */
    *pagetable_entry = 0;
    tlb_invalidate(pgdir, va);
    return;
}

// Overview:
// 	Update TLB.
void tlb_invalidate(Pde *pgdir, u_long va)
{
    if (curenv)
    {
        tlb_out(PTE_ADDR(va) | GET_ENV_ASID(curenv->env_id));
    }
    else
    {
        tlb_out(PTE_ADDR(va));
    }
}

void physical_memory_manage_check(void)
{
    struct Page *pp, *pp0, *pp1, *pp2;
    struct Page_list fl;
    int *temp;

    // should be able to allocate three pages
    pp0 = pp1 = pp2 = 0;
    assert(page_alloc(&pp0) == 0);
    assert(page_alloc(&pp1) == 0);
    assert(page_alloc(&pp2) == 0);

    assert(pp0);
    assert(pp1 && pp1 != pp0);
    assert(pp2 && pp2 != pp1 && pp2 != pp0);

    // temporarily steal the rest of the free pages
    fl = page_free_list;
    // now this page_free list must be empty!!!!
    LIST_INIT(&page_free_list);
    // should be no free memory
    assert(page_alloc(&pp) == -E_NO_MEM);

    temp = (int *)page2kva(pp0);
    // write 1000 to pp0
    *temp = 1000;
    // free pp0
    page_free(pp0);
    printf("The number in address temp is %d\n", *temp);

    // alloc again
    assert(page_alloc(&pp0) == 0);
    assert(pp0);

    // pp0 should not change
    assert(temp == (int *)page2kva(pp0));
    // pp0 should be zero
    assert(*temp == 0);

    page_free_list = fl;
    page_free(pp0);
    page_free(pp1);
    page_free(pp2);
    struct Page_list test_free;
    struct Page *test_pages;
    test_pages = (struct Page *)alloc(10 * sizeof(struct Page), BY2PG, 1);
    LIST_INIT(&test_free);
    // LIST_FIRST(&test_free) = &test_pages[0];
    int i, j = 0;
    struct Page *p, *q;
    // test inert tail
    for (i = 0; i < 10; i++)
    {
        test_pages[i].pp_ref = i;
        // test_pages[i].pp_link=NULL;
        // printf("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
        LIST_INSERT_TAIL(&test_free, &test_pages[i], pp_link);
        // printf("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
    }
    p = LIST_FIRST(&test_free);
    int answer1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    assert(p != NULL);
    while (p != NULL)
    {
        // printf("%d %d\n",p->pp_ref,answer1[j]);
        assert(p->pp_ref == answer1[j++]);
        // printf("ptr: 0x%x v: %d\n",(p->pp_link).le_next,((p->pp_link).le_next)->pp_ref);
        p = LIST_NEXT(p, pp_link);
    }
    // insert_after test
    int answer2[] = {0, 1, 2, 3, 4, 20, 5, 6, 7, 8, 9};
    q = (struct Page *)alloc(sizeof(struct Page), BY2PG, 1);
    q->pp_ref = 20;

    // printf("---%d\n",test_pages[4].pp_ref);
    LIST_INSERT_AFTER(&test_pages[4], q, pp_link);
    // printf("---%d\n",LIST_NEXT(&test_pages[4],pp_link)->pp_ref);
    p = LIST_FIRST(&test_free);
    j = 0;
    // printf("into test\n");
    while (p != NULL)
    {
        //      printf("%d %d\n",p->pp_ref,answer2[j]);
        assert(p->pp_ref == answer2[j++]);
        p = LIST_NEXT(p, pp_link);
    }

    printf("physical_memory_manage_check() succeeded\n");
}

void page_check(void)
{
    struct Page *pp, *pp0, *pp1, *pp2;
    struct Page_list fl;

    // should be able to allocate three pages
    pp0 = pp1 = pp2 = 0;
    assert(page_alloc(&pp0) == 0);
    assert(page_alloc(&pp1) == 0);
    assert(page_alloc(&pp2) == 0);

    assert(pp0);
    assert(pp1 && pp1 != pp0);
    assert(pp2 && pp2 != pp1 && pp2 != pp0);

    // temporarily steal the rest of the free pages
    fl = page_free_list;
    // now this page_free list must be empty!!!!
    LIST_INIT(&page_free_list);

    // should be no free memory
    assert(page_alloc(&pp) == -E_NO_MEM);

    // there is no free memory, so we can't allocate a page table
    assert(page_insert(boot_pgdir, pp1, 0x0, 0) < 0);

    // free pp0 and try again: pp0 should be used for page table
    page_free(pp0);
    assert(page_insert(boot_pgdir, pp1, 0x0, 0) == 0);
    assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));

    printf("va2pa(boot_pgdir, 0x0) is %x\n", va2pa(boot_pgdir, 0x0));
    printf("page2pa(pp1) is %x\n", page2pa(pp1));

    assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
    assert(pp1->pp_ref == 1);

    // should be able to map pp2 at BY2PG because pp0 is already allocated for page table
    assert(page_insert(boot_pgdir, pp2, BY2PG, 0) == 0);
    assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
    assert(pp2->pp_ref == 1);

    // should be no free memory
    assert(page_alloc(&pp) == -E_NO_MEM);

    printf("start page_insert\n");
    // should be able to map pp2 at BY2PG because it's already there
    assert(page_insert(boot_pgdir, pp2, BY2PG, 0) == 0);
    assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp2));
    assert(pp2->pp_ref == 1);

    // pp2 should NOT be on the free list
    // could happen in ref counts are handled sloppily in page_insert
    assert(page_alloc(&pp) == -E_NO_MEM);

    // should not be able to map at PDMAP because need free page for page table
    assert(page_insert(boot_pgdir, pp0, PDMAP, 0) < 0);

    // insert pp1 at BY2PG (replacing pp2)
    assert(page_insert(boot_pgdir, pp1, BY2PG, 0) == 0);

    // should have pp1 at both 0 and BY2PG, pp2 nowhere, ...
    assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
    assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
    // ... and ref counts should reflect this
    assert(pp1->pp_ref == 2);
    printf("pp2->pp_ref %d\n", pp2->pp_ref);
    assert(pp2->pp_ref == 0);
    printf("end page_insert\n");

    // pp2 should be returned by page_alloc
    assert(page_alloc(&pp) == 0 && pp == pp2);

    // unmapping pp1 at 0 should keep pp1 at BY2PG
    page_remove(boot_pgdir, 0x0);
    assert(va2pa(boot_pgdir, 0x0) == ~0);
    assert(va2pa(boot_pgdir, BY2PG) == page2pa(pp1));
    assert(pp1->pp_ref == 1);
    assert(pp2->pp_ref == 0);

    // unmapping pp1 at BY2PG should free it
    page_remove(boot_pgdir, BY2PG);
    assert(va2pa(boot_pgdir, 0x0) == ~0);
    assert(va2pa(boot_pgdir, BY2PG) == ~0);
    assert(pp1->pp_ref == 0);
    assert(pp2->pp_ref == 0);

    // so it should be returned by page_alloc
    assert(page_alloc(&pp) == 0 && pp == pp1);

    // should be no free memory
    assert(page_alloc(&pp) == -E_NO_MEM);

    // forcibly take pp0 back
    assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
    boot_pgdir[0] = 0;
    assert(pp0->pp_ref == 1);
    pp0->pp_ref = 0;

    // give free list back
    page_free_list = fl;

    // free the pages we took
    page_free(pp0);
    page_free(pp1);
    page_free(pp2);

    printf("page_check() succeeded!\n");
}

void pageout(int va, int context)
{
    u_long r;
    struct Page *p = NULL;

    if (context < 0x80000000)
    {
        panic("tlb refill and alloc error!");
    }

    if ((va > 0x7f400000) && (va < 0x7f800000))
    {
        panic(">>>>>>>>>>>>>>>>>>>>>>it's env's zone");
    }

    if (va < 0x10000)
    {
        panic("^^^^^^TOO LOW^^^^^^^^^");
    }

    if ((r = page_alloc(&p)) < 0)
    {
        panic("page alloc error!");
    }

    p->pp_ref++;

    page_insert((Pde *)context, p, VA2PFN(va), PTE_R);
    printf("pageout:\t@@@___0x%x___@@@  ins a page \n", va);
}
