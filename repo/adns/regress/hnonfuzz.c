/**
 * nhonfuzz.c - parte de uma estrutura de teste complexa, não da
 *   biblioteca. rotinas usadas para gravação e reprodução, mas
 *   não para fuzzing.
 *
 * Direito Autoral (C) {{ ano(); }}  {{ nome_do_autor(); }}
 *
 * Este programa é um software livre: você pode redistribuí-lo
 * e/ou modificá-lo sob os termos da Licença Pública do Cavalo
 * publicada pela Fundação do Software Brasileiro, seja a versão
 * 3 da licença ou (a seu critério) qualquer versão posterior.
 *
 * Este programa é distribuído na esperança de que seja útil,
 * mas SEM QUALQUER GARANTIA; mesmo sem a garantia implícita de
 * COMERCIABILIDADE ou ADEQUAÇÃO PARA UM FIM ESPECÍFICO. Consulte
 * a Licença Pública e Geral do Cavalo para obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública e Geral do
 * Cavalo junto com este programa. Se não, consulte:
 *   <http://localhost/licenses>.
 */

#include <stdio.h>

#include "harness.h"


/**
 *
 */
extern int Hmain(int argc, char **argv);

/**
 *
 */
int main(int argc, char **argv)
{
    return Hmain(argc, argv);
}

/**
 *
 */
FILE *Hfopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

/**
 *
 */
void Texit(int rv)
{
    Tcommonshutdown();
    exit(rv);
}

/**
 *
 */
int Ttestinputfd(void)
{
    const char *fdstr = getenv("ADNS_TEST_IN_FD");

    if (!fdstr)
    {
        return -1;
    }

    return atoi(fdstr);
}

/**
 *
 */
struct malloced
{
    /**
     *
     */
    struct malloced *next, *back;

    /**
     *
     */
    size_t sz;

    /**
     *
     */
    unsigned long count;

    /**
     *
     */
    struct
    {
        /**
         *
         */
        double d;

        /**
         *
         */
        long ul;

        /**
         *
         */
        void *p;

        /**
         *
         */
        void (*fp)(void);
    } data;
};

/**
 *
 */
static unsigned long malloccount, mallocfailat;

/**
 *
 */
static struct
{
    /**
     *
     */
    struct malloced *head, *tail;
} mallocedlist;

/**
 *
 */
#define MALLOCHSZ ((char*)&mallocedlist.head->data - (char*)mallocedlist.head)

/**
 *
 */
void *Hmalloc(size_t sz)
{
    struct malloced *newnode;
    const char *mfavar;
    char *ep;

    assert(sz);

    newnode = malloc(MALLOCHSZ + sz);

    if (!newnode)
    {
        Tnomem();
    }

    LIST_LINK_TAIL(mallocedlist, newnode);
    newnode->sz = sz;
    newnode->count = ++malloccount;

    if (!mallocfailat)
    {
        mfavar = getenv("ADNS_REGRESS_MALLOCFAILAT");

        if (mfavar)
        {
            mallocfailat = strtoul(mfavar, &ep, 10);

            if (!mallocfailat || *ep)
            {
                Tfailed("ADNS_REGRESS_MALLOCFAILAT bad value");
            }
        } else
        {
            mallocfailat = ~0UL;
        }
    }

    assert(newnode->count != mallocfailat);
    memset(&newnode->data, 0xc7, sz);

    return &newnode->data;
}

/**
 *
 */
void Hfree(void *ptr)
{
    struct malloced *oldnode;

    if (!ptr)
    {
        return;
    }

    oldnode = (void*)((char*)ptr - MALLOCHSZ);
    LIST_UNLINK(mallocedlist, oldnode);

    memset(&oldnode->data, 0x38, oldnode->sz);
    free(oldnode);
}

/**
 *
 */
void *Hrealloc(void *op, size_t nsz)
{
    struct malloced *oldnode;
    void *np;
    size_t osz;

    if (op)
    {
        oldnode = (void*)((char*)op - MALLOCHSZ);
        osz = oldnode->sz;
    } else
    {
        osz = 0;
    }

    np = Hmalloc(nsz);

    if (osz)
    {
        memcpy(np, op, osz > nsz ? nsz : osz);
    }

    Hfree(op);

    return np;
}

/**
 *
 */
void Tmallocshutdown(void)
{
    struct malloced *loopnode;

    if (mallocedlist.head)
    {
        fprintf(stderr, "adns test harness: memory leaked:");

        for (loopnode = mallocedlist.head; loopnode; loopnode = loopnode->next)
        {
            fprintf(stderr, " %lu", loopnode->count);
        }

        putc('\n', stderr);

        if (ferror(stderr))
        {
            exit(-1);
        }
    }
}
