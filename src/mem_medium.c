/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

unsigned int puiss2(unsigned long size) {
    unsigned int p=0;
    size = size -1; // allocation start in 0
    while(size) {  // get the largest bit
	p++;
	size >>= 1;
    }
    if (size > (1 << p))
	p++;
    return p;
}


void*
emalloc_medium_rec(unsigned long size) //fonction récursive pour l'allocation
{   
    //refill d'espace à allouer
    if (size == FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant) {
        mem_realloc_medium();
    }

    if (arena.TZL[size] == NULL) {
        //on cherche récursivement un espace libre de taille supérieur pour pouvoir le subdiviser
        void **ptr_grand_frere = emalloc_medium_rec(size + 1);

        //calcule du buddy dans cet zone mémoire plus grande
        void **ptr_buddy = (void **) ((unsigned long) ptr_grand_frere ^ (1<<size));
        
        //on met à jour arena.TZL[size+1] et on insère ptr et son buddy dans arena.TZL[size]
        arena.TZL[size+1] = *ptr_grand_frere;
        *ptr_buddy = arena.TZL[size];
        arena.TZL[size] = ptr_grand_frere;
        *ptr_grand_frere = ptr_buddy;
    }
    return arena.TZL[size];
}

void *
emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);

    //on rajoute les 32 octets de marquage
    size += 32;

    unsigned int taille_min = puiss2(size);
    void *ptr = emalloc_medium_rec(taille_min);

    //on enlève virutellement ptr de arena.TZL[taille_min]
    arena.TZL[taille_min] = *(void **)ptr;

    return mark_memarea_and_get_user_ptr(ptr, 1<<taille_min, 1);
}


//renvoie l'adresse la plus petite du couple de buddies
void **get_original_pointer(void **ptr, void **ptr_buddy)
{
    if ((uint64_t) ptr < (uint64_t) ptr_buddy) {
        return ptr;
    } else {
        return ptr_buddy;
    }
}

uint8_t efree_medium_rec(void **ptr, unsigned long size) //fonction récursive pour le free
{
    //on calcule l'adresse du buddy
    void **ptr_buddy = (void **)((unsigned long) ptr ^ (1<<size));
    //poiteur qui va chercher le buddy de ptr
    void **verif_ptr = arena.TZL[size];
    //poiteur qui va retenir l'élément précédant le buddy
    void **prec_verif_ptr = &(arena.TZL[size]);

    //on cherche le buddy dans arena.TZL[size]
    while (verif_ptr != NULL && verif_ptr != ptr_buddy) {
        prec_verif_ptr = verif_ptr;
        verif_ptr = *(void **) verif_ptr;
    }

    //si le buddy est bien dans arena.TZL[size], on le met à jour à NULL et on appelle récursivement la fonction au niveau d'au dessus
    if (verif_ptr == ptr_buddy) { 
        *prec_verif_ptr = *ptr_buddy;
        *ptr_buddy = NULL;

        //on appelle récursivement la fonction sur le poiteur original du couple (ptr, buddy)
        void **original_ptr = get_original_pointer(ptr, ptr_buddy);

        //vaut un quand on est remontés aussi haut que possible
        uint8_t max_rec = efree_medium_rec(original_ptr, size+1); 

        if (max_rec) {
            //on insère le pointeur "original" au début de arena.TZL[size+1]
            *original_ptr = arena.TZL[size+1];
            arena.TZL[size+1] = original_ptr; 
        }
        return 0;
    }
    return 1;
}

void efree_medium(Alloc a) {

    unsigned long size = a.size;
    unsigned int ind = puiss2(size);
    void **ptr = (void **) a.ptr;
    *ptr = NULL;

    //on regarde si le buddy se trouve dans arena.TZL[ind]
    uint8_t ok_or_not = efree_medium_rec(ptr, ind);

    //s'il n'y est pas, on l'y met
    if(ok_or_not) {
        *ptr = arena.TZL[ind];
        arena.TZL[ind] = ptr;
    }
}


