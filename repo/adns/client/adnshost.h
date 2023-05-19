/**
 * adnshost.h - programa cliente resolvedor de uso geral útil,
 *     arquivo de cabeçalho.
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

#ifndef ADNSHOST_H_INCLUDED
#define ADNSHOST_H_INCLUDED

    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <errno.h>
    #include <signal.h>
    #include <stdarg.h>
    #include <stdlib.h>
    #include <assert.h>
    #include <time.h>

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>

    /**
     *
     */
    #define ADNS_FEATURE_MANYAF

    #include "config.h"
    #include "adns.h"
    #include "dlist.h"
    #include "client.h"

    #ifdef ADNS_REGRESS_TEST
    #include "hredirect.h"
    #endif

    /**
     * Declarações relacionadas ao processamento de opções.
     */

    /**
     *
     */
    struct optioninfo;

    /**
     *
     */
    typedef void optfunc(const struct optioninfo *oi, const char *arg, const char *arg2);

    /**
     *
     */
    struct optioninfo
    {
        /**
         *
         */
        enum oi_type
        {
            ot_end,
            ot_desconly,
            ot_flag,
            ot_value,
            ot_func,
            ot_funcarg,
            ot_funcarg2
        } type;

        /**
         *
         */
        const char *desc;

        /**
         *
         */
        const char *sopt,
                   *lopt;

        /**
         *
         */
        int *storep, value;

        /**
         *
         */
        optfunc *func;

        /**
         *
         */
        const char *argdesc,
                   *argdesc2;
    };

    /**
     *
     */
    enum ttlmode
    {
        tm_none,
        tm_rel,
        tm_abs
    };

    /**
     *
     */
    enum outputformat
    {
        fmt_default,
        fmt_simple,
        fmt_inline,
        fmt_asynch
    };

    /**
     *
     */
    struct perqueryflags_remember
    {
         /**
          *
          */
        int show_owner,
            show_type,
            show_cname;

         /**
          *
          */
        int ttl;
    };

    /**
     *
     */
    extern int ov_env,
        ov_pipe,
        ov_asynch;

    /**
     *
     */
    extern int ov_verbose;

    /**
     *
     */
    extern adns_rrtype ov_type;

    /**
     *
     */
    extern int ov_search,
        ov_qc_query,
        ov_qc_anshost,
        ov_qc_cname;

    /**
     *
     */
    extern int ov_tcp,
        ov_cname,
        ov_afflags,
        ov_v6map,
        ov_format;

    /**
     *
     */
    extern int ov_checkc;

    /**
     *
     */
    extern char *ov_id;

    /**
     *
     */
    extern struct perqueryflags_remember ov_pqfr;

    /**
     *
     */
    extern optfunc of_config,
        of_version,
        of_help,
        of_type,
        of_ptr,
        of_reverse;

    /**
     *
     */
    extern optfunc of_asynch_id, of_cancel_id;

    /**
     *
     */
    const struct optioninfo *opt_findl(const char *opt);

    /**
     *
     */
    const struct optioninfo *opt_finds(const char **optp);

    /**
     *
     */
    void opt_do(const struct optioninfo *oip, int invert, const char *arg, const char *arg2);

    /**
     * Declarações relacionadas ao processamento de consultas.
     */

    /**
     *
     */
    struct query_node
    {
        /**
         *
         */
        struct query_node *next, *back;

        /**
         *
         */
        struct perqueryflags_remember pqfr;

        /**
         *
         */
        char *id, *owner;

        /**
         *
         */
        adns_query qu;
    };

    /**
     *
     */
    extern adns_state ads;

    /**
     *
     */
    extern struct outstanding_list
    {
        /**
         *
         */
        struct query_node *head, *tail;
    } outstanding;

    /**
     *
     */
    void ensure_adns_init(void);

    /**
     *
     */
    void query_do(const char *domain);

    /**
     *
     */
    void query_done(struct query_node *qun, adns_answer *answer);

    /**
     *
     */
    void type_info(adns_rrtype type, const char **typename_r, const void *datap, char **data_r);

    /**
     * Wrapper para adns_rr_info que usa um buffer estático para fornecer
     * *typename_r para adns_r_unknown.
     */

    /**
     * Declarações relacionadas ao programa principal e funções utilitárias úteis.
     */

    /**
     *
     */
    void sysfail(const char *what, int errnoval) NONRETURNING;

    /**
     *
     */
    void usageerr(const char *what, ...) NONRETURNPRINTFFORMAT(1,2);

    /**
     *
     */
    void outerr(void) NONRETURNING;

    /**
     *
     */
    void *xmalloc(size_t sz);

    /**
     *
     */
    char *xstrsave(const char *str);

    /**
     *
     */
    extern int rcode;

    /**
     * 0 => uso padrão.
     */
    extern const char *config_text;

#endif
