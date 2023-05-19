/**
 * setup.c.
 *     - análise de arquivo de configuração.
 *     - gestão do estado global.
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

#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "internal.h"


/**
 *
 */
static void readconfig(adns_state ads, const char *filename, int warnmissing);

/**
 *
 */
static void addserver(adns_state ads, const struct sockaddr *sa, int salen)
{
    int i;
    adns_rr_addr *ss;
    char buf[ADNS_ADDR2TEXT_BUFLEN];

    for (i = 0; i < ads->nservers; i++)
    {
        if (adns__sockaddrs_equal(sa, &ads->servers[i].addr.sa))
        {
            adns__debug(ads, -1, 0, "duplicate nameserver %s ignored", adns__sockaddr_ntoa(sa, buf));

            return;
        }
    }

    if (ads->nservers >= MAXSERVERS)
    {
        adns__diag(ads, -1, 0, "too many nameservers, ignoring %s", adns__sockaddr_ntoa(sa, buf));

        return;
    }

    ss = ads->servers + ads->nservers;
    assert(salen <= sizeof(ss->addr));
    ss->len = salen;

    memcpy(&ss->addr, sa, salen);
    ads->nservers++;
}

/**
 *
 */
static void freesearchlist(adns_state ads)
{
    if (ads->nsearchlist)
    {
        free(*ads->searchlist);
    }

    free(ads->searchlist);
}

/**
 *
 */
static void saveerr(adns_state ads, int en)
{
    if (!ads->configerrno)
    {
        ads->configerrno = en;
    }
}

/**
 *
 */
static void configparseerr(adns_state ads, const char *fn, int lno, const char *fmt, ...)
{
    va_list al;
    saveerr(ads, EINVAL);

    if (!ads->logfn || (ads->iflags & adns_if_noerrprint))
    {
        return;
    }

    if (lno == -1)
    {
        adns__lprintf(ads, "adns: %s: ", fn);
    } else
    {
        adns__lprintf(ads, "adns: %s:%d: ", fn, lno);
    }

    va_start(al, fmt);
    adns__vlprintf(ads, fmt, al);

    va_end(al);
    adns__lprintf(ads, "\n");
}

/**
 *
 */
static int nextword(const char **bufp_io, const char **word_r, int *l_r)
{
    const char *p;
    const char *q;

    p = *bufp_io;

    while (ctype_whitespace(*p))
    {
        p++;
    }

    if (!*p)
    {
        return 0;
    }

    q = p;

    while (*q && !ctype_whitespace(*q))
    {
        q++;
    }

    *l_r = q - p;
    *word_r = p;
    *bufp_io = q;

    return 1;
}

/**
 *
 */
static void ccf_nameserver(adns_state ads, const char *fn, int lno, const char *buf)
{
    adns_rr_addr a;
    char addrbuf[ADNS_ADDR2TEXT_BUFLEN];
    int err;
    socklen_t salen;

    salen = sizeof(a.addr);
    err = adns_text2addr(buf, DNS_PORT, 0, &a.addr.sa, &salen);
    a.len = salen;

    switch (err)
    {
        case 0:
            break;

        case EINVAL:
            configparseerr(ads, fn, lno, "invalid nameserver address `%s'", buf);
            return;

        default:
            configparseerr(ads, fn, lno, "failed to parse nameserver address `%s': %s", buf, strerror(err));
            return;
    }

    adns__debug(ads, -1, 0, "using nameserver %s", adns__sockaddr_ntoa(&a.addr.sa, addrbuf));
    addserver(ads, &a.addr.sa, salen);
}

/**
 *
 */
