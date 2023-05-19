/**
 * adnslogres.c - Um substituto para o programa Apache logresolve
 *     usando adns.
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

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "config.h"
#include "adns.h"
#include "client.h"

#ifdef ADNS_REGRESS_TEST
#include "hredirect.h"
#endif


/**
 * Número máximo de consultas DNS simultâneas.
 */
#define MAXMAXPENDING 64000
#define DEFMAXPENDING 2000

/**
 * Comprimento máximo de uma linha.
 */
#define MAXLINE 1024

/**
 * Sinalizadores de opção.
 */
#define OPT_DEBUG 1
#define OPT_POLL 2
#define OPT_CHECKC 4

/**
 *
 */
static const char *const progname = "adnslogres";
static const char *config_text;

/**
 *
 */
#define guard_null(str) ((str) ? (str) : "")

/**
 *
 */
#define sensible_ctype(type,ch) (type((unsigned char)(ch)))

/**
 * A função isfoo() de ctype.h não pode receber informações com
 * segurança char.
 */

/**
 *
 */
static void msg(const char *fmt, ...)
{
    va_list al;

    fprintf(stderr, "%s: ", progname);
    va_start(al, fmt);
    vfprintf(stderr, fmt, al);
    va_end(al);
    fputc('\n', stderr);
}

/**
 *
 */
static void aargh(const char *cause)
{
    const char *why = strerror(errno);

    if (!why)
    {
        why = "Unknown error";
    }

    msg("%s: %s (%d)", cause, why, errno);
    exit(1);
}

/**
 * Analise o endereço IP e converta em um nome de domínio reverso.
 */
static char *ipaddr2domain(char *start, char **addr, char **rest)
{
    /**
     * "123.123.123.123.in-addr.arpa.\0"
     */
    static char buf[30];

    char *ptrs[5];
    int i;

    ptrs[0]= start;

    retry:
        while (!sensible_ctype(isdigit, *ptrs[0]))
        {
            if (!*ptrs[0]++)
            {
                strcpy(buf, "invalid.");
                *addr = *rest = NULL;

                return buf;
            }
        }

        for (i= 1; i < 5; i++)
        {
            ptrs[i] = ptrs[i - 1];

            while (sensible_ctype(isdigit, *ptrs[i]++));

            if ((i == 4 && !sensible_ctype(isspace, ptrs[i][-1])) || (i != 4 && ptrs[i][-1] != '.') || (ptrs[i] - ptrs[i - 1] > 4))
            {
                ptrs[0] = ptrs[i] - 1;

                /**
                 * Volte lá para cima.
                 */
                goto retry;
            }
        }

        sprintf(buf, "%.*s.%.*s.%.*s.%.*s.in-addr.arpa.",
            (int)(ptrs[4] - ptrs[3] - 1), ptrs[3],
            (int)(ptrs[3] - ptrs[2] - 1), ptrs[2],
            (int)(ptrs[2] - ptrs[1] - 1), ptrs[1],
            (int)(ptrs[1] - ptrs[0] - 1), ptrs[0]);

        *addr = ptrs[0];
        *rest = ptrs[4] - 1;

        return buf;
}

/**
 *
 */
static void printline(FILE *outf, char *start, char *addr, char *rest, char *domain)
{
    if (domain)
    {
        fprintf(outf, "%.*s%s%s", (int)(addr - start), start, domain, rest);
    } else
    {
        fputs(start, outf);
    }

    if (ferror(outf))
    {
        aargh("write output");
    }
}

/**
 *
 */
typedef struct logline
{
    /**
     *
     */
    struct logline *next;

    /**
     *
     */
    char *start, *addr, *rest;

    /**
     *
     */
    adns_query query;
} logline;

/**
 *
 */
static logline *readline(FILE *inf, adns_state adns, int opts)
{
    static char buf[MAXLINE];
    char *str;
    logline *line;

    if (fgets(buf, MAXLINE, inf))
    {
        str = malloc(sizeof(*line) + strlen(buf) + 1);

        if (!str)
        {
            aargh("malloc");
        }

        line = (logline*)str;
        line->next = NULL;
        line->start = str + sizeof(logline);

        strcpy(line->start, buf);
        str = ipaddr2domain(line->start, &line->addr, &line->rest);

        if (opts & OPT_DEBUG)
        {
            msg("submitting %.*s -> %s", (int)(line->rest - line->addr), guard_null(line->addr), str);
        }

        if (adns_submit(adns, str, adns_r_ptr, adns_qf_quoteok_cname | adns_qf_cname_loose, NULL, &line->query))
        {
            aargh("adns_submit");
        }

        return line;
    }

    if (!feof(inf))
    {
        aargh("fgets");
    }

    return NULL;
}

