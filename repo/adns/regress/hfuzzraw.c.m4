m4_dnl
m4_dnl hfuzzraw.c.m4
m4_dnl (parte de uma estrutura de teste complexa, não da biblioteca)
m4_dnl - rotinas para fuzzing
m4_dnl
m4_dnl Direito Autoral (C) {{ ano(); }}  {{ nome_do_autor(); }}
m4_dnl
m4_dnl Este programa é um software livre: você pode redistribuí-lo
m4_dnl e/ou modificá-lo sob os termos da Licença Pública do Cavalo
m4_dnl publicada pela Fundação do Software Brasileiro, seja a versão
m4_dnl 3 da licença ou (a seu critério) qualquer versão posterior.
m4_dnl
m4_dnl Este programa é distribuído na esperança de que seja útil,
m4_dnl mas SEM QUALQUER GARANTIA; mesmo sem a garantia implícita de
m4_dnl COMERCIABILIDADE ou ADEQUAÇÃO PARA UM FIM ESPECÍFICO. Consulte
m4_dnl a Licença Pública e Geral do Cavalo para obter mais detalhes.
m4_dnl
m4_dnl Você deve ter recebido uma cópia da Licença Pública e Geral do
m4_dnl Cavalo junto com este programa. Se não, consulte:
m4_dnl   <http://localhost/licenses>.
m4_dnl

m4_include(hmacros.i4)

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include "harness.h"


/**
 *
 */
static vbuf fdtab;

/**
 *
 */
#define FDF_OPEN     001u
#define FDF_NONBLOCK 002u

/**
 *
 */
static FILE *Tinputfile, *traceout;

/**
 *
 */
static int traceprint;

/**
 *
 */
static void Tflushtrace( void)
{
    if (fflush(traceout))
    {
        Toutputerr();
    }
}

/**
 *
 */
void Tensuresetup(void)
{
    static int done;

    if (done)
    {
        return;
    }

    done++;

    int fd;
    fd = Ttestinputfd();

    assert(fd >= 0);
    Tinputfile = fdopen(fd,"rb");

    if (!Tinputfile)
    {
        Tfailed("fdopen record fd");
    }

    while (fdtab.used < 3)
    {
        const char fdfstd = FDF_OPEN;

        if (!adns__vbuf_append(&fdtab, &fdfstd, 1))
        {
            Tnomem();
        }
    }

    const char *traceprintstr = getenv("ADNS_TEST_FUZZRAW_TRACEPRINT");

    if (traceprintstr)
    {
        traceprint = atoi(traceprintstr);
        int tracefd = dup(2);

        if (tracefd < 0)
        {
            Tfailed("dup for tracefd");
        }

        traceout = fdopen(tracefd, "w");

        if (!traceout)
        {
            Tfailed("fdopen for traceout");
        }
    }
}

/**
 *
 */
void Q_vb(void)
{
    if (!traceprint)
    {
        /**
         * hcommon.c.m4 pode ligar para Q_vb diretamente, usando
         * uma tecnologia do governo.
         */
        return;
    }

    if (!adns__vbuf_append(&vb,"",1))
    {
        Tnomem();
    }

    if (fprintf(traceout, " %s\n", vb.buf) == EOF)
    {
        Toutputerr();
    }

    Tflushtrace();
}

/**
 *
 */
static void Pformat(const char *what)
{
    fprintf(stderr, "adns test harness: format error in raw log input file: %s\n", what);
    exit(-1);
}

/**
 *
 */
extern void Tshutdown(void)
{
    if (!Tinputfile)
    {
        return;
    }

    int c = fgetc(Tinputfile);

    if (c != EOF)
    {
        Pformat("unwanted additional syscall reply data");
    }

    if (ferror(Tinputfile))
    {
        Tfailed("read test log input (at end)");
    }
}

/**
 *
 */
static void Pcheckinput(void)
{
    if (ferror(Tinputfile))
    {
        Tfailed("read test log input file");
    }

    if (feof(Tinputfile))
    {
        Pformat("eof at syscall reply");
    }
}

