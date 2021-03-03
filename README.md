# molecules_Cthreads

Reactia care trebuie simulata este:

FeCl3 + 3NaOH ---> Fe(OH)3 + 3NaCl

Deci initial se creeaza threadurile care simuleaza atomii de Fe, Cl, Na, O, H
Acesti atomi vor forma moleculele FeCl3 + 3NaOH care vor intra in reactie si formeaza moleculele: Fe(OH)3 + 3NaCl.
Conditia de terminare: daca s-au format N molecule de NaCl

N se specifica ca si argument din linia de comanda.

Apelul programului:
./t2  N 
./t2 -o N     daca se doreste ca atomii sa intre in molecule in ordinea in care au aparut (au fost creati)
