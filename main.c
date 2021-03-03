#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define max(a, b) (a>b)?a:b
#define min(a, b) (a<b)?a:b

sem_t *bufferMutex;
void *buffer;
unsigned int *writeCount;

///Required structures:
typedef struct _creare {//SWOOSH
    char tipAtom[4];
    unsigned int idAtom;
    time_t timp;
} creare;

typedef struct _formare {//maybe
    char tipMolecula[16];
    char tipAtom[4];
    unsigned int nrMolecula;
    unsigned int idAtom;
    time_t timp;
} formare;

typedef struct _reactie {//SWOOOSH
    char tipReactie[64];
    char tipMolecula[16];
    unsigned int nrReactie;
    unsigned int nrMolecula;
    time_t timp;
} reactie;

typedef struct _terminareAtom {
    char tipAtom[4];
    unsigned int idAtom;
    time_t timp;
} terminareAtom;

typedef struct _terminareMolecula {
    char tipMolecula[16];
    unsigned int nrMolecula;
    time_t timp;
} terminareMolecula;

typedef struct _conditieFinal {
    time_t timp;
} conditieFinal;

///Define a structure in order to store an unlimited number of threads
typedef struct _node {
    struct _node *next;
    pthread_t *atom;
} node;

///Reaction stuff
sem_t *reactionBarrier;
sem_t *naClSem;
sem_t *feCl3Sem;
sem_t *naOHSem;
unsigned int *naClMoleculeCount;
unsigned int *naClMoleculeMax;

///FeCl3 stuff
sem_t *feCl3Barrier;
sem_t *feSem;
sem_t *clSem;

//unsigned int *feCl3MoleculeMax;
unsigned int *feCl3MoleculeCount;
unsigned int *feCount;
unsigned int *clCount;


///NaOH stuff
sem_t *naOHBarrier;
sem_t *naSem;
sem_t *oSem;
sem_t *hSem;

//unsigned int *naOHMoleculeMax;
unsigned int *naOHMoleculeCount;
unsigned int *naCount;
unsigned int *oCount;
unsigned int *hCount;

/**
* Reaction takes place
*/
void reaction() {
    reactie r;
    strcpy(r.tipReactie, "FeCl3 + 3NaOH ---> Fe(OH)3 + 3NaCl");
    strcpy(r.tipMolecula, "NaCl");
    r.timp = time(NULL);

    sem_wait(naClSem);
    (*naClMoleculeCount) += 3;
    r.nrReactie = (*naClMoleculeCount) / 3;
    r.nrMolecula = (*naClMoleculeCount);
    sem_post(naClSem);

    sem_wait(bufferMutex);
    memcpy(buffer + (*writeCount), &r, sizeof(reactie));
    (*writeCount) += sizeof(reactie);
    sem_post(bufferMutex);
}

/**
* FeCl3 is created.
*/
void bondFeCl3(char tipAtom[], unsigned int idAtom) {
    //Bond FeCl3
    //initializare structura
    formare f;
    strcpy(f.tipMolecula, "FeCl3");
    strcpy(f.tipAtom, tipAtom);
    f.idAtom = idAtom;
    f.timp = time(NULL);

    terminareMolecula tM;
    strcpy(tM.tipMolecula, "FeCl3");

    sem_wait(reactionBarrier);
    f.nrMolecula = (*feCl3MoleculeCount) + 1;   //asta trb dupa bariera pt ca e scris de mai multe threaduri
    tM.nrMolecula = (*feCl3MoleculeCount) + 1;  //asta trb dupa bariera pt ca e scris de mai multe threaduri
    //Check if reaction is possible
    if((*feCl3MoleculeCount) >= 0 && (*naOHMoleculeCount) >= 3) {
        *naOHMoleculeCount -= 3;

        sem_post(naOHSem);
        sem_post(naOHSem);
        sem_post(naOHSem);
        sem_post(reactionBarrier);

        reaction();
    } else {
        (*feCl3MoleculeCount)++;
        sem_post(reactionBarrier);
        sem_wait(feCl3Sem);
    }

    //scriere in buffer
    tM.timp = time(NULL);
    sem_wait(bufferMutex);
    memmove(buffer + (*writeCount), &f, sizeof(formare));
    (*writeCount) += sizeof(formare);
    memmove(buffer + (*writeCount), &tM, sizeof(terminareMolecula));
    (*writeCount) += sizeof(terminareMolecula);
    sem_post(bufferMutex);
}