static void ccf_search(adns_state ads, const char *fn, int lno, const char *buf)
{
    const char *bufp;
    const char *word;
    char *newchars;
    char **newptrs;
    char **pp;
    int count;
    int tl;
    int l;

    if (!buf)
    {
        return;
    }

    bufp = buf;
    count = 0;
    tl = 0;

    while (nextword(&bufp, &word, &l))
    {
        count++;
        tl += l + 1;
    }

    if (count)
    {
        newptrs = malloc(sizeof(char*)*count);

        if (!newptrs)
        {
            saveerr(ads, errno);

            return;
        }

        newchars = malloc(tl);

        if (!newchars)
        {
            saveerr(ads, errno);
            free(newptrs);

            return;
        }
    } else
    {
        assert(!tl);
        newptrs = 0;
        newchars = 0;
    }

    bufp = buf;
    pp = newptrs;

    while (nextword(&bufp, &word, &l))
    {
        *pp++ = newchars;
        memcpy(newchars, word, l);

        newchars += l;
        *newchars++ = 0;
    }

    freesearchlist(ads);
    ads->nsearchlist = count;
    ads->searchlist = newptrs;
}

/**
 *
 */
static int gen_pton(const char *text, int want_af, adns_sockaddr *a)
{
    int err;
    socklen_t len;

    len = sizeof(*a);
    err = adns_text2addr(text,0, adns_qf_addrlit_scope_forbid, &a->sa, &len);

    if (err)
    {
        assert(err == EINVAL);

        return 0;
    }

    if (want_af != AF_UNSPEC && a->sa.sa_family != want_af)
    {
        return 0;
    }

    return 1;
}

/**
 *
 */
static void ccf_sortlist(adns_state ads, const char *fn, int lno, const char *buf)
{
    const char *word;
    char tbuf[200];
    char *slash;
    char *ep;
    const char *maskwhat;
    struct sortlist *sl;

    int l;
    int initial = -1;

    if (!buf)
    {
        return;
    }

    ads->nsortlist = 0;

    while (nextword(&buf, &word, &l))
    {
        if (ads->nsortlist >= MAXSORTLIST)
        {
            adns__diag(ads, -1, 0, "too many sortlist entries, ignoring %.*s onwards", l, word);

            return;
        }

        if (l >= sizeof(tbuf))
        {
            configparseerr(ads, fn, lno, "sortlist entry `%.*s' too long", l, word);

            continue;
        }

        memcpy(tbuf, word, l);

        tbuf[l] = 0;
        slash = strchr(tbuf, '/');

        if (slash)
        {
            *slash++ = 0;
        }

        sl = &ads->sortlist[ads->nsortlist];

        if (!gen_pton(tbuf, AF_UNSPEC, &sl->base))
        {
            configparseerr(ads, fn, lno, "invalid address `%s' in sortlist", tbuf);

            continue;
        }

        if (slash)
        {
            if (slash[strspn(slash, "0123456789")])
            {
                maskwhat = "mask";

                if (!gen_pton(slash, sl->base.sa.sa_family, &sl->mask))
                {
                    configparseerr(ads, fn, lno, "invalid mask `%s' in sortlist", slash);

                    continue;
                }
            } else
            {
                maskwhat = "prefix length";
                unsigned long prefixlen = strtoul(slash, &ep, 10);

                if (*ep || prefixlen > adns__addr_width(sl->base.sa.sa_family))
                {
                    configparseerr(ads, fn, lno, "mask length `%s' invalid", slash);
                    continue;
                }

                initial = prefixlen;
                sl->mask.sa.sa_family = sl->base.sa.sa_family;
                adns__prefix_mask(&sl->mask, initial);
            }
        } else
        {
            maskwhat = "implied prefix length";
            initial = adns__guess_prefix_length(&sl->base);

            if (initial < 0)
            {
                configparseerr(ads, fn, lno,
                    "network address `%s'"
                    " in sortlist is not in classed ranges,"
                    " must specify mask explicitly", tbuf);

                continue;
            }

            sl->mask.sa.sa_family = sl->base.sa.sa_family;
            adns__prefix_mask(&sl->mask, initial);
        }

        if (!adns__addr_matches(sl->base.sa.sa_family, adns__sockaddr_addr(&sl->base.sa), &sl->base, &sl->mask))
        {
            if (initial >= 0)
            {
                configparseerr(ads, fn, lno, "%s %d in sortlist overlaps address `%s'", maskwhat, initial, tbuf);
            } else
            {
                configparseerr(ads, fn, lno, "%s `%s' in sortlist overlaps address `%s'", maskwhat, slash, tbuf);
            }

            continue;
        }

        ads->nsortlist++;
    }
}