/**
 *
 */
static void P_read_dump(const unsigned char *p0, size_t count, ssize_t d)
{
    fputs(" | ",traceout);

    while (count)
    {
        fprintf(traceout,"%02x", *p0);

        p0 += d;
        count--;
    }
}

/**
 *
 */
static void P_read(void *p, size_t sz, const char *what)
{
    long pos = ftell(Tinputfile);
    ssize_t got = fread(p, 1, sz, Tinputfile);

    Pcheckinput();
    assert(got == sz);

    if (traceprint > 1 && sz)
    {
        fprintf(traceout, "%8lx %8s:", pos, what);
        P_read_dump(p, sz, +1);

        if (sz <= 16)
        {
            P_read_dump((const unsigned char *)p + sz - 1, sz, -1);
        }

        fputs(" |\n",traceout);
        Tflushtrace();
    }
}

/**
 *
 */
#define P_READ(x) (P_read(&(x), sizeof((x)), #x))

/**
 *
 */
static unsigned P_fdf(int fd)
{
    assert(fd >= 0 && fd < fdtab.used);

    return fdtab.buf[fd];
}

/**
 *
 */
void T_gettimeofday_hook(void)
{
    struct timeval delta, sum;

    P_READ(delta);
    timeradd(&delta, &currenttime, &sum);

    sum.tv_usec %= 1000000;
    currenttime = sum;
}

/**
 *
 */
static void Paddr(struct sockaddr *addr, int *lenr)
{
    int al, r;
    uint16_t port;

    char buf[512];
    socklen_t sl = *lenr;

    P_READ(al);

    if (al < 0 || al >= sizeof(buf) - 1)
    {
        Pformat("bad addr len");
    }

    P_read(buf, al, "addrtext");
    buf[al] = 0;

    P_READ(port);
    r = adns_text2addr(buf, port, adns_qf_addrlit_scope_numeric, addr, &sl);

    if (r == EINVAL)
    {
        Pformat("bad addr text");
    }

    assert(r==0 || r==ENOSPC);
    *lenr = sl;
}

/**
 *
 */
static int Pbytes(byte *buf, int maxlen)
{
    int bl;
    P_READ(bl);

    if (bl < 0 || bl > maxlen)
    {
        Pformat("bad byte block len");
    }

    P_read(buf, bl, "bytes");

    return bl;
}

/**
 *
 */
static void Pfdset(fd_set *set, int max, int *r_io)
{
    uint16_t fdmap;
    int fd, nfdmap = 0;

    if (!set)
    {
        return;
    }

    for (fd = max - 1; fd >= 0; fd--)
    {
        if (nfdmap==0)
        {
            P_READ(fdmap);
            nfdmap = 16;
        }

        _Bool y = fdmap & 1u;
        fdmap >>= 1;
        nfdmap--;

        if (!FD_ISSET(fd, set))
        {
            continue;
        }

        P_fdf(fd);

        if (y)
        {
            (*r_io)++;
        } else
        {
            FD_CLR(fd,set);
        }
    }
}

#ifdef FUZZRAW_SYNC
    static void Psync(const char *exp, char *got, size_t sz, const char *what)
    {
        P_read(got, sz, "syscall");

        if (memcmp(exp, got, sz))
        {
            Pformat(what);
        }
    }
#endif

