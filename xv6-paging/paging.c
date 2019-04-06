#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "paging.h"
#include "fs.h"

static pte_t * walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

static int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;

  a = (char*)PGROUNDDOWN((uint)va);
  last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
  for(;;){
    if((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if(*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}


/* Allocate eight consecutive disk blocks.
 * Save the content of the physical page in the pte
 * to the disk blocks and save the block-id into the
 * pte.
 */
void
swap_page_from_pte(pte_t *pte)
{
	//blk = balloc_page
	//va = pte_to_vaddr
	//write_page_to_disk  dev=ROOTDEV, va, blk
	//mark pte not present
	//store blk in higher 20 bits
	//use one bit in lower 12 bits to mark pte as swapped
	//kfree (va)
	//cprintf("swap\n");
	uint blk = balloc_page(ROOTDEV);
	//numallocblocks+=1;
	//va = pte_to_vaddr(pte);
	//cprintf("done\n");
	uint pa = PTE_ADDR(*pte);
	uint* va = P2V(pa);
	//cprintf("write_page_to_disk\n");
	write_page_to_disk(ROOTDEV, (char*)va, blk);
	//cprintf("written\n");
	*pte = *pte & ~(PTE_P);
	//cprintf("written2\n");
	blk = blk<<12;
	*pte = blk | (PTE_SWP);
	//cprintf("written3\n");
	kfree((char*)va);
	//cprintf("written4\n");
}

/* Select a victim and swap the contents to the disk.
 */
int
swap_page(pde_t *pgdir)
{
	//cprintf("swap2");
	pte_t *pte = select_a_victim(pgdir);
	if(pte == 0){
		//cprintf("clear\n");
		clearaccessbit(pgdir);
		//cprintf("select_a_victim\n");
		pte = select_a_victim(pgdir);
	}
	swap_page_from_pte(pte);
	return 1;
}

/* Map a physical page to the virtual address addr.
 * If the page table entry points to a swapped block
 * restore the content of the page from the swapped
 * block and free the swapped block.
 */
void map_address(pde_t *pgdir, uint addr)
{
	// blk = getswappedblk
	// allocuvm
	// switchuvm
	// if blk was not -1, read_page_from_disk
	// bfree_page
	//check if your va is corresponding to a pte which is already
	//if already swapped, allocate pm to it with kalloc
	//if successful positive, or 0
	//swap page out
	//int blk = getswappedblk(pgdir,addr);
	//int newsize = allocuvm(pgdir,)
	char* mem;
	mem = kalloc();
	if(mem == 0){
		//cprintf("\n%s\n","hello");
		swap_page(pgdir);
		mem = kalloc();
		memset(mem, 0, PGSIZE);
		//mappages(pgdir, (char*)addr, PGSIZE, V2P(mem), PTE_W|PTE_U);
	}
	//cprintf("written5\n");
	int blk = getswappedblk(pgdir, addr);
	//cprintf("%d\nhuup", blk);
	char pg[4096] = "";
	//cprintf("written6\n");
	if(blk!=-1){
		read_page_from_disk(ROOTDEV,pg,blk);
		//cprintf("written7\n");
		memmove(mem, pg, PGSIZE);
		//cprintf("written8\n");
		//cprintf("%d\nhuu", blk);
		bfree_page(ROOTDEV, blk);
		//numallocblocks-=1;
		//cprintf("written9\n");
	}

	mappages(pgdir, (char*)addr, PGSIZE, V2P(mem), PTE_W|PTE_U);
	//cprintf("written10\n");s

}

/* page fault handler */
void
handle_pgfault()
{
	unsigned addr;
	struct proc *curproc = myproc();
	//cprintf("pgfault\n");
	asm volatile ("movl %%cr2, %0 \n\t" : "=r" (addr));
	addr &= ~0xfff;
	map_address(curproc->pgdir, addr);
}