/**
 *
 */
static void ccf_options(adns_state ads, const char *fn, int lno, const char *buf)
{
    const char *opt;
    const char *word;
    const char *endword;
    const char *endopt;
    char *ep;
    unsigned long v;
    int l;

    if (!buf)
    {
        return;
    }

    #define WORD__IS(s,op) ((endword-word) op (sizeof(s)-1) && !memcmp(word,s,(sizeof(s)-1)))
    #define WORD_IS(s) (WORD__IS(s, ==))
    #define WORD_STARTS(s) (WORD__IS(s, >=) ? ((word += sizeof(s) - 1)) : 0)

    while (nextword(&buf, &word, &l))
    {
        opt = word;
        endopt = endword = word + l;

        if (WORD_IS("debug"))
        {
            ads->iflags |= adns_if_debug;

            continue;
        }

        if (WORD_STARTS("ndots:"))
        {
            v = strtoul(word, &ep, 10);

            if (ep == word || ep != endword || v > INT_MAX)
            {
                configparseerr(ads, fn, lno, "option `%.*s' malformed or has bad value", l, opt);
                continue;
            }

            ads->searchndots = v;
            continue;
        }

        if (WORD_STARTS("adns_checkc:"))
        {
            if (WORD_IS("none"))
            {
                ads->iflags &= ~adns_if_checkc_freq;
                ads->iflags |= adns_if_checkc_entex;
            } else if (WORD_IS("entex"))
            {
                ads->iflags &= ~adns_if_checkc_freq;
                ads->iflags |= adns_if_checkc_entex;
            } else if (WORD_IS("freq"))
            {
                ads->iflags |= adns_if_checkc_freq;
            } else
            {
                configparseerr(ads, fn, lno, "option adns_checkc has bad value `%s' (must be none, entex or freq", word);
            }

            continue;
        }

        if (WORD_STARTS("adns_af:"))
        {
            ads->iflags &= ~adns_if_afmask;

            if (!WORD_IS("any"))
                for (;;)
                {
                    const char *comma = memchr(word, ',', endopt - word);
                    endword = comma ? comma : endopt;

                    if (WORD_IS("ipv4"))
                    {
                        ads->iflags |= adns_if_permit_ipv4;
                    } else if (WORD_IS("ipv6"))
                    {
                        ads->iflags |= adns_if_permit_ipv6;
                    } else
                    {
                        if (ads->config_report_unknown)
                        {
                            adns__diag(ads, -1, 0, "%s:%d: option adns_af has bad value or entry `%.*s' (option must be `any', or list of `ipv4',`ipv6')", fn, lno, (int)(endword - word), word);
                        }

                        break;
                    }

                    if (!comma)
                    {
                        break;
                    }

                    word = comma + 1;
                }

                continue;
        }

        if (WORD_IS("adns_ignoreunkcfg"))
        {
            ads->config_report_unknown = 0;

            continue;
        }

        /**
         * A estratégia de consulta do adns não é configurável.
         * adns fornece o aplicativo com botão para isso.
         *
         * adns normalmente faz IPv6 se o aplicativo assim o desejar; ao controle.
         * Isso com a opção adns_af: se quiser.
         *
         * adns não faz edns0 e isso não é um problema.
         */
        if (WORD_STARTS("timeout:") || WORD_STARTS("attempts:") || WORD_IS("rotate") || WORD_IS("no-check-names") || WORD_IS("inet6") || WORD_IS("edns0"))
        {
            continue;
        }

        if (ads->config_report_unknown)
        {
            adns__diag(ads, -1, 0, "%s:%d: unknown option `%.*s'", fn, lno, l, opt);
        }
    }

    #undef WORD__IS
    #undef WORD_IS
    #undef WORD_STARTS
}