/**
* Ferrum atom becomes active for bond.
*/
void *fe() {
    creare c;
    strcpy(c.tipAtom, "Fe");
    c.timp = time(NULL);

    terminareAtom tA;
    strcpy(tA.tipAtom, "Fe");

    sem_wait(feCl3Barrier);

    c.idAtom = *feCount;     //trb sub bariera pt ca e scris de mai multe threaduri
    tA.idAtom = *feCount;    //trb sub bariera pt ca e scris de mai multe threaduri
    //If we have three chlorine atoms, adding a fetrium atom will make the bondFeCl3 possible;
    if((*clCount) >= 3) {
        (*clCount) -= 3;
        sem_post(clSem);
        sem_post(clSem);
        sem_post(clSem);
        sem_post(feCl3Barrier);

        bondFeCl3("Fe", *feCount);
    //If we don't, add a fetrium atom;
    } else {
        (*feCount)++;
        sem_post(feCl3Barrier);
        sem_wait(feSem);
    }

    //write terminare atom
    tA.timp = time(NULL);
    sem_wait(bufferMutex);
    memmove(buffer + (*writeCount), &c, sizeof(creare));
    (*writeCount) += sizeof(creare);
    memmove(buffer+(*writeCount), &tA, sizeof(terminareAtom));
    (*writeCount)+=sizeof(terminareAtom);
    sem_post(bufferMutex);
}

/**
* Chlorine becomes active for bondFeCl3ing.
*/
void *cl() {
    creare c;
    strcpy(c.tipAtom, "Cl");
    c.timp = time(NULL);

    terminareAtom tA;
    strcpy(tA.tipAtom, "Cl");

    sem_wait(feCl3Barrier);

    c.idAtom = *clCount;     //trb sub bariera pt ca e scris de mai multe threaduri
    tA.idAtom = *clCount;    //trb sub bariera pt ca e scris de mai multe threaduri
    //If we have a fetrium atom and two chlorine atoms, adding a chlorine atom will make the bond FeCl3 possible;
    if((*feCount) >= 1 && (*clCount) >= 2) {
        (*feCount) -= 1;
        (*clCount) -= 2;
        sem_post(feSem);
        sem_post(clSem);
        sem_post(clSem);
        sem_post(feCl3Barrier);

        bondFeCl3("Cl", *clCount);
    //If we don't, add a chlorine atom;
    } else {
        (*clCount)++;
        sem_post(feCl3Barrier);
        sem_wait(clSem);
    }

    //write terminare atom
    tA.timp = time(NULL);
    sem_wait(bufferMutex);
    memmove(buffer + (*writeCount), &c, sizeof(creare));
    (*writeCount) += sizeof(creare);
    memmove(buffer+(*writeCount), &tA, sizeof(terminareAtom));
    (*writeCount)+=sizeof(terminareAtom);
    sem_post(bufferMutex);
}

/**
* Make new atom available to bond FeCl3.
*/
void *newAtomFeOrCl(void *linkedList) {
    //Keep adding new threads until the condition is met;
    for(node *n=(node *)linkedList;; n=n->next) {
        usleep(10);
        //Initialise next thread and allocate memory for next node;
        n->atom = (pthread_t *)malloc(sizeof(pthread_t));
        if(pthread_create(n->atom, NULL, (rand() > RAND_MAX / 4)?(&cl):(&fe), (void *)NULL) != 0) {
            perror("Could not create new Fe or Cl atom!");
        }
        n->next = (node *)malloc(sizeof(node));

        //Check if molecule target number is reached and stop creating threads if yes;
        sem_wait(naClSem);
        if((*naClMoleculeMax)<=(*naClMoleculeCount)) {
            sem_post(naClSem);

            conditieFinal cF;
            cF.timp = time(NULL);

            sem_wait(bufferMutex);
            memmove(buffer + (*writeCount), &cF, sizeof(conditieFinal));
            (*writeCount) += sizeof(conditieFinal);
            sem_post(bufferMutex);

            break;
        }
        sem_post(naClSem);
    }
}

