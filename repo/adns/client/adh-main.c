/**
 * adh-main.c - programa cliente resolvedor de propósito geral útil.
 *     programa principal e sub-rotinas úteis.
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

#include "adnshost.h"


/**
 *
 */
int rcode;

/**
 *
 */
const char *config_text;

/**
 *
 */
static int used, avail;
static char *buf;


/**
 *
 */
void quitnow(int rc)
{
    if (ads)
    {
        adns_finish(ads);
    }

    free(buf);
    free(ov_id);
    exit(rc);
}

/**
 *
 */
void sysfail(const char *what, int errnoval)
{
    fprintf(stderr, "adnshost failed: %s: %s\n", what, strerror(errnoval));
    quitnow(10);
}

/**
 *
 */
void usageerr(const char *fmt, ...)
{
    va_list al;

    fputs("adnshost usage error: ", stderr);
    va_start(al, fmt);
    vfprintf(stderr, fmt, al);
    va_end(al);
    putc('\n', stderr);
    quitnow(11);
}

/**
 *
 */
void outerr(void)
{
    sysfail("write to stdout", errno);
}

/**
 *
 */
void *xmalloc(size_t sz)
{
    void *p;

    p = malloc(sz);

    if (!p)
    {
        sysfail("malloc", sz);
    }

    return p;
}

/**
 *
 */
char *xstrsave(const char *str)
{
    char *p;

    p = xmalloc(strlen(str) + 1);
    strcpy(p,str);

    return p;
}

/**
 *
 */
void of_config(const struct optioninfo *oi, const char *arg, const char *arg2)
{
    config_text = arg;
}

/**
 *
 */
void of_type(const struct optioninfo *oi, const char *arg, const char *arg2)
{
    static const struct typename {
        /**
         *
         */
        adns_rrtype type;

        /**
         *
         */
        const char *desc;
    } typenames[] = {
        /**
         * Versões aprimoradas.
         */
        { adns_r_ns, "ns" },
        { adns_r_soa, "soa" },
        { adns_r_ptr, "ptr" },
        { adns_r_mx, "mx" },
        { adns_r_rp, "rp" },
        { adns_r_srv, "srv" },
        { adns_r_addr, "addr"},

        /**
         * Tipos com apenas uma versão.
         */
        { adns_r_cname, "cname" },
        { adns_r_hinfo, "hinfo" },
        { adns_r_txt, "txt" },
        
        /**
         * Versões enxutas.
         */
        { adns_r_a, "a" },
        { adns_r_aaaa, "aaaa" },
        { adns_r_ns_raw, "ns-" },
        { adns_r_soa_raw, "soa-" },
        { adns_r_ptr_raw, "ptr-" },
        { adns_r_mx_raw, "mx-" },
        { adns_r_rp_raw, "rp-" },
        { adns_r_srv_raw, "srv-" },

        /**
         *
         */
        { adns_r_none, 0 }
    };

    const struct typename *tnp;
    unsigned long unknowntype;
    char *ep;

    if (strlen(arg) > 4 && !memcmp(arg, "type", 4) && (unknowntype = strtoul(arg + 4, &ep, 10), !*ep) && unknowntype < 65536)
    {
        ov_type = unknowntype | adns_r_unknown;

        return;
    }

    for (tnp = typenames; tnp->type && strcmp(arg, tnp->desc); tnp++)
    {
    }

    if (!tnp->type)
    {
        usageerr("unknown RR type %s", arg);
    }

    ov_type = tnp->type;
}

/**
 *
 */