/**
 *
 */
static void ccf_clearnss(adns_state ads, const char *fn, int lno, const char *buf)
{
    ads->nservers = 0;
}

/**
 *
 */
static void ccf_include(adns_state ads, const char *fn, int lno, const char *buf)
{
    if (!*buf)
    {
        configparseerr(ads, fn, lno, "`include' directive with no filename");

        return;
    }

    readconfig(ads, buf, 1);
}

/**
 *
 */
static void ccf_lookup(adns_state ads, const char *fn, int lno, const char *buf)
{
    int found_bind = 0;
    const char *word;
    int l;

    if (!*buf)
    {
        configparseerr(ads, fn, lno, "`lookup' directive with no databases");

        return;
    }

    while (nextword(&buf, &word, &l))
    {
        if (l == 4 && !memcmp(word, "bind", 4))
        {
            found_bind = 1;
        } else if (l == 4 && !memcmp(word, "file", 4))
        {
            /**
             * Ignore isso e espere que /etc/hosts não seja essencial.
             */
        } else if (l == 2 && !memcmp(word, "yp", 2))
        {
            adns__diag(ads, -1, 0, "%s:%d: yp lookups not supported by adns", fn, lno);
            found_bind = -1;
        } else
        {
            if (ads->config_report_unknown)
            {
                adns__diag(ads, -1, 0, "%s:%d: unknown `lookup' database `%.*s'", fn, lno, l, word);
            }

            found_bind = -1;
        }
    }

    if (!found_bind)
    {
        adns__diag(ads, -1, 0, "%s:%d: `lookup' specified, but not `bind'", fn, lno);
    }
}

/**
 *
 */
static void ccf_ignore(adns_state ads, const char *fn, int lno, const char *buf)
{
}

/**
 *
 */
static const struct configcommandinfo
{
    /**
     *
     */
    const char *name;

    /**
     *
     */
    void (*fn)(adns_state ads, const char *fn, int lno, const char *buf);
} configcommandinfos[]=
{
    { "nameserver",       ccf_nameserver },
    { "domain",           ccf_search     },
    { "search",           ccf_search     },
    { "sortlist",         ccf_sortlist   },
    { "options",          ccf_options    },
    { "clearnameservers", ccf_clearnss   },
    { "include",          ccf_include    },
    { "lookup",           ccf_lookup     },
    { "lwserver",         ccf_ignore     },
    {  0                                 }
};

/**
 *
 */
typedef union
{
    /**
     *
     */
    FILE *file;

    /**
     *
     */
    const char *text;
} getline_ctx;

/**
 *
 */
static int gl_file(adns_state ads, getline_ctx *src_io, const char *filename, int lno, char *buf, int buflen)
{
    FILE *file = src_io->file;
    int c;
    int i;
    char *p;

    p = buf;
    buflen--;
    i = 0;

    for (;;)
    {
        /**
         * loop sobre caracteres.
         */

        if (i == buflen)
        {
            adns__diag(ads, -1, 0, "%s:%d: line too long, ignored", filename, lno);

            /**
             * Pular lá embaixo.
             */
            goto x_badline;
        }

        c = getc(file);

        if (!c)
        {
            adns__diag(ads, -1, 0, "%s:%d: line contains nul, ignored", filename, lno);

            /**
             * Pular lá embaixo.
             */
            goto x_badline;
        } else if (c == '\n')
        {
            break;
        } else if (c == EOF)
        {
            if (ferror(file))
            {
                saveerr(ads, errno);
                adns__diag(ads, -1, 0, "%s:%d: read error: %s", filename, lno, strerror(errno));

                return -1;
            }

            if (!i)
            {
                return -1;
            }

            break;
        } else
        {
            *p++ = c;
            i++;
        }
    }

    *p++ = 0;

    return i;

    x_badline:
        saveerr(ads, EINVAL);

        while ((c = getc(file)) != EOF && c != '\n');

        return -2;
}

