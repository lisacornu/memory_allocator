/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

#define SIZE_BLOCK_SM_ALLOC 96

void *
emalloc_small(unsigned long size)
{
    // si la liste est vide : non initialisée ou vidée
    if (arena.chunkpool == NULL) {
        //récupération bloc de mémoire
        uint64_t new_size = mem_realloc_small();
        void *ptr_debut = arena.chunkpool;
        void **ptr_en_cours = ptr_debut;
        
        // initialisation liste chainée
        for (uint64_t i = 0; i < (new_size/SIZE_BLOCK_SM_ALLOC) - 1; i++) {
            *ptr_en_cours = (void **) (ptr_en_cours + 12);
            ptr_en_cours = *ptr_en_cours;
        }
        //fin de liste pointe sur NULL
        *ptr_en_cours = NULL;
    }
    // récupération premier élément, arenan.chunkpool pointe alors sur le suivant (le prochain qui est libre)
    void *ptr = arena.chunkpool;
    arena.chunkpool = *((void **) arena.chunkpool);
    return mark_memarea_and_get_user_ptr(ptr, SIZE_BLOCK_SM_ALLOC, 0);
}

void efree_small(Alloc a) {
    // Bloc libéré retourne en tête de liste
    a.ptr = arena.chunkpool;
    arena.chunkpool = a.ptr;
}