/**
 *
 */
static void proclog(FILE *inf, FILE *outf, int maxpending, int opts)
{
    int eof, err, len;
    adns_state adns;
    adns_answer *answer;
    logline *head, *tail, *line;
    adns_initflags initflags;

    initflags = (((opts & OPT_DEBUG) ? adns_if_debug : 0) |
                 ((opts & OPT_CHECKC) ? adns_if_checkc_entex : 0));

    if (config_text)
    {
        errno = adns_init_strcfg(&adns, initflags, stderr, config_text);
    } else
    {
        errno = adns_init(&adns, initflags, 0);
    }

    if (errno)
    {
        aargh("adns_init");
    }

    head = tail = readline(inf, adns, opts);
    len = 1;
    eof = 0;

    while (head)
    {
        while (head)
        {
            if (opts & OPT_DEBUG)
            {
                msg("%d in queue; checking %.*s", len, (int)(head->rest-head->addr), guard_null(head->addr));
            }

            if (eof || len >= maxpending)
            {
                if (opts & OPT_POLL)
                {
                    err = adns_wait_poll(adns, &head->query, &answer, NULL);
                } else
                {
                    err = adns_wait(adns, &head->query, &answer, NULL);
                }
            } else
            {
                err = adns_check(adns, &head->query, &answer, NULL);
            }

            if (err == EAGAIN)
            {
                break;
            }

            if (err)
            {
                fprintf(stderr, "%s: adns_wait/check: %s", progname, strerror(err));
                exit(1);
            }

            printline(outf, head->start, head->addr, head->rest, answer->status == adns_s_ok ? *answer->rrs.str : NULL);
            line = head;
            head = head->next;

            free(line);
            free(answer);
            len--;
        }

        if (!eof)
        {
            line = readline(inf, adns, opts);

            if (line)
            {
                if (!head)
                {
                    head = line;
                } else
                {
                    tail->next = line;
                }

                tail = line;
                len++;
            } else
            {
                eof = 1;
            }
        }
    }

    adns_finish(adns);
}

/**
 *
 */
static void printhelp(FILE *file)
{
    fputs("usage: adnslogres [<options>] [<logfile>]\n"
          "       adnslogres --version|--help\n"
          "options: -c <concurrency>  set max number of outstanding queries\n"
          "         -p                use poll(2) instead of select(2)\n"
          "         -d                turn on debugging\n"
          "         -C <config>       use instead of contents of resolv.conf\n", stdout);
}

/**
 *
 */
static void usage(void)
{
    printhelp(stderr);
    exit(1);
}

/**
 *
 */
int main(int argc, char *argv[])
{
    int c, opts, maxpending;
    extern char *optarg;
    FILE *inf;
    opts = 0;

    if (argv[1] && !strcmp(argv[1],"--checkc-freq"))
    {
        opts |= OPT_CHECKC;
        argv++;
        argc--;
    }

    if (argv[1] && !strncmp(argv[1], "--", 2))
    {
        if (!strcmp(argv[1], "--help"))
        {
            printhelp(stdout);
        } else if (!strcmp(argv[1], "--version"))
        {
            fputs(VERSION_MESSAGE("adnslogres"), stdout);
        } else
        {
            usage();
        }

        if (ferror(stdout) || fclose(stdout))
        {
            perror("stdout");
            exit(1);
        }

        exit(0);
    }

    maxpending = DEFMAXPENDING;

    while ((c = getopt(argc, argv, "c:C:dp")) != -1)
    {
        switch (c)
        {
            case 'c':
                maxpending = atoi(optarg);

                if (maxpending < 1 || maxpending > MAXMAXPENDING)
                {
                    fprintf(stderr, "%s: unfeasible concurrency %d\n", progname, maxpending);
                    exit(1);
                }

                break;

            case 'C':
                config_text = optarg;
                break;

            case 'd':
                opts |= OPT_DEBUG;
                break;

            case 'p':
                opts |= OPT_POLL;
                break;

            default:
                usage();
        }
    }

    argc-= optind;
    argv += optind;
    inf = NULL;

    if (argc == 0)
    {
        inf = stdin;
    } else if (argc == 1)
    {
        inf = fopen(*argv, "r");
    } else
    {
        usage();
    }

    if (!inf)
    {
        aargh("couldn't open input");
    }

    proclog(inf, stdout, maxpending, opts);

    if (fclose(inf))
    {
        aargh("fclose input");
    }

    if (fclose(stdout))
    {
        aargh("fclose output");
    }

    return 0;
}