void *generateFeCl3() {
    ///FOR THE ATOM THREADS
    //Initialise feCl3Barrier semaphore
    feCl3Barrier = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(feCl3Barrier, 1, 1);

    //Initalise fetrium semaphore and counter
    feSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(feSem, 1, 0);
    feCount = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *feCount = 0;

    //Initialise clorium semaphore and counter
    clSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(clSem, 1, 0);
    clCount = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *clCount = 0;

    //Initialise molecule count and semaphore(also stopping condition) and condition semaphore
    feCl3Sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(feCl3Sem, 1, 0);
    feCl3MoleculeCount = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *feCl3MoleculeCount = 0;

    ///FOR ATOM CREATOR THREAD
    //Initialise rand function
    srand(time(NULL));

    //Define list of threads(since we have no thread number limit we use lists)
    node *linkedList = (node *)malloc(sizeof(node));

    //Define and create the atom creator thread
    pthread_t threadCreator;
    if(pthread_create(&threadCreator, NULL, &newAtomFeOrCl, (void *)linkedList) != 0) {
        perror("Could not create the thread creator!");
    }


    ///FREE AND CLOSE EVERYTHING AFTER CONDITION IS MET
    //Asteptam pafe
    pthread_join(threadCreator, NULL);

    //Free struct list
    node *q = linkedList;
    pthread_cancel(*(q->atom));
    free(q->atom);
    for(node *p=q->next; p->next!=NULL; p=p->next) {
        //pthread_cancel(*(p->atom));
        free(p->atom);
        free(q);
        q=p;
    }
    free(q);

    //Unmap shared memory
    munmap(feCl3Barrier, sizeof(sem_t));
    munmap(feSem, sizeof(sem_t));
    munmap(feCount, sizeof(unsigned int));

    munmap(clSem, sizeof(sem_t));
    munmap(clCount, sizeof(unsigned int));

    munmap(feCl3Sem, sizeof(sem_t));
    munmap(feCl3MoleculeCount, sizeof(unsigned int));
}

/**
* NaOH is created.(poate da seg fault pt ca se acceseaza din alt proces ce e mapat de alt proces);
*/
void bondNaOH(char tipAtom[], unsigned int idAtom) {
    //Bond NaOH
    formare f;
    strcpy(f.tipMolecula, "NaOH");
    strcpy(f.tipAtom, tipAtom);
    f.idAtom = idAtom;
    f.timp = time(NULL);

    terminareMolecula tM;
    strcpy(tM.tipMolecula, "NaOH");

    sem_wait(reactionBarrier);
    f.nrMolecula = (*naOHMoleculeCount) + 1;   //asta trb dupa bariera pt ca e scris de mai multe threaduri
    tM.nrMolecula = (*naOHMoleculeCount) + 1;  //asta trb dupa bariera pt ca e scris de mai multe threaduri
    //Check if reaction is possible
    if((*feCl3MoleculeCount) >= 1 && (*naOHMoleculeCount) >= 2) {
        (*feCl3MoleculeCount) -= 1;
        (*naOHMoleculeCount) -= 2;

        sem_post(feCl3Sem);
        sem_post(naOHSem);
        sem_post(naOHSem);
        sem_post(reactionBarrier);

        reaction();
    } else {
        (*naOHMoleculeCount)++;
        sem_post(reactionBarrier);
        sem_wait(naOHSem);
    }

    //scriere in buffer
    tM.timp = time(NULL);
    sem_wait(bufferMutex);
    memmove(buffer + (*writeCount), &f, sizeof(formare));
    (*writeCount) += sizeof(formare);
    memmove(buffer + (*writeCount), &tM, sizeof(terminareMolecula));
    (*writeCount) += sizeof(terminareMolecula);
    sem_post(bufferMutex);
}


/**
* Natrium atom becomes active for bond.
*/
void *na() {
    creare c;
    strcpy(c.tipAtom, "Na");
    c.timp = time(NULL);

    terminareAtom tA;
    strcpy(tA.tipAtom, "Na");

    sem_wait(naOHBarrier);
    //If we have a chlorine atom, adding a fetrium atom will make the bondFeCl3 possible;
    c.idAtom = *naCount;     //trb sub bariera pt ca e scris de mai multe threaduri
    tA.idAtom = *naCount;    //trb sub bariera pt ca e scris de mai multe threaduri
    if((*oCount) >= 1 && (*hCount) >= 1) {
        (*oCount) -= 1;
        (*hCount) -= 1;
        sem_post(oSem);
        sem_post(hSem);
        sem_post(naOHBarrier);

        bondNaOH("Na", *naCount);
    //If we don't, add a fetrium atom;
    } else {
        (*naCount)++;
        sem_post(naOHBarrier);
        sem_wait(naSem);
    }

    //write terminare atom
    tA.timp = time(NULL);
    sem_wait(bufferMutex);
    memmove(buffer + (*writeCount), &c, sizeof(creare));
    (*writeCount) += sizeof(creare);
    memmove(buffer+(*writeCount), &tA, sizeof(terminareAtom));
    (*writeCount)+=sizeof(terminareAtom);
    sem_post(bufferMutex);
}