/**
 *
 */
static int gl_text(adns_state ads, getline_ctx *src_io, const char *filename, int lno, char *buf, int buflen)
{
    const char *cp = src_io->text;
    int l;

    if (!cp || !*cp)
    {
        return -1;
    }

    if (*cp == ';' || *cp == '\n')
    {
        cp++;
    }

    l = strcspn(cp, ";\n");
    src_io->text = cp + l;

    if (l >= buflen)
    {
        adns__diag(ads, -1, 0, "%s:%d: line too long, ignored", filename, lno);
        saveerr(ads, EINVAL);

        return -2;
    }

    memcpy(buf, cp, l);
    buf[l] = 0;

    return l;
}

/**
 * Retorna:
 *     >= 0 para sucesso,
 *       -1 para EOF ou falha (a falha foi relatada) ou
 *       -2 para linha com falha.
 */
static void readconfiggeneric(adns_state ads, const char *filename, int (*getline)(adns_state ads, getline_ctx*, const char *filename, int lno, char *buf, int buflen), getline_ctx gl_ctx)
{
    char linebuf[2000];
    char *p;
    char *q;
    int lno;
    int l;
    int dirl;
    const struct configcommandinfo *ccip;

    for (lno = 1; (l = getline(ads, &gl_ctx, filename, lno, linebuf, sizeof(linebuf))) != -1; lno++)
    {
        if (l == -2)
        {
            continue;
        }

        while (l > 0 && ctype_whitespace(linebuf[l - 1]))
        {
            l--;
        }

        linebuf[l] = 0;
        p = linebuf;

        while (ctype_whitespace(*p))
        {
            p++;
        }

        if (*p == '#' || *p == ';' || !*p)
        {
            continue;
        }

        q = p;

        while (*q && !ctype_whitespace(*q))
        {
            q++;
        }

        dirl = q - p;
        for (ccip = configcommandinfos; ccip->name && !(strlen(ccip->name) == dirl && !memcmp(ccip->name, p, q - p)); ccip++)
        {
        }

        if (!ccip->name)
        {
            if (ads->config_report_unknown)
            {
                adns__diag(ads, -1, 0, "%s:%d: unknown configuration directive `%.*s'", filename, lno, (int)(q - p), p);
            }

            continue;
        }

        while (ctype_whitespace(*q))
        {
            q++;
        }

        ccip->fn(ads, filename, lno, q);
    }
}

/**
 *
 */
static const char *instrum_getenv(adns_state ads, const char *envvar)
{
    const char *value;
    value = getenv(envvar);

    if (!value)
    {
        adns__debug(ads, -1, 0, "environment variable %s not set", envvar);
    } else
    {
        adns__debug(ads, -1, 0, "environment variable %s set to `%s'", envvar, value);
    }

    return value;
}

/**
 *
 */
static void readconfig(adns_state ads, const char *filename, int warnmissing)
{
    getline_ctx gl_ctx;
    gl_ctx.file = fopen(filename, "r");

    if (!gl_ctx.file)
    {
        if (errno == ENOENT)
        {
            if (warnmissing)
            {
                adns__debug(ads, -1, 0, "configuration file `%s' does not exist", filename);
            }

            return;
        }

        saveerr(ads, errno);
        adns__diag(ads, -1, 0, "cannot open configuration file `%s': %s", filename, strerror(errno));

        return;
    }

    readconfiggeneric(ads, filename, gl_file, gl_ctx);
    fclose(gl_ctx.file);
}

