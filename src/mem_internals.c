/******************************************************
 * Copyright Grégory Mounié 2018-2022                 *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

unsigned long knuth_mmix_one_round(unsigned long in)
{
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k)
{
    uint64_t* ptr_to_mem = (uint64_t*)ptr;
    uint64_t magic_number = (uint64_t) ((knuth_mmix_one_round((unsigned long)ptr) & ~(0b11UL)) | k) ;

    // marquage début
    *ptr_to_mem = (uint64_t) size;
    *(ptr_to_mem + 1) = magic_number;    
    
    // marquage fin
    *(ptr_to_mem + (size/8) + (size%8 == 0 ? 0 : 1) - 2) = magic_number;
    *(ptr_to_mem + (size/8) + (size%8 == 0 ? 0 : 1) - 1) = (uint64_t) size;

    return (void *)(ptr_to_mem + 2);
}

Alloc
mark_check_and_get_alloc(void *ptr)
{
    uint64_t* ptr_to_block = ((uint64_t*)ptr - 2);

    uint64_t size = *ptr_to_block;
    uint64_t magic_number = *(ptr_to_block + 1);

    MemKind memkin = magic_number & 0b11;

    uint64_t th_magic_number = (uint64_t) ((knuth_mmix_one_round((unsigned long)(ptr_to_block)) & ~(0b11UL)) | memkin) ;
    assert(magic_number == th_magic_number);

    //taille deb == taille fin
    assert(size == *(ptr_to_block + (size/8) + (size%8 == 0 ? 0 : 1) - 1));
    //magic deb == magic fin
    assert(magic_number == (*((ptr_to_block + (size/8)) + (size%8 == 0 ? 0 : 1) - 2)));

    Alloc a = {
        (void*)(ptr_to_block),
        memkin,
        size
    };
    return a;
}


unsigned long
mem_realloc_small() {
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
			   size,
			   PROT_READ | PROT_WRITE | PROT_EXEC,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   -1,
			   0);
    if (arena.chunkpool == MAP_FAILED)
	handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium() {
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert( size == (1UL << indice));
    arena.TZL[indice] = mmap(0,
			     size*2, // twice the size to allign
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     -1,
			     0);
    if (arena.TZL[indice] == MAP_FAILED)
	handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}


// used for test in buddy algo
unsigned int
nb_TZL_entries() {
    int nb = 0;
    
    for(int i=0; i < TZL_SIZE; i++)
	if ( arena.TZL[i] )
	    nb ++;

    return nb;
}