static void process_optarg(const char *arg, const char *const **argv_p, char *value)
{
    const struct optioninfo *oip;
    const char *arg2;
    int invert;

    if (arg[0] == '-' || arg[0] == '+')
    {
        if (arg[0] == '-' && arg[1] == '-')
        {
            if (!strncmp(arg, "--no-", 5))
            {
                invert = 1;
                oip = opt_findl(arg + 5);
            } else
            {
                invert = 0;
                oip = opt_findl(arg + 2);
            }

            if (oip->type == ot_funcarg)
            {
                if (argv_p)
                {
                    arg = *++(*argv_p);
                } else
                {
                    arg = value;
                }

                if (!arg)
                {
                    usageerr("option --%s requires a value argument", oip->lopt);
                }

                arg2 = 0;
            } else if (oip->type == ot_funcarg2)
            {
                if (argv_p)
                {
                    arg = *++(*argv_p);

                    if (arg)
                    {
                        arg2 = *++(*argv_p);
                    } else
                    {
                        arg2 = 0;
                    }
                } else if (value)
                {
                    arg = value;
                    char *space = strchr(value, ' ');

                    if (space)
                    {
                        *space++ = 0;
                    }

                    arg2 = space;
                } else
                {
                    arg = 0;
                }

                if (!arg || !arg2)
                {
                    usageerr("option --%s requires two more arguments", oip->lopt);
                }
            } else
            {
                if (value)
                {
                    usageerr("option --%s does not take a value", oip->lopt);
                }

                arg = 0;
                arg2 = 0;
            }

            opt_do(oip, invert, arg, arg2);
        } else if (arg[0] == '-' && arg[1] == 0)
        {
            arg = argv_p ? *++(*argv_p) : value;

            if (!arg)
            {
                usageerr("option `-' must be followed by a domain");
            }

            query_do(arg);
        } else
        {
            /**
             * arg[1] != '-', != '\0'.
             */
            invert = (arg[0] == '+');
            ++arg;

            while (*arg)
            {
                oip = opt_finds(&arg);

                if (oip->type == ot_funcarg)
                {
                    if (!*arg)
                    {
                        if (argv_p)
                        {
                            arg = *++(*argv_p);
                        } else
                        {
                            arg = value;
                        }

                        if (!arg)
                        {
                            usageerr("option -%s requires a value argument", oip->sopt);
                        }
                    } else
                    {
                        if (value)
                        {
                            usageerr("two values for option -%s given !", oip->sopt);
                        }
                    }

                    opt_do(oip, invert, arg, 0);
                    arg = "";
                } else
                {
                    if (value)
                    {
                        usageerr("option -%s does not take a value", oip->sopt);
                    }

                    opt_do(oip, invert, 0, 0);
                }
            }
        }
    } else
    {
        /**
         * arg[0] != '-'.
         */
        query_do(arg);
    }
}

/**
 *
 */
static void read_stdin(void)
{
    int anydone, r;
    char *newline, *space;

    anydone = 0;

    while (!anydone || used)
    {
        while (!(newline = memchr(buf, '\n', used)))
        {
            if (used == avail)
            {
                avail += 20;
                avail <<= 1;

                buf = realloc(buf, avail);

                if (!buf)
                {
                    sysfail("realloc stdin buffer", errno);
                }
            }

            do
            {
                r = read(0, buf + used, avail - used);
            } while (r < 0 && errno == EINTR);

            if (r == 0)
            {
                if (used)
                {
                    /**
                     * Transforme a última nova linha .
                     */
                    buf[used] = '\n';
                    r = 1;
                } else
                {
                    ov_pipe = 0;
                    return;
                }
            }

            if (r < 0)
            {
                sysfail("read stdin", errno);
            }

            used += r;
        }

        *newline++ = 0;
        space = strchr(buf, ' ');

        if (space)
        {
            *space++ = 0;
        }

        process_optarg(buf, 0, space);
        used -= (newline - buf);
        memmove(buf, newline, used);
        anydone = 1;
    }
}

/**
 *
 */
int main(int argc, const char *const *argv)
{
    struct timeval *tv, tvbuf;
    adns_query qu;

    void *qun_v;
    adns_answer *answer;

    int r;
    int maxfd;

    fd_set readfds, writefds, exceptfds;
    const char *arg;

    while ((arg = *++argv))
    {
        process_optarg(arg, &argv, 0);
    }

    if (!ov_pipe && !ads)
    {
        usageerr("no domains given, and -f/--pipe not used; try --help");
    }

    ensure_adns_init();

    for (;;)
    {
        for (;;)
        {
            qu = ov_asynch ? 0 : outstanding.head ? outstanding.head->qu : 0;
            r = adns_check(ads, &qu, &answer, &qun_v);

            if (r == EAGAIN)
            {
                break;
            }

            if (r == ESRCH)
            {
                if (!ov_pipe)
                {
                    /**
                     * Pular lá embaixo.
                     */
                    goto x_quit;
                } else
                {
                    break;
                }
            }

            assert(!r);
            query_done(qun_v, answer);
        }

        maxfd = 0;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);

        if (ov_pipe)
        {
            maxfd = 1;
            FD_SET(0, &readfds);
        }

        tv = 0;
        adns_beforeselect(ads, &maxfd, &readfds, &writefds, &exceptfds, &tv, &tvbuf, 0);
        r = select(maxfd, &readfds, &writefds, &exceptfds, tv);

        if (r == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            sysfail("select",errno);
        }

        adns_afterselect(ads, maxfd, &readfds, &writefds, &exceptfds, 0);

        if (ov_pipe && FD_ISSET(0, &readfds))
        {
            read_stdin();
        }
    }

    x_quit:
        if (fclose(stdout))
        {
            outerr();
        }

        quitnow(rcode);
}
