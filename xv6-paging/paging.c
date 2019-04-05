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
	uint blk = balloc_page(ROOTDEV);
	//va = pte_to_vaddr(pte);
	uint va = P2V(PTE_ADDR(*pte));
	write_page_to_disk(ROOTDEV, (char*)va, blk);
	*pte = *pte & ~(PTE_P);
	blk = blk<<12;
	*pte = blk & ~(PTE_SWP);
	kfree(V2P(va));

}

/* Select a victim and swap the contents to the disk.
 */
int
swap_page(pde_t *pgdir)
{
	pte_t *pte = select_a_victim(pgdir);
	if(*pte == 0){
		clearaccessbit(pgdir);
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
void
map_address(pde_t *pgdir, uint addr)
{
	// blk = getswappedblk
	// allocuvm
	// switchuvm
	// if blk was not -1, read_page_from_disk
	// bfree_page
	panic("map_address is not implemented");
}

/* page fault handler */
void
handle_pgfault()
{
	unsigned addr;
	struct proc *curproc = myproc();

	asm volatile ("movl %%cr2, %0 \n\t" : "=r" (addr));
	addr &= ~0xfff;
	map_address(curproc->pgdir, addr);
}
