#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include "proc_maps_parser/pmparser.h"

static procmaps_struct *maps = NULL;
static pthread_mutex_t lock;
static pthread_mutex_t lock2;
static pthread_cond_t cond;

void *threadfn(void *p)
{
	int val=100;
    size_t guard_size = (size_t)(p);

    pthread_mutex_lock (&lock2);
    pthread_cond_wait (&cond, &lock2);

    procmaps_struct *maps_tmp = NULL;
    procmaps_struct *prev = NULL;
    while( (maps_tmp = pmparser_next ()) !=NULL ){
        if (maps_tmp->addr_start < (void*)&val && maps_tmp->addr_end > (void *)&val) {
            if (!prev) {
               printf("ERROR: condition that should never happen!\n");
               continue;
            }
#if DEBUG
            pmparser_print (prev,0);
            printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            pmparser_print (maps_tmp,0);
            printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
            if ((size_t)prev->length == guard_size)
                printf ("GOOD: Guard sizes (%d) match\n", (int)guard_size);
            else
                printf ("BAD: Guard page sizes don't match (%d != %d)\n",
                        (int)prev->length, (int)guard_size);
        }
        prev = maps_tmp;
    }
    pthread_cond_signal (&cond);
    pthread_mutex_unlock (&lock2);

    pthread_exit (NULL);
}

int main(int argc, char *argv[])
{
    int rc;
	int val=100;
    size_t guard_size, guard_size2;
	pthread_t t1, t2;
    pthread_attr_t attr, attr2;


    if (argc != 2) {
        fprintf (stderr, "Usage guard_page_test guard_size\n");
        fprintf (stderr, "    guard_size must be a multiple of 4096\n");
        exit (1);
    }

    guard_size = atoi (argv[1]);
    guard_size2 = guard_size;

    if ((rc = pthread_attr_init (&attr)) == -1) {
        perror ("pthread_attr_init");
        return EXIT_FAILURE;
    }
    if ((rc = pthread_attr_init (&attr2)) == -1) {
        perror ("pthread_attr_init");
        return EXIT_FAILURE;
    }
    if ((rc = pthread_attr_setguardsize (&attr, guard_size)) == -1) {
        perror ("pthread_attr_setguardsize");
        return EXIT_FAILURE;
    }

    if ((rc = pthread_attr_setguardsize (&attr2, guard_size2)) == -1) {
        perror ("pthread_attr_setguardsize");
        return EXIT_FAILURE;
    }
	pthread_create(&t1, &attr, threadfn, (void *)guard_size);
	pthread_create(&t2, &attr2, threadfn, (void *)guard_size2);

    pthread_mutex_lock (&lock);
    if ((maps = pmparser_parse (getpid ())) == NULL) {
        fprintf (stderr, "ERROR: Cannot parse the memory map of %d\n",
                 getpid ());
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock (&lock);

#if DEBUG
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    procmaps_struct *maps_tmp = NULL;
    while( (maps_tmp=pmparser_next ()) !=NULL ){
        if (maps_tmp->addr_start < (void*)&val
            && maps_tmp->addr_end > (void *)&val) {
            pmparser_print (maps_tmp,0);
            printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        }
    }
#endif

    pthread_mutex_lock (&lock2);
    pthread_cond_signal (&cond);
    pthread_mutex_unlock (&lock2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

	return EXIT_SUCCESS;
}