/**
 *
 */
static void readconfigtext(adns_state ads, const char *text, const char *showname)
{
    getline_ctx gl_ctx;
    gl_ctx.text= text;

    readconfiggeneric(ads, showname, gl_text, gl_ctx);
}

/**
 *
 */
static void readconfigenv(adns_state ads, const char *envvar)
{
    const char *filename;

    if (ads->iflags & adns_if_noenv)
    {
        adns__debug(ads, -1, 0, "not checking environment variable `%s'", envvar);

        return;
    }

    filename = instrum_getenv(ads, envvar);

    if (filename)
    {
        readconfig(ads, filename, 1);
    }
}

/**
 *
 */
static void readconfigenvtext(adns_state ads, const char *envvar)
{
    const char *textdata;

    if (ads->iflags & adns_if_noenv)
    {
        adns__debug(ads, -1, 0, "not checking environment variable `%s'", envvar);

        return;
    }

    textdata = instrum_getenv(ads, envvar);

    if (textdata)
    {
        readconfigtext(ads, textdata, envvar);
    }
}

/**
 *
 */
int adns__setnonblock(adns_state ads, int fd)
{
    int r;

    r = fcntl(fd, F_GETFL, 0);

    if (r < 0)
    {
        return errno;
    }

    r |= O_NONBLOCK;
    r = fcntl(fd,F_SETFL,r);

    if (r < 0)
    {
        return errno;
    }

    return 0;
}

/**
 *
 */
static int init_begin(adns_state *ads_r, adns_initflags flags, adns_logcallbackfn *logfn, void *logfndata)
{
    adns_state ads;
    pid_t pid;

    if (flags & ~(adns_initflags)(0x4fff))
    {
        /**
         * 0x4000 é reservado para expansão futura.
         */
        return ENOSYS;
    }

    ads = malloc(sizeof(*ads));

    if (!ads)
    {
        return errno;
    }

    ads->iflags = flags;
    ads->logfn = logfn;
    ads->logfndata = logfndata;
    ads->configerrno = 0;

    LIST_INIT(ads->udpw);
    LIST_INIT(ads->tcpw);
    LIST_INIT(ads->childw);
    LIST_INIT(ads->output);
    LIST_INIT(ads->intdone);

    ads->forallnext = 0;
    ads->nextid = 0x311f;
    ads->nudpsockets = 0;
    ads->tcpsocket = -1;
    adns__vbuf_init(&ads->tcpsend);
    adns__vbuf_init(&ads->tcprecv);

    ads->tcprecv_skip = 0;
    ads->nservers = ads->nsortlist = ads->nsearchlist = ads->tcpserver = 0;
    ads->searchndots = 1;
    ads->tcpstate = server_disconnected;
    timerclear(&ads->tcptimeout);

    ads->searchlist = 0;
    ads->config_report_unknown = 1;
    pid = getpid();
    ads->rand48xsubi[0] = pid;
    ads->rand48xsubi[1] = (unsigned long)pid >> 16;
    ads->rand48xsubi[2] = pid ^ ((unsigned long)pid >> 16);
    *ads_r = ads;

    return 0;
}

/**
 *
 */
