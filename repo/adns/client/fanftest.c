/**
 * fanftest.c - um pequeno programa de teste.
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

#include <sys/types.h>
#include <sys/time.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "config.h"
#include "adns.h"


/**
 *
 */
static const char *progname;

/**
 *
 */
static void aargh(const char *msg)
{
    fprintf(stderr, "%s: %s: %s (%d)\n", progname, msg, strerror(errno) ? strerror(errno) : "Unknown error", errno);
    exit(1);
}

/**
 *
 */
int main(int argc, char *argv[])
{
    adns_state adns;
    adns_query query;
    adns_answer *answer;
    progname = strrchr(*argv, '/');

    if (progname)
    {
        progname++;
    } else
    {
        progname = *argv;
    }

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <domain>\n", progname);
        exit(1);
    }

    errno = adns_init(&adns, adns_if_debug, 0);

    if (errno)
    {
        aargh("adns_init");
    }

    errno= adns_submit(adns, argv[1],
        adns_r_ptr,
        adns_qf_quoteok_cname | adns_qf_cname_loose, NULL, &query);

    if (errno)
    {
        aargh("adns_submit");
    }

    errno = adns_wait(adns, &query, &answer, NULL);

    if (errno)
    {
        aargh("adns_init");
    }

    printf("%s\n", answer->status == adns_s_ok ? *answer->rrs.str : "dunno");
    adns_finish(adns);

    return 0;
}
