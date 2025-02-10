# Allocateur mémoire

L'objectif de ce projet était de produire une fonction similaire à la fonction malloc de la libc.

## Ce qui est implémenté

La stratégie utilisée pour l'allocation de la mémoire diffère en fonction de la taille demandée :
- **Grandes allocations** ( > 128 ko) : Après l'appel système mmap, le bloc de mémoire est marqué au début et à la fin de la zone enfin de détecter les débordements.
- **Petites allocations** ( < 64 octets) : Gérées via un pool (une liste simplement chaînée) de chunks (des morceaux de taille identique) disponibles. Lorsqu’il n’a plus de chunk, une demande est faite au système, avec un mécanisme de recursive doubling pour amortir le coût.
- **Moyennes allocations** : Algorithme du buddy, qui essaie de conserver des zones libres contiguës les plus grandes possibles. Cet algorithme s’écarte un peu de celui de GNU malloc().

## Tester le programme

Toutes les commandes ci-dessous sont à exécuter dans le répertoire **./build**.

### Compilation

```
cmake ../
make
```

### Exécution

```
./memshell
```
Vous vous trouverez alors dans le shell permettant de tester l'allocation. Tapez **help** pour obtenir la liste des commandes possibles.

### Lancement des tests

Pour un très gros résumé :
```
make test
```

Pour quelque chose de plus détaillé :
```
make check
```