/**
* Oxygen atom becomes active for bond.
*/
void *o() {
    creare c;
    strcpy(c.tipAtom, "O");
    c.timp = time(NULL);

    terminareAtom tA;
    strcpy(tA.tipAtom, "O");

    sem_wait(naOHBarrier);
    //If we have a chlorine atom, adding a fetrium atom will make the bondFeCl3 possible;
    c.idAtom = *oCount;     //trb sub bariera pt ca e scris de mai multe threaduri
    tA.idAtom = *oCount;    //trb sub bariera pt ca e scris de mai multe threaduri
    if((*naCount) >= 1 && (*hCount) >= 1) {
        (*naCount) -= 1;
        (*hCount) -= 1;
        sem_post(naSem);
        sem_post(hSem);
        sem_post(naOHBarrier);

        bondNaOH("O", *oCount);
    //If we don't, add a fetrium atom;
    } else {
        (*oCount)++;
        sem_post(naOHBarrier);
        sem_wait(oSem);
    }

    //write terminare atom
    tA.timp = time(NULL);
    sem_wait(bufferMutex);
    memmove(buffer + (*writeCount), &c, sizeof(creare));
    (*writeCount) += sizeof(creare);
    memmove(buffer+(*writeCount), &tA, sizeof(terminareAtom));
    (*writeCount)+=sizeof(terminareAtom);
    sem_post(bufferMutex);
}

/**
* Hydrogen atom becomes active for bond.
*/
void *h() {
    creare c;
    strcpy(c.tipAtom, "H");
    c.timp = time(NULL);

    terminareAtom tA;
    strcpy(tA.tipAtom, "H");

    sem_wait(naOHBarrier);
    //If we have a chlorine atom, adding a fetrium atom will make the bondFeCl3 possible;
    c.idAtom = *hCount;     //trb sub bariera pt ca e scris de mai multe threaduri
    tA.idAtom = *hCount;    //trb sub bariera pt ca e scris de mai multe threaduri
    if((*naCount) >= 1 && (*oCount) >= 1) {
        (*naCount) -= 1;
        (*oCount) -= 1;
        sem_post(naSem);
        sem_post(oSem);
        sem_post(naOHBarrier);

        bondNaOH("H", *hCount);
    //If we don't, add a fetrium atom;
    } else {
        (*hCount)++;
        sem_post(naOHBarrier);
        sem_wait(hSem);
    }

    //write terminare atom
    tA.timp = time(NULL);
    sem_wait(bufferMutex);
    memmove(buffer + (*writeCount), &c, sizeof(creare));
    (*writeCount) += sizeof(creare);
    memmove(buffer+(*writeCount), &tA, sizeof(terminareAtom));
    (*writeCount)+=sizeof(terminareAtom);
    sem_post(bufferMutex);
}

/**
* Make new atom available to bond NaOH.
*/
void *newAtomNaOrOOrH(void *linkedList) {
    //Keep adding new threads until the condition is met;
    for(node *n=(node *)linkedList;; n=n->next) {
        usleep(10);
        //Initialise next thread and allocate memory for next node;
        n->atom = (pthread_t *)malloc(sizeof(pthread_t));
        if(pthread_create(n->atom, NULL, (rand() > RAND_MAX / 3)?((rand() > RAND_MAX / 2)?(&na):(&o)):(&h), (void *)NULL) != 0) {
            perror("Could not create new Na or O or H atom!");
        }
        n->next = (node *)malloc(sizeof(node));

        //Check if molecule target number is reached and stop creating threads if reached;
        sem_wait(naClSem);
        if((*naClMoleculeMax)<=(*naClMoleculeCount)) {
            sem_post(naClSem);

            conditieFinal cF;
            cF.timp = time(NULL);

            sem_wait(bufferMutex);
            memmove(buffer + (*writeCount), &cF, sizeof(conditieFinal));
            (*writeCount) += sizeof(conditieFinal);
            sem_post(bufferMutex);

            break;
        }
        sem_post(naClSem);
    }
}