static int init_finish(adns_state ads)
{
    struct sockaddr_in sin;
    struct protoent *proto;
    struct udpsocket *udp;

    int i;
    int r;

    if (!ads->nservers)
    {
        if (ads->logfn && ads->iflags & adns_if_debug)
        {
            adns__lprintf(ads, "adns: no nameservers, using IPv4 localhost\n");
        }

        memset(&sin, 0, sizeof(sin));

        sin.sin_family = AF_INET;
        sin.sin_port = htons(DNS_PORT);
        sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addserver(ads,(struct sockaddr *)&sin, sizeof(sin));
    }

    proto = getprotobyname("udp");

    if (!proto)
    {
        r = ENOPROTOOPT;

        /**
         * Pular lá embaixo.
         */
        goto x_free;
    }

    ads->nudpsockets = 0;
    for (i = 0; i < ads->nservers; i++)
    {
        if (adns__udpsocket_by_af(ads, ads->servers[i].addr.sa.sa_family))
        {
            continue;
        }

        assert(ads->nudpsockets < MAXUDP);
        udp = &ads->udpsockets[ads->nudpsockets];
        udp->af = ads->servers[i].addr.sa.sa_family;
        udp->fd = socket(udp->af, SOCK_DGRAM, proto->p_proto);

        if (udp->fd < 0)
        {
            r = errno;

            /**
             * Pular lá embaixo.
             */
            goto x_free;
        }

        ads->nudpsockets++;
        r = adns__setnonblock(ads, udp->fd);

        if (r)
        {
            r = errno;

            /**
             * Pular lá embaixo.
             */
            goto x_closeudp;
        }
    }

    return 0;

    x_closeudp:
        for (i = 0; i < ads->nudpsockets; i++)
        {
            close(ads->udpsockets[i].fd);
        }

    x_free:
        free(ads);

        return r;
}

/**
 *
 */
static void init_abort(adns_state ads)
{
    if (ads->nsearchlist)
    {
        free(ads->searchlist[0]);
        free(ads->searchlist);
    }

    free(ads);
}

/**
 *
 */
static void logfn_file(adns_state ads, void *logfndata, const char *fmt, va_list al)
{
    vfprintf(logfndata,fmt,al);
}

/**
 *
 */
static int init_files(adns_state *ads_r, adns_initflags flags, adns_logcallbackfn *logfn, void *logfndata)
{
    adns_state ads;
    const char *res_options;
    const char *adns_res_options;
    int r;

    r = init_begin(&ads, flags, logfn, logfndata);

    if (r)
    {
        return r;
    }

    res_options = instrum_getenv(ads, "RES_OPTIONS");
    adns_res_options = instrum_getenv(ads, "ADNS_RES_OPTIONS");
    ccf_options(ads, "RES_OPTIONS", -1, res_options);
    ccf_options(ads, "ADNS_RES_OPTIONS", -1, adns_res_options);
    readconfig(ads, "/etc/resolv.conf", 1);
    readconfig(ads, "/etc/resolv-adns.conf", 0);
    readconfigenv(ads, "RES_CONF");
    readconfigenv(ads, "ADNS_RES_CONF");
    readconfigenvtext(ads, "RES_CONF_TEXT");
    readconfigenvtext(ads, "ADNS_RES_CONF_TEXT");
    ccf_options(ads, "RES_OPTIONS", -1, res_options);
    ccf_options(ads, "ADNS_RES_OPTIONS", -1, adns_res_options);
    ccf_search(ads, "LOCALDOMAIN", -1, instrum_getenv(ads, "LOCALDOMAIN"));
    ccf_search(ads, "ADNS_LOCALDOMAIN", -1, instrum_getenv(ads, "ADNS_LOCALDOMAIN"));

    if (ads->configerrno && ads->configerrno != EINVAL)
    {
        r = ads->configerrno;
        init_abort(ads);

        return r;
    }

    r = init_finish(ads);

    if (r)
    {
        return r;
    }

    adns__consistency(ads, 0, cc_exit);
    *ads_r = ads;

    return 0;
}

/**
 *
 */
int adns_init(adns_state *ads_r, adns_initflags flags, FILE *diagfile)
{
    return init_files(ads_r, flags, logfn_file, diagfile ? diagfile : stderr);
}

/**
 *
 */