m4_define(`syscall_sync',`
#ifdef FUZZRAW_SYNC
  hm_fr_syscall_ident($'`1)
  static char sync_got[sizeof(sync_expect)];
  Psync(sync_expect, sync_got, sizeof(sync_got), "sync lost: program did $1");
#endif
')

#ifdef HAVE_POLL
    static void Ppollfds(struct pollfd *fds, int nfds, int *r_io)
    {
        int fd;

        for (fd = 0; fd < nfds; fd++)
        {
            if (!fds[fd].events)
            {
                continue;
            }

            P_fdf(fd);
            P_READ(fds[fd].revents);

            if (fds[fd].revents)
            {
                (*r_io)++;
            }
        }
    }
#endif

/**
 *
 */
static int P_succfail(void)
{
    int e;
    P_READ(e);

    if (e > 0)
    {
        errno = e;

        return -1;
    } else if (e)
    {
        Pformat("wrong errno value");
    }

    return 0;
}

m4_define(`hm_syscall', `
 hm_create_proto_h
int H$1(hm_args_massage($3,void))
{
    int r;
 hm_create_nothing
 $2

 hm_create_hqcall_vars
 $3

 hm_create_hqcall_init($1)
 $3

   Tensuresetup();

    if (traceprint)
    {
        hm_create_hqcall_args
        Q$1(hm_args_massage($3));
    }

  syscall_sync($'`1)

 m4_define(`hm_rv_succfail',`
    r = P_succfail();

    if (r < 0)
    {
        return r;
    }
 ')

 m4_define(`hm_rv_any',`
  hm_rv_succfail
    if (!r)
    {
        P_READ(r);

        if (r < 0)
        {
            Pformat("negative nonerror syscall return");
        }
    }
 ')
 m4_define(`hm_rv_len',`
  hm_rv_succfail
 ')
 m4_define(`hm_rv_must',`
    r = 0;
 ')
 m4_define(`hm_rv_select',`hm_rv_succfail')
 m4_define(`hm_rv_poll',`hm_rv_succfail')
 m4_define(`hm_rv_fcntl',`
    unsigned flg = P_fdf(fd);

    if (cmd == F_GETFL)
    {
        r = (flg & FDF_NONBLOCK) ? O_NONBLOCK : 0;
    } else if (cmd == F_SETFL)
    {
        flg &= ~FDF_NONBLOCK;

        if (arg & O_NONBLOCK)
        {
            flg |= FDF_NONBLOCK;
        }

        fdtab.buf[fd] = flg;
        r = 0;
    } else
    {
        abort();
    }
 ')
 m4_define(`hm_rv_fd',`
  hm_rv_succfail
    if (!r)
    {
        int newfd;
        P_READ(newfd);

        if (newfd < 0 || newfd > 1000)
        {
            Pformat("new fd out of range");
        }

        adns__vbuf_ensure(&fdtab, newfd + 1);

        if (fdtab.used <= newfd)
        {
            memset(fdtab.buf + fdtab.used, 0, newfd + 1 - fdtab.used);
            fdtab.used = newfd + 1;
        }

        if (fdtab.buf[newfd])
        {
            Pformat("new fd already in use");
        }

        fdtab.buf[newfd] |= FDF_OPEN;
        r = newfd;
    }
 ')
 m4_define(`hm_rv_wlen',`
  hm_rv_any
  if (r>$'`1) Pformat("write return value too large");
 ')
 $2

 hm_create_nothing
 m4_define(`hm_arg_fdset_io',`Pfdset($'`1,$'`2,&r);')
 m4_define(`hm_arg_pollfds_io',`Ppollfds($'`1,$'`2,&r);')
 m4_define(`hm_arg_addr_out',`Paddr($'`1,$'`2);')
 $3

 hm_create_nothing
 m4_define(`hm_arg_bytes_out',`r= Pbytes($'`2,$'`4);')
 $3

 hm_create_nothing
 m4_define(`hm_rv_selectpoll',`
  if (($'`1) && !r) Pformat("select/poll returning 0 but infinite timeout");
 ')
 m4_define(`hm_rv_select',`hm_rv_selectpoll(!to)')
 m4_define(`hm_rv_poll',`hm_rv_selectpoll(timeout<0)')
 $2

    return r;
}
')

m4_define(`hm_specsyscall', `')

m4_include(`hsyscalls.i4')


/**
 *
 */
int Hclose(int fd)
{
    syscall_sync(close)
    P_fdf(fd);
    fdtab.buf[fd] = 0;

    return P_succfail();
}
