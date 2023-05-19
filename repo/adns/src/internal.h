/**
 * internal.h
 *     - declarações de objetos privados com ligação externa (adns__*).
 *     - definições de macros internas.
 *     - comentários sobre as estruturas de dados da biblioteca.
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

#ifndef ADNS_INTERNAL_H_INCLUDED
#define ADNS_INTERNAL_H_INCLUDED

    /**
     *
     */
    #include "config.h"

    /**
     *
     */
    typedef unsigned char byte;

    #include <stdarg.h>
    #include <assert.h>
    #include <unistd.h>
    #include <signal.h>
    #include <errno.h>
    #include <string.h>
    #include <stdlib.h>
    #include <stdbool.h>

    #include <sys/time.h>

    /**
     *
     */
    #define ADNS_FEATURE_MANYAF

    #include "adns.h"
    #include "dlist.h"

    /**
     *
     */
    #ifdef ADNS_REGRESS_TEST
        /**
         *
         */
        #include "hredirect.h"
    #endif

    /**
     * Configuração e constantes.
     */

    /**
     * não aumente além do n. de bits em `unsigned'!
     */
    #define MAXSERVERS 5

    #define MAXSORTLIST 15
    #define UDPMAXRETRIES 15
    #define UDPRETRYMS 2000
    #define TCPWAITMS 30000
    #define TCPCONNMS 14000
    #define TCPIDLEMS 30000

    /**
     * qualquer TTL > 7 dias é limitado.
     */
    #define MAXTTLBELIEVE (7 * 86400)

    #define DNS_PORT 53
    #define DNS_MAXUDP 512
    #define DNS_MAXLABEL 63
    #define DNS_MAXDOMAIN 255
    #define DNS_HDRSIZE 12
    #define DNS_IDOFFSET 0
    #define DNS_CLASS_IN 1

    #define MAX_POLLFDS ADNS_POLLFDS_RECOMMENDED

    /**
     * Alguns bandidos de pré-processador.
     */

    #define GLUE(x, y) GLUE_(x, y)
    #define GLUE_(x, y) x##y

    /**
     * A macro C99 `...' deve corresponder a pelo menos um argumento, portanto, a definição
     * errada de `#define CAR(car, ...) car' está errado. Mas é fácil fazer com que a coleção
     * não esteja vazia se formos descartá-la.
     */
    #define CAR(...) CAR_(__VA_ARGS__, _)
    #define CAR_(car, ...) car

    /**
     * Extrair o final de uma lista de argumentos é bem mais difícil. O truque a seguir
     * é baseado em um de Unicornios Felizes para contar o número de argumentos para
     * uma macro, simplificado de duas maneiras: (a) ele lida com até oito argumentos
     * e (b) precisa apenas distinguir o caso de um argumento de muitos argumentos.
     */
    #define CDR(...) CDR_(__VA_ARGS__, m, m, m, m, m, m, m, 1, _)(__VA_ARGS__)
    #define CDR_(_1, _2, _3, _4, _5, _6, _7, _8, n, ...) CDR_##n
    #define CDR_1(_)
    #define CDR_m(_, ...) __VA_ARGS__

    /**
     *
     */
    typedef enum {
        cc_user,
        cc_enter,
        cc_exit,
        cc_freq
    } consistency_checks;

    /**
     *
     */
    typedef enum {
        rcode_noerror,
        rcode_formaterror,
        rcode_servfail,
        rcode_nxdomain,
        rcode_notimp,
        rcode_refused
    } dns_rcode;

    /**
     *
     */
    enum {
        /**
         * A consulta addr recebeu uma resposta.
         */
        adns__qf_addr_answer = 0x01000000,

        /**
         * addr subconsulta executada em cname.
         */
        adns__qf_addr_cname = 0x02000000
    };

    /**
     * Estruturas de dados compartilhados.
     */

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        int used, avail;

        /**
         *
         */
        byte *buf;
    } vbuf;

    /**
     *
     */
    typedef struct
    {
        adns_state ads;
        adns_query qu;

        int serv;
        const byte *dgram;

        int dglen, nsstart, nscount, arcount;
        struct timeval now;
    } parseinfo;

    /**
     * mantenha-se sincronizado com addrfam!
     */
    #define MAXREVLABELS 34

    /**
     *
     */
    struct revparse_state
    {
        /**
         *
         */
        uint16_t labstart[MAXREVLABELS];

        /**
         *
         */
        uint8_t lablen[MAXREVLABELS];
    };

    /**
     *
     */
    union checklabel_state
    {
        /**
         *
         */
        struct revparse_state ptr;
    };

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        void *ext;

        /**
         *
         */
        void (*callback)(adns_query parent, adns_query child);

        /**
         * Estado específico do tipo para a própria consulta: zero-init
         * se você não souber melhor.
         */
        union
        {
            /**
             *
             */
            struct
            {
                /**
                 *
                 */
                adns_rrtype rev_rrtype;

                /**
                 *
                 */
                adns_sockaddr addr;
            } ptr;

            struct
            {
                /**
                 *
                 */
                unsigned want, have;
            } addr;
        } tinfo;

        /**
         * Estado para uso pela função de retorno de chamada do nível
         * mais alto.
         */
        union
        {
            /**
             *
             */
            adns_rr_hostaddr *hostaddr;
        } pinfo;
    } qcontext;

    /**
     *
     */
    typedef struct typeinfo
    {
        /**
         *
         */
        adns_rrtype typekey;

        /**
         *
         */
        const char *rrtname;

        /**
         *
         */
        const char *fmtname;

        /**
         *
         */
        int fixed_rrsz;

        /**
         * Altere o gerenciamento de dados de *data.
         * Anteriormente, usava alloc_interim, agora usa alloc_final.
         */
        void (*makefinal)(adns_query qu, void *data);

        /**
         * Converte os dados RR em uma representação de string em vbuf.
         * vbuf será anexado (deve ter sido inicializado) e não será
         * terminado em null por convstring.
         */
        adns_status (*convstring)(vbuf *vb, adns_rrtype, const void *data);

        /**
         * Analisa um RR, em dgram de comprimento dglen, começando em
         * cbyte e estendendo-se até no máximo max.
         *
         * O RR deve ser armazenado em *store_r, de comprimento qu->typei->getrrsz().
         *
         * Se houver um excesso que possa indicar truncamento, ele deve definir
         * *rdstart como -1; caso contrário, pode configurá-lo para qualquer
         * outra coisa positiva.
         *
         * nsstart é o deslocamento da seção de autoridade.
         */
        adns_status (*parse)(const parseinfo *pai, int cbyte, int max, void *store_r);

        /**
         * Retorna !0 se RR a deve ser estritamente após RR b na ordem de
         * classificação, 0 caso contrário. Não deve falhar.
         */
        int (*diff_needswap)(adns_state ads,const void *datap_a,const void *datap_b);

        /**
         * Verifique um rótulo da string de domínio de consulta. O rótulo não é necessariamente
         * terminado em nulo. A entrada do plug pode recusar o envio da consulta retornando um
         * status diferente de zero. O estado pode ser armazenado em *cls entre chamadas e
         * informações úteis podem ser armazenadas em ctx->tinfo, para serem armazenadas com a
         * consulta (por exemplo, estarão disponíveis para a entrada do plug de análise). Essa
         * entrada de plug pode detectar uma primeira chamada porque labnum é zero e uma chamada
         * final porque lablen é zero.
         */
        adns_status (*checklabel)(adns_state ads, adns_queryflags flags, union checklabel_state *cls, qcontext *ctx, int labnum, const char *dgram, int labstart, int lablen);

        /**
         * Chamado imediatamente após a classificação dos RRs e pode reorganizá-los. (Isso
         * é realmente para o benefício do material diferenciado de ponderação do SRV.).
         * Pode ser 0 para significar que nenhum processamento precisa ser feito.
         */
        void (*postsort)(adns_state ads, void *array, int nrrs, int rrsz, const struct typeinfo *typei);

        /**
         * Retorne o tamanho do elemento de registro de recurso de saída; se for
         * nulo, o membro rrsz poderá ser usado.
         */
        int (*getrrsz)(const struct typeinfo *typei, adns_rrtype type);

        /**
         * Envie a consulta para os fornecedores de nomes e conecte-a à fila
         * apropriada. O comportamento normal é chamar adns__query_send, mas
         * isso pode ser substituído para efeitos especiais.
         */
        void (*query_send)(adns_query qu, struct timeval now);
    } typeinfo;

    /**
     * Implementado em query.c, usado por types.c como padrão e como parte da
     * implementação para alguns tipos mais sofisticados não requer
     * nenhum estado.
     */
    adns_status adns__ckl_hostname(adns_state ads, adns_queryflags flags, union checklabel_state *cls, qcontext *ctx, int labnum, const char *dgram, int labstart, int lablen);

    /**
     *
     */
    typedef struct allocnode
    {
        /**
         *
         */
        struct allocnode *next, *back;

        /**
         *
         */
        size_t sz;
    } allocnode;

    /**
     *
     */
    union maxalign
    {
        /**
         *
         */
        byte d[1];

        /**
         *
         */
        struct in_addr ia;

        /**
         *
         */
        long l;

        /**
         *
         */
        void *p;

        /**
         *
         */
        void (*fp)(void);

        /**
         *
         */
        union maxalign *up;
    };

    /**
     *
     */
    struct adns__query
    {
        /**
         *
         */
        adns_state ads;

        /**
         *
         */
        enum
        {
            query_tosend,
            query_tcpw,
            query_childw,
            query_done
        } state;

        /**
         *
         */
        adns_query back, next, parent;

        /**
         *
         */
        struct
        {
            /**
             *
             */
            adns_query head, tail;
        } children;

        /**
         *
         */
        struct
        {
            /**
             *
             */
            adns_query back, next;
        } siblings;

        /**
         *
         */
        struct
        {
            /**
             *
             */
            allocnode *head, *tail;
        } allocations;

        /**
         *
         */
        int interim_allocd, preserved_allocd;

        /**
         *
         */
        void *final_allocspace;

        /**
         *
         */
        const typeinfo *typei;

        /**
         *
         */
        byte *query_dgram;

        /**
         *
         */
        int query_dglen;

        /**
         * Buffer de prostíbulo de uso geral. Onde quer que uma interface 'grande' seja
         * cruzada, ela pode ser inválida/alterada, a menos que especificado de
         * outra forma.
         */
        vbuf vb;

        /**
         * Isso é alocado quando uma consulta é enviada, para evitar que não possamos
         * relacionar erros às consultas se ficarmos sem dados. Durante o status de
         * processamento da consulta, rrs é 0. cname é definido se encontrarmos um
         * cname (isso corresponde a cname_dgram na estrutura da consulta). o tipo
         * é definido desde o início. nrrs e rrs são colocados juntos, quando
         * descobrimos quantos rrs existem. owner é definido durante a consulta,
         * a menos que estejamos fazendo uma lista de pesquisa; nesse caso, ele é
         * definido apenas quando encontramos uma resposta.
         */
        adns_answer *answer;

        /**
         * Se não-0, foi alocado usando.
         */
        byte *cname_dgram;
        int cname_dglen, cname_begin;

        /**
         * Usado pelo algoritmo de busca. O domínio de consulta na forma textual é copiado
         * para o vbuf e _origlen definido para seu comprimento. Em seguida, percorremos a
         * lista de pesquisa, se quisermos. _pos diz onde estamos (próxima entrada para
         * tentar) e _doneabs diz se já fizemos a consulta absoluta (0=ainda não, 1=concluído,
         * -1=preciso fazer imediatamente, mas ainda não o fiz). Se os sinalizadores não
         * tiverem adns_qf_search, o vbuf será inicializado, mas vazio e todo o resto será
         * zero.
         */
        vbuf search_vb;
        int search_origlen, search_pos, search_doneabs;

        /**
         *
         */
        int id, flags, retries;

        /**
         *
         */
        int udpnextserver;

        /**
         * bitmap indexado pelo servidor.
         */
        unsigned long udpsent;

        /**
         *
         */
        int timeout_ms;

        /**
         *
         */
        struct timeval timeout_started;

        /**
         * Tempo de expiração mais cedo de qualquer registro que usamos.
         */
        time_t expires;

        /**
         *
         */
        qcontext ctx;

        /**
         * Estados possíveis:
         *     state   Queue   child  id   nextudpserver  udpsent     tcpfailed
         *
         *     tosend  NONE    null   >=0  0              zero        zero
         *     tosend  udpw    null   >=0  any            nonzero     zero
         *     tosend  NONE    null   >=0  any            nonzero     zero
         *
         *     tcpw    tcpw    null   >=0  irrelevant     any         any
         *
         *     child   childw  set    >=0  irrelevant     irrelevant  irrelevant
         *     child   NONE    null   >=0  irrelevant     irrelevant  irrelevant
         *     done    output  null   -1   irrelevant     irrelevant  irrelevant
         *
         * As consultas só não estão em uma fila quando estão realmente sendo
         * processadas. As consultas no estado tcpw/tcpw foram enviadas (ou
         * estão no buffer de envio) se a conexão tcp estiver no estado
         * server_ok.
         *
         * As consultas internas (de adns__submit_internal) terminam em intdone
         * em vez de saída, e os retornos de chamada são feitos na saída de adns,
         * para evitar riscos de reentrada.
         *
         *            +------------------------+
         *             START -----> |      tosend/NONE       |
         *            +------------------------+
         *                         /                       |\  \
         *        too big for UDP /             UDP timeout  \  \ send via UDP
         *        send via TCP   /              more retries  \  \
         *        when conn'd   /                  desired     \  \
         *                     |                                |  |
         *                     v          |  v
         *              +-----------+                 +-------------+
         *              | tcpw/tcpw | ________                | tosend/udpw |
         *              +-----------+         \       +-------------+
         *                 |    |              |     UDP timeout | |
         *                 |    |              |      no more    | |
         *                 |    |              |      retries    | |
         *                  \   | TCP died     |      desired    | |
         *                   \   \ no more     |                 | |
         *                    \   \ servers    | TCP            /  |
         *                     \   \ to try    | timeout       /   |
         *                  got \   \          v             |_    | got
         *                 reply \   _| +------------------+      / reply
         *                        \     | done/output FAIL |     /
         *                         \    +------------------+    /
         *                          \                          /
         *                           _|                      |_
         *                             (..... got reply ....)
         *                              /                   \
         *        need child query/ies /                     \ no child query
         *                            /                       \
         *                          |_                         _|
         *       +---------------+           +----------------+
         *               | childw/childw | ----------------> | done/output OK |
         *               +---------------+  children done    +----------------+
         */
    };

    /**
     *
     */
    struct query_queue
    {
        /**
         *
         */
        adns_query head, tail;
    };

    /**
     *
     */
    #define MAXUDP 2

    /**
     *
     */
    struct adns__state
    {
        /**
         *
         */
        adns_initflags iflags;

        /**
         *
         */
        adns_logcallbackfn *logfn;

        /**
         *
         */
        void *logfndata;

        /**
         *
         */
        int configerrno;

        /**
         *
         */
        struct query_queue udpw, tcpw, childw, output, intdone;

        /**
         *
         */
        adns_query forallnext;

        /**
         *
         */
        unsigned nextid;

        /**
         *
         */
        int tcpsocket;

        /**
         *
         */
        struct udpsocket
        {
            /**
             *
             */
            int af;

            /**
             *
             */
            int fd;
        } udpsockets[MAXUDP];

        /**
         *
         */
        int nudpsockets;

        /**
         *
         */
        vbuf tcpsend, tcprecv;

        /**
         *
         */
        int nservers, nsortlist, nsearchlist, searchndots, tcpserver, tcprecv_skip;

        /**
         *
         */
        enum adns__tcpstate
        {
            /**
             *
             */
            server_disconnected,

            /**
             *
             */
            server_connecting,

            /**
             *
             */
            server_ok,

            /**
             *
             */
            server_broken
        } tcpstate;

        /**
         *
         */
        struct timeval tcptimeout;

        /**
         * Isso terá tv_sec==0 se não for válido. Sempre será válido se tcpstate _connecting.
         * Quando _ok, será diferente de zero se estivermos ociosos (ou seja, a fila tcpw está
         * vazia), caso em que é o tempo absoluto em que fecharemos a conexão.
         */

        /**
         *
         */
        struct sigaction stdsigpipe;

        /**
         *
         */
        sigset_t stdsigmask;

        /**
         *
         */
        struct pollfd pollfds_buf[MAX_POLLFDS];

        /**
         *
         */
        adns_rr_addr servers[MAXSERVERS];

        /**
         *
         */
        struct sortlist
        {
            /**
             *
             */
            adns_sockaddr base, mask;
        } sortlist[MAXSORTLIST];

        /**
         *
         */
        char **searchlist;

        /**
         *
         */
        unsigned config_report_unknown:1;

        /**
         *
         */
        unsigned short rand48xsubi[3];
    };

    /**
     * Do addrfam.c:
     */

    /**
     * Retorna diferente de zero, a família de a é bf e o campo de endereço de protocolo de a
     * refere-se ao mesmo endereço de protocolo armazenado em ba.
     */
    extern int adns__addrs_equal_raw(const struct sockaddr *a, int bf, const void *b);

    /**
     * Retorna diferente de zero se os dois se referem ao mesmo endereço de
     * protocolo (desconsiderando porta, escopo IPv6, etc).
     */
    extern int adns__addrs_equal(const adns_sockaddr *a, const adns_sockaddr *b);

    /**
     * Retorna diferente de zero se os dois endereços de plug forem iguais (em
     * todos os aspectos significativos).
     */
    extern int adns__sockaddrs_equal(const struct sockaddr *sa, const struct sockaddr *sb);

    /**
     * Retorna a largura dos endereços da família af, em bits.
     */
    extern int adns__addr_width(int af);

    /**
     * Armazena no campo de endereço do protocolo sa uma máscara de endereço para a
     * família de endereços af, cujos primeiros len bits são definidos e os restantes
     * são limpos. Na entrada, o campo af de sa deve ser definido. Isso é o que você
     * deseja para converter um comprimento de prefixo em uma máscara de rede.
     */
    extern void adns__prefix_mask(adns_sockaddr *sa, int len);

    /**
     * Dado um endereço base de rede, obtenha o tamanho do prefixo apropriado com base
     * nas regras apropriadas para a família de endereços (por exemplo, para IPv4, isso
     * usa as antigas classes de endereços).
     */
    extern int adns__guess_prefix_length(const adns_sockaddr *addr);

    /**
     * Retorna diferente de zero se o endereço de protocolo especificado por
     * af e addr estiver dentro da rede especificada por base e mask.
     */
    extern int adns__addr_matches(int af, const void *addr, const adns_sockaddr *base, const adns_sockaddr *mask);

    /**
     * Injeta o endereço de protocolo *a no endereço de plug sa. Assume
     * que sa->sa_family já está configurado corretamente.
     */
    extern void adns__addr_inject(const void *a, adns_sockaddr *sa);

    /**
     * Retorna o endereço do campo de endereço de protocolo em sa.
     */
    extern const void *adns__sockaddr_addr(const struct sockaddr *sa);

    /**
     * Converte sa em uma string e a grava em buf, que deve ter pelo menos
     * ADNS_ADDR2TEXT_BUFLEN bytes (desmarcado). Retorna buf; não pode falhar.
     */
    char *adns__sockaddr_ntoa(const struct sockaddr *sa, char *buf);

    /**
     * Construa uma string de domínio reverso, dado um endereço de plug e uma zona
     * mais alta. Se a zona for nula, use a zona de pesquisa inversa padrão para a
     * família de endereços. Se o comprimento da string resultante não for maior
     * que bufsz, o resultado será armazenado iniciando em *buf_io; caso contrário,
     * um novo buffer é alocado e um ponteiro para ele é armazenado em *buf_io e
     * *buf_free_r (o último dos quais deve ser nulo na entrada). Se algo der
     * errado, um valor errno será retornado: ENOSYS se a família de endereço
     * de sa não for reconhecida ou ENOMEM se a tentativa de alocar um buffer
     * de saída falhar.
     */
    extern int adns__make_reverse_domain(const struct sockaddr *sa, const char *zone, char **buf_io, size_t bufsz, char **buf_free_r);

    /**
     * Analisa um rótulo em um nome de domínio reverso, dado seu índice labnum
     * (começando de zero), um ponteiro para seu conteúdo (que não precisa ser
     * terminado em nulo) e seu comprimento. O estado em *rps é inicializado
     * implicitamente quando labnum é zero.
     *
     * Retorna 1 se a análise estiver ocorrendo com sucesso, 0 se o nome de
     * domínio for definitivamente inválido e a análise deve ser terminada.
     */
    extern bool adns__revparse_label(struct revparse_state *rps, int labnum, const char *dgram, int labstart, int lablen);

    /**
     * Termina a análise de um nome de domínio reverso, dado o número total de
     * rótulos no nome. Em caso de sucesso, preenche o af e o endereço de
     * protocolo em *addr_r e o tipo de consulta de encaminhamento em
     * *rrtype_r (porque isso acaba sendo útil). Retorna 1 se a análise
     * foi bem-sucedida.
     */
    extern bool adns__revparse_done(struct revparse_state *rps, const char *dgram, int nlabels, adns_rrtype *rrtype_r, adns_sockaddr *addr_r);

    /**
     * Do setup.c:
     */

    /**
     * Valor errno.
     */
    int adns__setnonblock(adns_state ads, int fd);

    /**
     * Do general.c:
     */

    /**
     *
     */
    void adns__vlprintf(adns_state ads, const char *fmt, va_list al);

    /**
     *
     */
    void adns__lprintf(adns_state ads, const char *fmt, ...) PRINTFFORMAT(2,3);

    /**
     *
     */
    void adns__vdiag(adns_state ads, const char *pfx, adns_initflags prevent, int serv, adns_query qu, const char *fmt, va_list al);

    /**
     *
     */
    void adns__debug(adns_state ads, int serv, adns_query qu, const char *fmt, ...) PRINTFFORMAT(4,5);

    /**
     *
     */
    void adns__warn(adns_state ads, int serv, adns_query qu, const char *fmt, ...) PRINTFFORMAT(4,5);

    /**
     *
     */
    void adns__diag(adns_state ads, int serv, adns_query qu, const char *fmt, ...) PRINTFFORMAT(4,5);

    /**
     *
     */
    int adns__vbuf_ensure(vbuf *vb, int want);

    /**
     * Não inclui nul.
     */
    int adns__vbuf_appendstr(vbuf *vb, const char *data);

    /**
     *
     */
    int adns__vbuf_append(vbuf *vb, const byte *data, int len);

    /**
     * 1 => success,
     * 0 => realloc failed.
     */

    /**
     *
     */
    void adns__vbuf_appendq(vbuf *vb, const byte *data, int len);

    /**
     *
     */
    void adns__vbuf_init(vbuf *vb);

    /**
     *
     */
    void adns__vbuf_free(vbuf *vb);

    /**
     * Desmarca um domínio em um datagrama e retorna uma string adequada para
     * imprimi-lo como. Nunca falha - se ocorrer uma falha, ele retornará
     * algum tipo de string descrevendo a falha.
     *
     * serv pode ser -1 e qu pode ser 0. vb deve ter sido inicializado e será
     * deixado em um estado consistente arbitrário.
     *
     * Retorna vb->buf ou um ponteiro para uma string literal.
     * Não modifique vb antes de usar o valor de retorno.
     */
    const char *adns__diag_domain(adns_state ads, int serv, adns_query qu, vbuf *vb, const byte *dgram, int dglen, int cbyte);

    /**
     * Função padrão para o plug do tipo `getrrsz'; retorna o valor `fixed_rrsz'
     * da entrada typeinfo.
     */
    int adns__getrrsz_default(const typeinfo *typei, adns_rrtype type);

    /**
     * Faz um tipo de inserção de array que deve conter objetos nobjs com cada sz
     * bytes de comprimento. tempbuf deve apontar para um buffer com pelo menos sz
     * bytes de comprimento. needswap deve retornar !0 se a>b (estritamente, ou seja,
     * ordem inválida) 0 se a<=b (ou seja, ordem está correta).
     */
    void adns__isort(void *array, int nobjs, int sz, void *tempbuf, int (*needswap)(void *context, const void *a, const void *b), void *context);

    /**
     *
     */
    void adns__sigpipe_protect(adns_state);

    /**
     * Se a proteção SIGPIPE não estiver desativada, bloqueará todos os sinais,
     * exceto SIGPIPE, e definirá a disposição do SIGPIPE como SIG_IGN. (E então
     * restaurar.) Cada chamada para _protect deve ser seguida por uma chamada
     * para _unprotect antes que qualquer quantidade significativa de código
     * seja processada, já que a antiga máscara de sinal é armazenada na
     * estrutura adns.
     */
    void adns__sigpipe_unprotect(adns_state);

    /**
     * Do transmit.c:
     */

    /**
     * Monta um pacote de consulta em vb. Um novo id é alocado e retornado.
     */
    adns_status adns__mkquery(adns_state ads, vbuf *vb, int *id_r, const char *owner, int ol, const typeinfo *typei, adns_rrtype type, adns_queryflags flags);

    /**
     * O mesmo que adns__mkquery, mas obtém o domínio proprietário de um
     * datagrama existente. Esse domínio deve estar correto e não truncado.
     */
    adns_status adns__mkquery_frdgram(adns_state ads, vbuf *vb, int *id_r, const byte *qd_dgram, int qd_dglen, int qd_begin, adns_rrtype type, adns_queryflags flags);

    /**
     * A consulta deve estar no estado tcpw/tcpw; ele será enviado, se possível,
     * e nenhum processamento adicional poderá ser feito nele por enquanto. A
     * conexão pode ser interrompida, mas nenhuma reconexão será tentada.
     */
    void adns__querysend_tcp(adns_query qu, struct timeval now);

    /**
     * Encontre a estrutura do plug UDP em ads que tenham a família de
     * endereços fornecida. Retorne null se não houver um.
     *
     * Isso é usado durante a inicialização, portanto, os ads são
     * preenchidos apenas parcialmente. Os requisitos são que o nudp
     * esteja definido e que o udpsocket[i].af esteja definido para
     * 0<=i<nudp.
     */
    struct udpsocket *adns__udpsocket_by_af(adns_state ads, int af);

    /**
     * A consulta deve estar no estado tosend/NONE; ele será movido
     * para um novo estado e nenhum processamento adicional poderá
     * ser feito nele por enquanto. (O estado resultante é udp/timew,
     * tcpwait/timew (se o servidor não estiver conectado), tcpsent/timew,
     * child/childw ou done/output.) __query_send pode decidir usar UDP ou
     * TCP dependendo se _qf_usevc está definido (ou tem torna-se definido)
     * e se a consulta é muito grande.
     */
    void adns__query_send(adns_query qu, struct timeval now);

    /**
     * Do query.c:
     */

    /**
     * Envia uma consulta (para uso interno, chamada durante
     * envios externos).
     *
     * A nova consulta é retornada em *query_r ou retornamos
     * adns_s_nomemory.
     *
     * O datagrama de consulta já deve ter sido montado em qumsg_vb;
     * os dados para ele é (_taken) assumida por esta rotina, quer
     * seja bem-sucedida ou falhe (se for bem-sucedida, o vbuf é
     * reutilizado para qu->vb).
     *
     * *ctx é copiado byte por byte na consulta. Antes de fazer isso,
     * seu campo tinfo pode ser modificado por entradas de plug
     * de tipo.
     *
     * Quando a consulta de sub processo terminar, ctx->callback será chamado.
     * O sub processo já terá sido retirado da lista global de consultas em
     * ads e da lista de sub processos no processo principal. O sub processo
     * será liberada quando o retorno de chamada retornar. O processo principal
     * terá sido retirado da fila global childw.
     *
     * O retorno de chamada deve chamar adns__query_done, se estiver completo,
     * ou adns__query_fail, se ocorrer uma falha, caso em que os outros sub
     * processos (se houver) serão cancelados. Se o processo principal tiver
     * mais sub processos em espera (ou acabou de enviar mais rotinas), o
     * retorno de chamada pode esperar por eles - ele deve então colocar
     * o processo principal de volta na fila de sub processos.
     */
    adns_status adns__internal_submit(adns_state ads, adns_query *query_r, adns_query parent, const typeinfo *typei, adns_rrtype type, vbuf *qumsg_vb, int id, adns_queryflags flags, struct timeval now, qcontext *ctx);

    /**
     * Percorre a lista de pesquisa para uma consulta com adns_qf_search.
     * A consulta deve ter apenas uma resposta negativa, ou ainda não ter
     * nenhuma consulta enviada, e não deve estar em nenhuma fila. O
     * query_dgram, se houver, será liberado e esquecido e um novo
     * será construído a partir dos membros search_* da consulta.
     *
     * Não pode falhar (em caso de erro, chama adns__query_fail).
     */
    void adns__search_next(adns_state ads, adns_query qu, struct timeval now);

    /**
     *
     */
    void *adns__alloc_interim(adns_query qu, size_t sz);

    /**
     * Aloca um pouco de dados e registra de qual consulta veio e
     * quanto havia.
     *
     * Se ocorrer um erro na consulta, todos os dados de _interim é
     * simplesmente liberada. Se a consulta for bem-sucedida, um buffer
     * grande será feito, o que é grande o suficiente para todas essas
     * alocações e, em seguida, adns__alloc_final obterá dados desse
     * buffer.
     *
     * _alloc_interim pode falhar (e retornar 0).
     * O chamador deve garantir que a consulta falhou.
     *
     * Os dados de _preserved é mantida e transferida para o buffer
     * maior - a menos que fiquemos sem dados, caso em que ela também
     * é liberada. Ao usar _preserved, você deve adicionar código ao
     * caso de saída de falha x_nomem em adns__makefinal_query para
     * limpar os ponteiros que você fez para essas alocações, porque
     * é quando eles são livres; você também deve fazer uma observação
     * na declaração dessas variáveis de ponteiro, para observar que
     * elas são _preserved em vez de _interim. Se eles estiverem na
     * resposta, anote aqui: answer->cname e answer->owner são
     * _preserved (Preservados).
     */
    void *adns__alloc_preserved(adns_query qu, size_t sz);

    /**
     * Transfere uma alocação temporária de uma consulta para outra, para
     * que a consulta `para' tenha espaço para os dados quando chegarmos
     * ao makefinal e para que o livre aconteça quando a consulta `para'
     * for liberada em vez da consulta `de' .
     *
     * É legal chamar adns__transfer_interim com um ponteiro nulo;
     * isso não tem efeito.
     *
     * _transfer_interim também garante que o tempo de expiração da consulta
     * `para' não seja posterior ao da consulta `de', de modo que os TTLs das
     * consultas de sub processo sejam extendido por processos principal.
     */
    void adns__transfer_interim(adns_query from, adns_query to, void *block);

    /**
     * Esqueça um bloco alocado por adns__alloc_interim.
     */
    void adns__free_interim(adns_query qu, void *p);

    /**
     * Semelhante á _interim, mas não registra o comprimento para copiar
     * posteriormente na resposta. Isso apenas garante que os dados será
     * liberado quando terminarmos a consulta.
     */
    void *adns__alloc_mine(adns_query qu, size_t sz);

    /**
     * Não pode falhar e não pode retornar 0.
     */
    void *adns__alloc_final(adns_query qu, size_t sz);

    /**
     *
     */
    void adns__makefinal_block(adns_query qu, void **blpp, size_t sz);

    /**
     *
     */
    void adns__makefinal_str(adns_query qu, char **strp);

    /**
     * Redefine todo o material de gerenciamento de dados etc. para levar em
     * consideração apenas o material _preserved de _alloc_preserved. Usado
     * quando encontramos uma falha em algum lugar e queremos apenas relatar
     * a falha (talvez com informações de CNAME, proprietário etc.) e também
     * quando estamos no meio de RRs em um datagrama e descobrimos que
     * precisamos repetir a consulta.
     */
    void adns__reset_preserved(adns_query qu);

    /**
     *
     */
    void adns__cancel(adns_query qu);

    /**
     *
     */
    void adns__query_done(adns_query qu);

    /**
     *
     */
    void adns__query_fail(adns_query qu, adns_status st);

    /**
     *
     */
    void adns__cancel_children(adns_query qu);

    /**
     * Deve ser chamado antes de retornar de adns sempre que tivermos progredido
     * (incluindo consultas feitas, concluídas ou destruídas).
     *
     * Pode reinserir adns por meio de retornos de chamada de consulta
     * interna, portanto, funções externas que chamam adns__returning
     * normalmente devem ser evitadas no código interno.
     */
    void adns__returning(adns_state ads, adns_query qu);

    /**
     *
     */
    void adns__intdone_process(adns_state ads);

    /**
     * Do reply.c:
     */

    /**
     * Esta função permite fazer com que novos datagramas sejam construídos e
     * enviados, ou mesmo novas consultas sejam iniciadas. No entanto, as
     * funções de envio de consulta não têm permissão para chamar nenhuma
     * função geral de loop de eventos, caso a chamem acidentalmente.
     *
     * Ou seja, funções de recebimento podem chamar funções de envio.
     * Funções de envio NÃO podem chamar funções de recebimento.
     */
    void adns__procdgram(adns_state ads, const byte *dgram, int len, int serv, int viatcp, struct timeval now);

    /**
     * Do types.c:
     */

    /**
     *
     */
    const typeinfo *adns__findtype(adns_rrtype type);

    /**
     * Do parse.c:
     */

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        adns_state ads;

        /**
         *
         */
        adns_query qu;

        /**
         *
         */
        int serv;

        /**
         *
         */
        const byte *dgram;

        /**
         *
         */
        int dglen, max, cbyte, namelen;

        /**
         *
         */
        int *dmend_r;
    } findlabel_state;

    /**
     * Localiza rótulos em um domínio em um datagrama.
     *
     * Chame esta rotina primeiro.
     * dmend_rlater pode ser nulo. ads (e, claro, fls) podem não ser.
     * serv pode ser -1, qu pode ser nulo - eles são para relatórios
     * de falhas.
     */
    void adns__findlabel_start(findlabel_state *fls, adns_state ads, int serv, adns_query qu, const byte *dgram, int dglen, int max, int dmbegin, int *dmend_rlater);

    /**
     * Em seguida, ligue para este repetidamente.
     *
     * Ele retornará adns_s_ok se tudo estiver bem e informará o comprimento
     * e o início dos rótulos sucessivos. labstart_r pode ser nulo, mas
     * lablen_r não deve ser.
     *
     * Após o último rótulo, retornará com *lablen_r zero.
     * Não ligue novamente; em vez disso, deixe limpo o findlabel_state.
     *
     * *dmend_rlater terá sido definido para apontar para a próxima parte do
     * datagrama após o rótulo (ou após a parte não compactada, se a compactação
     * foi usada). *namelen_rlater terá sido definido para o comprimento do nome
     * de domínio (comprimento total dos rótulos mais 1 para cada ponto intermediário).
     *
     * Se o datagrama parecer truncado, *lablen_r será -1.
     * *dmend_rlater, *labstart_r e *namelen_r podem conter conteúdo inválido.
     * Não chame _next novamente.
     *
     * Também pode haver falhas, caso em que *dmend_rlater, *namelen_rlater, *lablen_r e
     * *labstart_r podem conter conteúdo inválido. Não chame findlabel_next novamente.
     */
    adns_status adns__findlabel_next(findlabel_state *fls, int *lablen_r, int *labstart_r);

    /**
     *
     */
    typedef enum
    {
        /**
         *
         */
        pdf_quoteok = 0x001
    } parsedomain_flags;

    /**
     * vb já deve ter sido inicializado; ele será redefinido, se necessário.
     * Se houver truncamento, vb->used será definido como 0; caso contrário
     * (se não houver falha), vb terá terminação nula. Se houver uma falha,
     * vb e *cbyte_io podem ser deixados indeterminados.
     *
     * serv pode ser -1 e qu pode ser 0 - eles são usados apenas para
     * relatar falhas.
     */
    adns_status adns__parse_domain(adns_state ads, int serv, adns_query qu, vbuf *vb, parsedomain_flags flags, const byte *dgram, int dglen, int *cbyte_io, int max);

    /**
     * Como adns__parse_domain, mas você passa a ele um findlabel_state pré-inicializado,
     * para continuar um domínio existente ou algum tipo semelhante. Além disso, ao
     * contrário de _parse_domain, os dados do domínio serão anexados a vb, em vez
     * de substituir o conteúdo existente.
     */
    adns_status adns__parse_domain_more(findlabel_state *fls, adns_state ads, adns_query qu, vbuf *vb, parsedomain_flags flags, const byte *dgram);

    /**
     * Localiza a extensão e parte do conteúdo de um RR em um datagrama e
     * faz algumas verificações. O datagrama é *dgram, comprimento dglen,
     * e o RR começa em *cbyte_io (que é atualizado posteriormente para
     * apontar para o final do RR).
     *
     * O tipo, classe, comprimento TTL e RRdata e início são retornados se
     * as variáveis de ponteiro correspondentes não forem nulas. type_r,
     * class_r e ttl_r não podem ser nulos. O TTL será limitado.
     *
     * Se ownermatchedquery_r != 0, o domínio do proprietário deste RR
     * será comparado com o da consulta (ou, se a consulta tiver ido
     * para uma pesquisa CNAME, com o nome canônico). Neste caso,
     * *ownermatchedquery_r será definido como 0 ou 1. O datagrama
     * de consulta (ou datagrama CNAME) DEVE ser válido e não
     * truncado.
     *
     * Se houver truncamento, *type_r será definido como -1 e *cbyte_io, *class_r,
     * *rdlen_r, *rdstart_r e *eo_matched_r serão indefinidos.
     *
     * qu obviamente deve ser não nulo.
     *
     * Se um erro for retornado, *type_r também será indefinido.
     */
    adns_status adns__findrr(adns_query qu, int serv, const byte *dgram, int dglen, int *cbyte_io, int *type_r, int *class_r, unsigned long *ttl_r, int *rdlen_r, int *rdstart_r, int *ownermatchedquery_r);

    /**
     * Como adns__findrr_checked, exceto que o datagrama e o proprietário
     * para comparação podem ser especificados explicitamente.
     *
     * Se o chamador achar que sabe qual deve ser o proprietário do RR, ele
     * pode passar os detalhes em eo_*: este é outro (ou talvez o mesmo
     * datagrama) e um ponteiro para onde o suposto proprietário começa
     * naquele datagrama. Neste caso, *eo_matched_r será definido como 1
     * se o datagrama for compatível ou 0 se não for. Ambos eo_dgram e
     * eo_matched_r devem ser não nulos, ou ambos devem ser nulos (nesse
     * caso, eo_dglen e eo_cbyte serão ignorados). O eo datagrama e o
     * domínio do proprietário contido DEVEM ser válidos e não truncados.
     */
    adns_status adns__findrr_anychk(adns_query qu, int serv, const byte *dgram, int dglen, int *cbyte_io, int *type_r, int *class_r, unsigned long *ttl_r, int *rdlen_r, int *rdstart_r, const byte *eo_dgram, int eo_dglen, int eo_cbyte, int *eo_matched_r);

    /**
     * Atualiza o campo `expires' na consulta, para que
     * não exceda now + ttl.
     */
    void adns__update_expires(adns_query qu, unsigned long ttl, struct timeval now);

    /**
     *
     */
    bool adns__labels_equal(const byte *a, int al, const byte *b, int bl);

    /**
     * Do event.c:
     */

    /**
     * O que e por que podem ser ambos 0 ou ambos não-0.
     */
    void adns__tcp_broken(adns_state ads, const char *what, const char *why);

    /**
     *
     */
    void adns__tcp_tryconnect(adns_state ads, struct timeval now);

    /**
     * Faça todas as chamadas de sistema que quisermos, se o programa
     * assim o desejar. Não deve ser chamado de dentro das funções de
     * processamento interno do adns, para que não acabemos em descida
     * recursiva!
     */
    void adns__autosys(adns_state ads, struct timeval now);

    /**
     *
     */
    static inline void adns__timeout_set(adns_query qu, struct timeval now, long ms)
    {
        qu->timeout_ms = ms;
        qu->timeout_started = now;
    }

    /**
     *
     */
    static inline void adns__timeout_clear(adns_query qu)
    {
        qu->timeout_ms = 0;

        timerclear(&qu->timeout_started);
    }

    /**
     *
     */
    int adns__gettimeofday(adns_state ads, struct timeval *tv_buf);

    /**
     * Ligue com cuidado - pode fazer com que as consultas sejam
     * concluídas de forma reentrante!
     */
    void adns__must_gettimeofday(adns_state ads, const struct timeval **now_io, struct timeval *tv_buf);

    /**
     *
     */
    int adns__pollfds(adns_state ads, struct pollfd pollfds_buf[MAX_POLLFDS]);

    /**
     *
     */
    void adns__fdevents(adns_state ads, const struct pollfd *pollfds, int npollfds, int maxfd, const fd_set *readfds, const fd_set *writefds, const fd_set *exceptfds, struct timeval now, int *r_r);

    /**
     *
     */
    int adns__internal_check(adns_state ads, adns_query *query_io, adns_answer **answer, void **context_r);

    /**
     * Se o act for !0, isso também lidará com a conexão TCP.
     * Se eventos anteriores o interromperam ou exigiram que
     * ele fosse conectado.
     */
    void adns__timeouts(adns_state ads, int act, struct timeval **tv_io, struct timeval *tvbuf, struct timeval now);

    /**
     * Do check.c:
     */

    /**
     *
     */
    void adns__consistency(adns_state ads, adns_query qu, consistency_checks cc);

    /**
     * Funções inline estáticas úteis:
     */

    /**
     *
     */
    static inline int ctype_whitespace(int c)
    {
        return c == ' ' || c == '\n' || c == '\t';
    }

    /**
     *
     */
    static inline int ctype_digit(int c)
    {
        return c >= '0' && c <= '9';
    }

    /**
     *
     */
    static inline int ctype_alpha(int c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    /**
     *
     */
    static inline int ctype_toupper(int c)
    {
        return ctype_alpha(c) ? (c & ~32) : c;
    }

    /**
     *
     */
    static inline int ctype_822special(int c)
    {
        return strchr("()<>@,;:\\\".[]",c) != 0;
    }

    /**
     *
     */
    static inline int ctype_domainunquoted(int c)
    {
        return ctype_alpha(c) || ctype_digit(c) || (strchr("-_/+",c) != 0);
    }

    /**
     *
     */
    static inline int errno_resources(int e)
    {
        return e == ENOMEM || e == ENOBUFS;
    }

    /**
     * Macros úteis.
     */

    /**
     *
     */
    #define MEM_ROUND(sz) ((((sz) + sizeof(union maxalign) - 1) / sizeof(union maxalign)) * sizeof(union maxalign))

    /**
     *
     */
    #define GETIL_B(cb) (((dgram)[(cb)++]) & 0x0ffu)

    /**
     *
     */
    #define GET_B(cb, tv) ((tv) = GETIL_B((cb)))

    /**
     *
     */
    #define GET_W(cb, tv) ((tv) = 0,(tv)|=(GETIL_B((cb))<<8), (tv)|=GETIL_B(cb), (tv))

    /**
     *
     */
    #define GET_L(cb,tv) ((tv) = 0, \
        (tv)|=(GETIL_B((cb)) << 24), \
        (tv)|=(GETIL_B((cb)) << 16), \
        (tv)|=(GETIL_B((cb)) <<  8), \
        (tv)|=GETIL_B(cb), \
        (tv))

#endif