static int init_strcfg(adns_state *ads_r, adns_initflags flags, adns_logcallbackfn *logfn, void *logfndata, const char *configtext)
{
    adns_state ads;
    int r;

    r = init_begin(&ads, flags, logfn, logfndata);

    if (r)
    {
        return r;
    }

    readconfigtext(ads, configtext, "<supplied configuration text>");

    if (ads->configerrno)
    {
        r = ads->configerrno;
        init_abort(ads);

        return r;
    }

    r = init_finish(ads);

    if (r)
    {
        return r;
    }

    adns__consistency(ads, 0, cc_exit);
    *ads_r = ads;

    return 0;
}

/**
 *
 */
int adns_init_strcfg(adns_state *ads_r, adns_initflags flags, FILE *diagfile, const char *configtext)
{
    return init_strcfg(ads_r, flags, diagfile ? logfn_file : 0, diagfile, configtext);
}

/**
 * const char *configtext    ~> 0=> usar arquivos de configuração padrão.
 * adns_logcallbackfn *logfn ~> 0=> logfndata é um arquivo FILE*.
 * void *logfndata           ~> 0 com logfn == 0 => discard.
 */
int adns_init_logfn(adns_state *newstate_r, adns_initflags flags, const char *configtext, adns_logcallbackfn *logfn, void *logfndata)
{
    if (!logfn && logfndata)
    {
        logfn = logfn_file;
    }

    if (configtext)
    {
        return init_strcfg(newstate_r, flags, logfn, logfndata, configtext);
    } else
    {
        return init_files(newstate_r, flags, logfn, logfndata);
    }
}

/**
 *
 */
static void cancel_all(adns_query qu)
{
    if (!qu->parent)
    {
        adns__cancel(qu);
    } else
    {
        cancel_all(qu->parent);
    }
}

/**
 *
 */
void adns_finish(adns_state ads)
{
    int i;
    adns__consistency(ads, 0, cc_enter);

    for (;;)
    {
        if (ads->udpw.head)
        {
            cancel_all(ads->udpw.head);
        } else if (ads->tcpw.head)
        {
            cancel_all(ads->tcpw.head);
        } else if (ads->childw.head)
        {
            cancel_all(ads->childw.head);
        } else if (ads->output.head)
        {
            cancel_all(ads->output.head);
        } else if (ads->intdone.head)
        {
            cancel_all(ads->output.head);
        } else
        {
            break;
        }
    }

    for (i = 0; i < ads->nudpsockets; i++)
    {
        close(ads->udpsockets[i].fd);
    }

    if (ads->tcpsocket >= 0)
    {
        close(ads->tcpsocket);
    }

    adns__vbuf_free(&ads->tcpsend);
    adns__vbuf_free(&ads->tcprecv);
    freesearchlist(ads);
    free(ads);
}

/**
 *
 */
void adns_forallqueries_begin(adns_state ads)
{
    adns__consistency(ads, 0, cc_enter);
    ads->forallnext =
        ads->udpw.head ? ads->udpw.head :
        ads->tcpw.head ? ads->tcpw.head :
        ads->childw.head ? ads->childw.head :
        ads->output.head;
}

/**
 *
 */
adns_query adns_forallqueries_next(adns_state ads, void **context_r)
{
    adns_query qu, nqu;
    adns__consistency(ads, 0, cc_enter);
    nqu = ads->forallnext;

    for (;;)
    {
        qu = nqu;

        if (!qu)
        {
            return 0;
        }

        if (qu->next)
        {
            nqu = qu->next;
        } else if (qu == ads->udpw.tail)
        {
            nqu =
                ads->tcpw.head ? ads->tcpw.head :
                ads->childw.head ? ads->childw.head :
                ads->output.head;
        } else if (qu == ads->tcpw.tail)
        {
            nqu =
                ads->childw.head ? ads->childw.head :
                ads->output.head;
        } else if (qu == ads->childw.tail)
        {
            nqu = ads->output.head;
        } else
        {
            nqu = 0;
        }

        if (!qu->parent)
        {
            break;
        }
    }

    ads->forallnext = nqu;

    if (context_r)
    {
        *context_r = qu->ctx.ext;
    }

    return qu;
}