void *generateNaOH() {
    ///FOR THE ATOM THREADS
    //Initialise naOHBarrier semaphore
    naOHBarrier = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(naOHBarrier, 1, 1);

    //Initalise natrium semaphore and counter
    naSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(naSem, 1, 0);
    naCount = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *naCount = 0;

    //Initialise oxygen semaphore and counter
    oSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(oSem, 1, 0);
    oCount = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *oCount = 0;

    //Initialise hydrogen semaphore and counter
    hSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(hSem, 1, 0);
    hCount = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *hCount = 0;

    //Initialise molecule count and semaphore(also stopping condition) and condition semaphore
    naOHSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(naOHSem, 1, 0);
    naOHMoleculeCount = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *naOHMoleculeCount = 0;




    ///FOR ATOM CREATOR THREAD
    //Initialise rand function
    srand(time(NULL));

    //Define list of threads(since we have no thread number limit we use lists)
    node *linkedList = (node *)malloc(sizeof(node));

    //Define and create the atom creator thread
    pthread_t threadCreator;
    if(pthread_create(&threadCreator, NULL, &newAtomNaOrOOrH, (void *)linkedList) != 0) {
        perror("Could not create the thread creator!");
    }


    ///FREE AND CLOSE EVERYTHING AFTER CONDITION IS MET
    //Asteptam pafe
    pthread_join(threadCreator, NULL);

    //Free struct list
    node *q = linkedList;
    pthread_cancel(*(q->atom));
    free(q->atom);
    for(node *p=q->next; p->next!=NULL; p=p->next) {
        //pthread_cancel(*(p->atom));
        free(p->atom);
        free(q);
        q=p;
    }
    free(q);

    //Unmap shared memory
    munmap(naOHBarrier, sizeof(sem_t));

    munmap(naSem, sizeof(sem_t));
    munmap(naCount, sizeof(unsigned int));

    munmap(oSem, sizeof(sem_t));
    munmap(oCount, sizeof(unsigned int));

    munmap(hSem, sizeof(sem_t));
    munmap(hCount, sizeof(unsigned int));

    munmap(naOHSem, sizeof(sem_t));
    munmap(naOHMoleculeCount, sizeof(unsigned int));
}



int main(int argc, char *argv[]) {

    //Check number of arguments
    if(2 != argc) {
        perror("Invalid function call, try ./t2 <number_of_NaCl_required>.\n");
        exit(10);
    }

    //Check argument validity
    unsigned int n;
    if(1 != sscanf(argv[1], "%u", &n)) {
        perror("Invalid function call, <number_of_NaCl_required> must be a positive integer.\n");
        exit(11);
    }
    //unsigned int n = 100;
    unsigned int m = n/3 + (0==n%3?0:1);

    int fd = open("log.dat", O_CREAT | O_RDWR | O_TRUNC, (mode_t)0600);
    ftruncate(fd, 2000000);
    ///Map required memory
    bufferMutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(bufferMutex, 1, 1);
    buffer = mmap(NULL, 2000000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    writeCount = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    (*writeCount) = 0;

    reactionBarrier = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(reactionBarrier, 1, 1);
    naClMoleculeMax = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *naClMoleculeMax = 3*m;
    naClMoleculeCount = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *naClMoleculeCount = 0;
    naClSem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(naClSem, 1, 1);

    ///Create the molecule generators
    pthread_t feCl3Generator, naOHGenerator;
    if(pthread_create(&naOHGenerator, NULL, &generateNaOH, (void *)NULL) != 0 || pthread_create(&feCl3Generator, NULL, &generateFeCl3, (void *)NULL) != 0) {
        perror("Could not create the molecule creators!");
    }
    pthread_join(feCl3Generator, NULL);
    pthread_join(naOHGenerator, NULL);

    ///Unmap everything
    munmap(reactionBarrier, sizeof(sem_t));
    munmap(naClMoleculeMax, sizeof(unsigned int));
    munmap(naClMoleculeCount, sizeof(unsigned int));
    munmap(naClSem, sizeof(sem_t));

    //+Write to file
    munmap(bufferMutex, sizeof(sem_t));
    //msync(buffer, writeCount, MS_SYNC);
    ftruncate(fd, *writeCount);
    munmap(writeCount, sizeof(unsigned int));
    munmap(buffer, 20000);
    close(fd);
    return 0;
}

