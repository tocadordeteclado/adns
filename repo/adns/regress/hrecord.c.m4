m4_dnl
m4_dnl hrecord.c.m4
m4_dnl (parte de uma estrutura de teste complexa, não da biblioteca)
m4_dnl - rotinas de gravação.
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
#include <unistd.h>
#include <fcntl.h>

#include "harness.h"


/**
 *
 */
static FILE *Toutputfile;


/**
 *
 */
void Tshutdown(void)
{
}

/**
 *
 */
static void R_recordtime(void)
{
    int r;
    struct timeval tv, tvrel;

    Tensuresetup();

    r = gettimeofday(&tv, 0);

    if (r)
    {
        Tfailed("gettimeofday syscallbegin");
    }

    tvrel.tv_sec = tv.tv_sec - currenttime.tv_sec;
    tvrel.tv_usec = tv.tv_usec - currenttime.tv_usec;

    if (tv.tv_usec < 0)
    {
        tvrel.tv_usec += 1000000;
        tvrel.tv_sec--;
    }

    Tvbf("\n +%ld.%06ld", (long)tvrel.tv_sec, (long)tvrel.tv_usec);

    currenttime = tv;
}

/**
 *
 */
void T_gettimeofday_hook(void)
{
}

/**
 *
 */
void Tensuresetup(void)
{
    const char *fdstr;
    int fd, r;

    if (Toutputfile)
    {
        return;
    }

    Toutputfile = stdout;
    fdstr = getenv("ADNS_TEST_OUT_FD");

    if (fdstr)
    {
        fd = atoi(fdstr);
        Toutputfile = fdopen(fd,"a");

        if (!Toutputfile)
        {
            Tfailed("fdopen ADNS_TEST_OUT_FD");
        }
    }

    r = gettimeofday(&currenttime,0);

    if (r)
    {
        Tfailed("gettimeofday syscallbegin");
    }

    if (fprintf(Toutputfile, " start %ld.%06ld\n", (long)currenttime.tv_sec, (long)currenttime.tv_usec) == EOF)
    {
        Toutputerr();
    }
}

/**
 *
 */
void Q_vb(void)
{
    if (!adns__vbuf_append(&vb, "", 1))
    {
        Tnomem();
    }

    Tensuresetup();

    if (fprintf(Toutputfile, " %s\n", vb.buf) == EOF)
    {
        Toutputerr();
    }

    if (fflush(Toutputfile))
    {
        Toutputerr();
    }
}

/**
 *
 */
static void R_vb(void)
{
    Q_vb();
}

m4_define(`hm_syscall', `
 hm_create_proto_h
int H$1(hm_args_massage($3,void)) {
 int r, e;

 hm_create_hqcall_vars
 $3

 hm_create_hqcall_init($1)
 $3

 hm_create_hqcall_args
 Q$1(hm_args_massage($3));

 hm_create_realcall_args
 r= $1(hm_args_massage($3));
 e= errno;

 vb.used= 0;
 Tvba("$1=");
 m4_define(`hm_rv_any',`
    if (r == -1)
    {
        Tvberrno(e);

        /**
         * Para aonde eu vou agora ?
         */
        goto x_error;
    }
  Tvbf("%d",r);')
 m4_define(`hm_rv_fd',`hm_rv_any($'`1)')
 m4_define(`hm_rv_succfail',`
  if (r)
  {
        Tvberrno(e);

        /**
         * Para aonde eu vou agora ?
         */
        goto x_error;
  }
  Tvba("OK");')
 m4_define(`hm_rv_must',`Tmust("$1","return",!r); Tvba("OK");')
 m4_define(`hm_rv_len',`
  if (r==-1)
  {
       Tvberrno(e);

        /**
         * Para aonde eu vou agora ?
         */
        goto x_error;
  }
  Tmust("$'`1","return",r<=$'`1);
  Tvba("OK");')
 m4_define(`hm_rv_fcntl',`
  if (r==-1)
  {
        Tvberrno(e);

        /**
         * Para aonde eu vou agora ?
         */
        goto x_error;
  }
  if (cmd == F_GETFL) {
    Tvbf(r & O_NONBLOCK ? "O_NONBLOCK|..." : "~O_NONBLOCK&...");
  } else {
    if (cmd == F_SETFL) {
      Tmust("$1","return",!r);
    } else {
      Tmust("cmd","F_GETFL/F_SETFL",0);
    }
    Tvba("OK");
  }')
 $2

 hm_create_nothing
 m4_define(`hm_arg_fdset_io',`Tvba(" $'`1="); Tvbfdset($'`2,$'`1);')
 m4_define(`hm_arg_pollfds_io',`Tvba(" $'`1="); Tvbpollfds($'`1,$'`2);')
 m4_define(`hm_arg_addr_out',`Tvba(" $'`1="); Tvbaddr($'`1,*$'`2);')
 $3

 hm_create_nothing
 m4_define(`hm_arg_bytes_out',`Tvbbytes($'`2,r);')
 $3

 hm_create_nothing
 m4_define(`hm_rv_any',`x_error:')
 m4_define(`hm_rv_fd',`x_error:')
 m4_define(`hm_rv_succfail',`x_error:')
 m4_define(`hm_rv_len',`x_error:')
 m4_define(`hm_rv_fcntl',`x_error:')
 m4_define(`hm_rv_must',`')
 $2

 R_recordtime();
 R_vb();
 errno= e;
 return r;
}
')

m4_define(`hm_specsyscall', `')

m4_include(`hsyscalls.i4')

hm_stdsyscall_close
