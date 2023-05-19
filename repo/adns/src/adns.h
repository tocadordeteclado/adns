/**
 * adns.h - API adns visível ao usuário.
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

#ifndef ADNS_H_INCLUDED
#define ADNS_H_INCLUDED

    #include <stdio.h>
    #include <stdarg.h>

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <sys/time.h>
    #include <unistd.h>
    #include <net/if.h>


    #ifdef __cplusplus
        /**
         * Eu realmente não posso opinar sobre isso.
         */
        extern "C" {
    #endif

    /**
     * Se deve suportar famílias de endereços diferentes de IPv4 em respostas que
     * usam a estrutura `adns_rr_addr'. Este é um problema de compatibilidade de
     * origem: clientes antigos podem não esperar encontrar famílias de endereços
     * diferentes de AF_INET em seus resultados de consulta. Há um problema
     * separado de compatibilidade binária relacionado ao tamanho da estrutura
     * `adns_rr_addr', mas assumiremos que você pode lidar com isso porque possui
     * esse arquivo de cabeçalho. Defina `ADNS_FEATURE_IPV4ONLY' se desejar apenas
     * ver endereços AF_INET por padrão ou `ADNS_FEATURE_MANYAF' para permitir
     * várias famílias de endereços; o padrão atualmente é ficar apenas com AF_INET,
     * mas isso provavelmente mudará em uma versão posterior do ADNS. Observe que
     * todos os sinalizadores adns_qf_want_... em sua consulta são observados: essa
     * configuração afeta apenas as famílias de endereços padrão.
     */
    #if !defined(ADNS_FEATURE_IPV4ONLY) && !defined(ADNS_FEATURE_MANYAF)
        /**
         *
         */
        #define ADNS_FEATURE_IPV4ONLY
    #elif defined(ADNS_FEATURE_IPV4ONLY) && defined(ADNS_FEATURE_MANYAF)
        /**
         *
         */
        #error "Feature flags ADNS_FEATURE_IPV4ONLY and ..._MANYAF are incompatible"
    #endif

    /**
     * Todos os struct in_addr em qualquer lugar em adns estão na
     * ordem de byte NETWORK.
     */

    /**
     *
     */
    typedef struct adns__state *adns_state;

    /**
     *
     */
    typedef struct adns__query *adns_query;

    /**
     * Em geral, ou em conjunto as bandeiras desejadas:
     */
    typedef enum
    {
        /**
         * Sem bandeiras. melhor do que 0 para alguns compiladores.
         */
        adns_if_none = 0x0000,

        /**
         * Não olhe para o ambiente.
         */
        adns_if_noenv = 0x0001,

        /**
         * Nunca imprima para stderr (substituições _debug).
         */
        adns_if_noerrprint = 0x0002,

        /**
         * Não avise ao stderr sobre servidores duff etc.
         */
        adns_if_noserverwarn = 0x0004,

        /**
         * Habilite todas as saídas para stderr mais mensagens de depuração.
         */
        adns_if_debug = 0x0008,

        /**
         * Incluir pid na saída de diagnóstico.
         */
        adns_if_logpid = 0x0080,

        /**
         * Não faça chamadas de sistema em todas as oportunidades.
         */
        adns_if_noautosys = 0x0010,

        /**
         * Permitir _wait e _synchronous para retornar EINTR.
         */
        adns_if_eintr = 0x0020,

        /**
         * Applic tem SIGPIPE ignorado, não consegue ser atacado.
         */
        adns_if_nosigpipe = 0x0040,

        /**
         * Habilite se puder; consulte adns_processtimeouts.
         */
        adns_if_monotonic = 0x0080,

        /**
         * Verificações de consistência na entrada/saída para adns fns.
         */
        adns_if_checkc_entex = 0x0100,

        /**
         * Verificações de consistência com muita frequência (baixas verificações!).
         */
        adns_if_checkc_freq = 0x0300,

        /**
         * Permitir que consultas _addr retornem endereços IPv4.
         */
        adns_if_permit_ipv4 = 0x0400,

        /**
         * Permitir que consultas _addr retornem endereços IPv6.
         */
        adns_if_permit_ipv6 = 0x0800,

        /**
         *
         */
        adns_if_afmask = 0x0c00,

        /**
         * Esses são sinalizadores de política e substituídos pela opção adns_af:... em resolv.conf.
         * Se os sinalizadores de consulta adns_qf_want_... forem incompatíveis com essas
         * configurações (no sentido de que nenhuma família de endereço é permitida), os
         * sinalizadores de consulta terão precedência; caso contrário, apenas registros
         * que satisfaçam todos os requisitos estabelecidos são permitidos.
         */
        adns__if_sizeforce = 0x7fff,
    } adns_initflags;

    /**
     * Em geral, ou em conjunto as bandeiras desejadas:
     */
    typedef enum
    {
        /**
         * Sem selos.
         */
        adns_qf_none = 0x00000000,

        /**
         * Use a lista de busca (searchlist).
         */
        adns_qf_search = 0x00000001,

        /**
         * Usar um componente virtual (TCP conn).
         */
        adns_qf_usevc = 0x00000002,

        /**
         * Preencha o campo proprietário na resposta.
         */
        adns_qf_owner = 0x00000004,

        /**
         * Permitir caracteres especiais no domínio de consulta.
         */
        adns_qf_quoteok_query = 0x00000010,

        /**
         * ... em CNAME vamos via (agora padrão).
         */
        adns_qf_quoteok_cname = 0x00000000,

        /**
         * ... em coisas supostamente hostnames.
         */
        adns_qf_quoteok_anshost = 0x00000040,

        /**
         * Recusar se os caracteres de solicitação de cotação no CNAME passarem.
         */
        adns_qf_quotefail_cname = 0x00000080,

        /**
         * Permitir refs para CNAMEs - sem, obter _s_cname.
         */
        adns_qf_cname_loose = 0x00000100,

        /**
         * Proibir referências CNAME (padrão, atualmente).
         */
        adns_qf_cname_strict = 0x00010000,

        /**
         * Não siga CNAMEs, em vez disso dê _s_cname.
         */
        adns_qf_cname_forbid = 0x00000200,

        /**
         * Tente retornar endereços IPv4.
         */
        adns_qf_want_ipv4 = 0x00000400,

        /**
         * Tente retornar endereços IPv6.
         */
        adns_qf_want_ipv6 = 0x00000800,

        /**
         * Todos os bits sinalizadores acima.
         */
        adns_qf_want_allaf = 0x00000c00,

        /**
         * Sem nenhum dos sinalizadores _qf_want_..., as consultas _qtf_deref tentam
         * retornar todas as famílias de endereços permitidas por _if_permit_...
         * (conforme substituído pela opção de configuração `adns_af:...'). Defina
         * sinalizadores para restringir as famílias de endereços retornadas àquelas
         * selecionadas.
         */

        /**
         * ... devolve um endereço IPv4  como v6-mapeado.
         */
        adns_qf_ipv6_mapv4 = 0x00001000,

        /**
         * Proibir %<scope> em IPv6 literais.
         */
        adns_qf_addrlit_scope_forbid = 0x00002000,

        /**
         * %<scope> só pode ser numérico.
         */
        adns_qf_addrlit_scope_numeric = 0x00004000,

        /**
         * Rejeitar ipv4 não pontilhado.
         */
        adns_qf_addrlit_ipv4_quadonly = 0x00008000,

        adns__qf_internalmask = 0x0ff00000,
        adns__qf_sizeforce = 0x7fffffff
    } adns_queryflags;

    /**
     *
     */
    typedef enum
    {
        adns_rrt_typemask = 0x0ffff,
        adns_rrt_reprmask = 0xffffff,

        /**
         * Versão interna de ..._deref abaixo.
         */
        adns__qtf_deref_bit = 0x10000,

        /**
         * Caixas de correio de retorno no campo RFC822 rcpt fmt.
         */
        adns__qtf_mail822 = 0x20000,

        /**
         * Use a nova união sockaddr maior.
         */
        adns__qtf_bigaddr = 0x1000000,

        /**
         * Permissão para retornar várias famílias de endereços.
         */
        adns__qtf_manyaf = 0x2000000,

        /**
         * Domínios de desreferência; talvez obter dados extras.
         */
        adns__qtf_deref = adns__qtf_deref_bit | adns__qtf_bigaddr
            #ifdef ADNS_FEATURE_MANYAF
                | adns__qtf_manyaf
            #endif
            ,

        /**
         * Para usar isso, solicite registros do tipo <rr-type-code>|adns_r_unknown. adns
         * não processará o RDATA - você obterá adns_rr_byteblocks, onde o int é o comprimento
         * e o unsigned char* aponta para os dados. A representação de string dos dados RR (por
         * adns_rrinfo) é como em RFC3597. adns_rr_info não retornará o nome do tipo em *rrtname_r
         * (devido a problemas de gerenciamento de dados); *fmtname_r será definido como
         * "unknown" ("desconhecido").
         *
         * Não especifique adns_r_unknown junto com um tipo de RR conhecido que requer descompactação
         * de nome de domínio (consulte RFC3597 s4); nomes de domínio não serão descompactados e os
         * dados resultantes seriam inúteis. Pedir tipos de meta-RR via adns_r_unknown também não
         * funcionará corretamente e pode fazer com que os adns reclamem sobre o mau comportamento
         * do funcionário, então não faça isso.
         *
         * Não se esqueça de adns_qf_quoteok.
         */
        adns_r_unknown = 0x40000,

        /**
         *
         */
        adns_r_none = 0,

        /**
         *
         */
        adns_r_a = 1,

        /**
         *
         */
        adns_r_ns_raw = 2,

        /**
         *
         */
        adns_r_ns = adns_r_ns_raw | adns__qtf_deref,

        /**
         *
         */
        adns_r_cname = 5,

        /**
         *
         */
        adns_r_soa_raw = 6,

        /**
         *
         */
        adns_r_soa = adns_r_soa_raw | adns__qtf_mail822,

        /**
         * Não se importe com PTR com endereço inválidos ou faltando.
         */
        adns_r_ptr_raw = 12,

        /**
         *
         */
        adns_r_ptr = adns_r_ptr_raw | adns__qtf_deref,

        /**
         *
         */
        adns_r_hinfo = 13,

        /**
         *
         */
        adns_r_mx_raw = 15,

        /**
         *
         */
        adns_r_mx = adns_r_mx_raw | adns__qtf_deref,

        /**
         *
         */
        adns_r_txt = 16,

        /**
         *
         */
        adns_r_rp_raw = 17,

        /**
         *
         */
        adns_r_rp = adns_r_rp_raw | adns__qtf_mail822,

        /**
         *
         */
        adns_r_aaaa = 28,

        /**
         * Para registros SRV, o domínio de consulta sem _qf_quoteok_query deve ter a
         * aparência esperada de SRV RFC com nome semelhante ao nome do host.
         * _With_ _quoteok_query, qualquer domínio de consulta é permitido.
         */
        adns_r_srv_raw = 33,

        /**
         *
         */
        adns_r_srv = adns_r_srv_raw | adns__qtf_deref,

        /**
         *
         */
        adns_r_addr = adns_r_a | adns__qtf_deref,

        /**
         *
         */
        adns__rrt_sizeforce = 0x7fffffff,
    } adns_rrtype;

    /**
     * Em consultas sem qf_quoteok_*, todos os domínios devem ter sintaxe legal padrão,
     * ou o cavalo obtém adns_s_querydomainvalid (se o domínio da consulta contiver
     * caracteres incorretos) ou adns_s_answerdomaininvalid (se a resposta contiver
     * caracteres incorretos).
     *
     * Nas consultas _with_ qf_quoteok_*, os domínios na consulta ou resposta podem
     * conter quaisquer caracteres, citados conforme RFC1035 5.1. Na entrada para adns,
     * o char* é um ponteiro para o interior de uma " string delimitada, exceto que "
     * pode aparecer nela sem aspas. Na saída, o char* é um ponteiro para uma string
     * que seria válida dentro ou fora dos " delimitadores; qualquer caractere que
     * não seja permitido em um nome de máquina (ou seja, alfanumérico ou hífen) ou
     * um de _ / + (as outras três pontuações caracteres comumente usadas com
     * frequência em nomes de domínio) serão citados, como \X se for um caractere
     * ASCII impresso ou \DDD caso contrário.
     *
     * Se a consulta for por meio de um CNAME, o nome canônico (ou seja, a coisa a
     * que o registro CNAME se refere) geralmente pode conter quaisquer caracteres,
     * que serão citados como acima. Com adns_qf_quotefail_cname, você obtém
     * adns_s_answerdomaininvalid quando isso acontece. (Essa é uma alteração
     * da versão 0.4 e anterior, na qual a falha na consulta era o padrão e
     * você tinha que dizer adns_qf_quoteok_cname para evitar isso; esse
     * sinalizador agora não é mais válido.).
     *
     * Na versão 0.4 e anteriores, obter registros _raw contendo caixas de entrada
     * sem especificar _qf_quoteok_anshost não era legal. Este não é mais o caso.
     * Nesta versão, apenas partes das respostas que deveriam ser nomes de host
     * serão recusadas por padrão se caracteres que exigem aspas forem
     * encontrados.
     */

    /**
     * Se você solicitar um RR que contenha domínios que são realmente caixas de entrada
     * codificadas e não solicitar a versão _raw, adns retornará a caixa de entrada
     * formatada adequadamente para um campo de cabeçalho de destinatário RFC822.
     * O formato específico usado é que, se a caixa de entrada exigir aspas de
     * acordo com as regras do RFC822, a parte local será citada entre aspas
     * duplas, que terminará na próxima aspa dupla sem escape (\ é o caractere de escape
     * e é duplicado e é usado para escapar apenas \ e "). Se a parte local for legal sem
     * citar de acordo com RFC822, ela será apresentada como está. Em qualquer caso, a
     * parte local é seguida por um @ e o domínio. O domínio não conterá nenhum caracteres
     * ilegais em nomes de host.
     *
     * As partes locais sem aspas podem conter qualquer impressão ASCII de 7 bits, exceto
     * os caracteres de pontuação ( ) < > @ , ; : \ " [ ]. Ou seja, podem conter caracteres
     * alfanuméricos e os seguintes caracteres de pontuação: ! # % ^ & * - _ = + { } .
     *
     * adns rejeitará partes locais contendo caracteres de controle (valores de byte:
     * 0-31, 127-159 e 255) - estes parecem ser legais de acordo com RFC822 (pelo
     * menos 0-127), mas são claramente uma má ideia. A sintaxe RFC1035 não faz
     * nenhuma distinção entre uma única string entre aspas RFC822 contendo pontos
     * finais e uma série de strings entre aspas separadas por pontos finais; adns
     * retornará qualquer coisa que não seja todos os átomos válidos como uma única
     * string entre aspas. O RFC822 não permite caracteres de conjunto de bits altos,
     * mas o adns os permite em partes locais, tratando-os como se precisassem de aspas.
     *
     * Se você solicitar o domínio com _raw, _no_ (nenhuma) verificação será feita
     * (mesmo na parte do host, independentemente de adns_qf_quoteok_anshost) e você
     * apenas obterá o nome do domínio no formato de arquivo mestre.
     *
     * Se nenhuma caixa de entrada for fornecida, a string retornada
     * será `.' em ambos os casos.
     */

    /**
     *
     */
    typedef enum
    {
        /**
         *
         */
        adns_s_ok,

        /**
         * Erros induzidos localmente.
         */
        adns_s_nomemory,
        adns_s_unknownrrtype,
        adns_s_systemfail,

        /**
         *
         */
        adns_s_max_localfail = 29,

        /**
         * Erros induzidos remotamente, detectados localmente.
         */
        adns_s_timeout,
        adns_s_allservfail,
        adns_s_norecurse,
        adns_s_invalidresponse,
        adns_s_unknownformat,

        /**
         *
         */
        adns_s_max_remotefail = 59,

        /**
         * Falhas induzidos remotamente, que já forão relatados pelo servidor
         * remoto para nós.
         *     Resposta: Já está em processo de correção.
         */
        adns_s_rcodeservfail,
        adns_s_rcodeformaterror,
        adns_s_rcodenotimplemented,
        adns_s_rcoderefused,
        adns_s_rcodeunknown,

        /**
         *
         */
        adns_s_max_tempfail = 99,

        /**
         * Falhas de configuração remota.
         */

        /**
         * PTR fornece domínio cujo caminho de rede está ausente ou incompatível.
         */
        adns_s_inconsistent,

        /**
         * CNAME, mas por exemplo A esperado (não se _qf_cname_loose).
         */
        adns_s_prohibitedcname,

        /**
         *
         */
        adns_s_answerdomaininvalid,
        adns_s_answerdomaintoolong,
        adns_s_invaliddata,

        /**
         *
         */
        adns_s_max_misconfig = 199,

        /**
         * Problemas permanentes com a consulta.
         */
        adns_s_querydomainwrong,
        adns_s_querydomaininvalid,
        adns_s_querydomaintoolong,

        /**
         *
         */
        adns_s_max_misquery = 299,

        /**
         * Erros permanentes.
         */
        adns_s_nxdomain,
        adns_s_nodata,

        /**
         *
         */
        adns_s_max_permfail = 499
    } adns_status;

    /**
     *
     */
    typedef union
    {
        /**
         *
         */
        struct sockaddr sa;

        /**
         *
         */
        struct sockaddr_in inet;
    } adns_sockaddr_v4only;

    /**
     *
     */
    typedef union
    {
        /**
         *
         */
        struct sockaddr sa;

        /**
         *
         */
        struct sockaddr_in inet;

        /**
         *
         */
        struct sockaddr_in6 inet6;
    } adns_sockaddr;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        int len;

        /**
         *
         */
        adns_sockaddr addr;
    } adns_rr_addr;

    /**
     *
     */
    typedef struct
    {
        /**
         * A antiga estrutura somente v4; útil se você tiver problemas complicados
         * de compatibilidade binária.
         */
        int len;

        /**
         *
         */
        adns_sockaddr_v4only addr;
    } adns_rr_addr_v4only;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        char *host;

        /**
         *
         */
        adns_status astatus;

        /**
         * temp fail => -1,
         * perm fail =>  0,
         * s_ok      => >0
         */
        int naddrs;

        /**
         *
         */
        adns_rr_addr *addrs;
    } adns_rr_hostaddr;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        char *(array[2]);
    } adns_rr_strpair;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        int i;

        /**
         *
         */
        adns_rr_hostaddr ha;
    } adns_rr_inthostaddr;

    /**
     *
     */
    typedef struct
    {
        /**
         * Usado tanto para mx_raw, caso em que i é a preferência e str o domínio,
         * quanto para txt, em que cada entrada tem i para o comprimento do `texto'
         * e str para os dados (que terão um nul extra anexado para que se fosse
         * texto simples, agora é uma string terminada em nulo).
         */
        int i;

        /**
         *
         */
        char *str;
    } adns_rr_intstr;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        adns_rr_intstr array[2];
    } adns_rr_intstrpair;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        char *mname, *rname;

        /**
         *
         */
        unsigned long serial, refresh, retry, expire, minimum;
    } adns_rr_soa;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
       int priority, weight, port;

        /**
         *
         */
       char *host;
    } adns_rr_srvraw;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        int priority, weight, port;

        /**
         *
         */
        adns_rr_hostaddr ha;
    } adns_rr_srvha;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        int len;

        /**
         *
         */
        unsigned char *data;
    } adns_rr_byteblock;

    /**
     *
     */
    typedef struct
    {
        /**
         *
         */
        adns_status status;

        /**
         * sempre NULL se a consulta for para registros CNAME.
         */
        char *cname;

        /**
         * Definido apenas se exigido em sinalizadores de consulta; talvez 0
         * em falhas desconhecidas.
         */
        char *owner;

        /**
         * Normalmente com funcionamento igual para ser o mesmo que na consulta.
         */
        adns_rrtype type;

        /**
         * hora do plim-plim. def somente se _s_ok, nxdomain ou nodata.
         * Sem TTL.
         */
        time_t expires;

        /**
         * nrrs é 0 se ocorrer uma falha.
         */
        int nrrs, rrsz;

        /**
         *
         */
        union
        {
            /**
             *
             */
            void *untyped;

            /**
             *
             */
            unsigned char *bytes;

            /**
             * ns_raw, cname, ptr, ptr_raw.
             */
            char *(*str);

            /**
             * txt (list strs ends with i=-1, str=0).
             */
            adns_rr_intstr *(*manyistr);

            /**
             * addr.
             */
            adns_rr_addr *addr;

            /**
             * a.
             */
            struct in_addr *inaddr;

            /**
             * aaaa.
             */
            struct in6_addr *in6addr;

            /**
             * ns.
             */
            adns_rr_hostaddr *hostaddr;

            /**
             * hinfo.
             */
            adns_rr_intstrpair *intstrpair;

            /**
             * rp, rp_raw.
             */
            adns_rr_strpair *strpair;

            /**
             * mx.
             */
            adns_rr_inthostaddr *inthostaddr;

            /**
             * mx_raw.
             */
            adns_rr_intstr *intstr;

            /**
             * soa, soa_raw.
             */
            adns_rr_soa *soa;

            /**
             * srv_raw.
             */
            adns_rr_srvraw *srvraw;

            /**
             * srv.
             */
            adns_rr_srvha *srvha;

            /**
             * ...|unknown.
             */
            adns_rr_byteblock *byteblock;
        } rrs;
    } adns_answer;

    /**
     * Gerenciamento de Dados:
     *     adns_state e adns_query são, na verdade, ponteiros para o estado malloc'd;
     *     No envio, as perguntas são copiadas, incluindo o domínio do proprietário;
     *     As respostas são armazenadas como um único pedaço de dados; ponteiros na
     *     estrutura de resposta apontam para mais espaço de dados na resposta.
     *
     * query_io:
     *     Deve ser sempre um ponteiro não nulo;
     *     Se *query_io for 0 para começar, qualquer consulta poderá ser retornada;
     *     Se *query_io for !0 adns_query, somente essa consulta poderá ser retornada.
     *     Se a chamada for bem-sucedida, *query_io, *answer_r e *context_r serão todos definidos.
     *
     * Falhas:
     *     Os valores de retorno são 0 ou um valor errno.
     *
     *     Para _init, _init_strcfg, _submit e _synchronous, falhas de sistema (por exemplo,
     *     falha ao criar plug's, falha de malloc, etc.) retornam valores de errno. EINVAL de
     *     _init et al significa que o arquivo de configuração está corrompido e não pode
     *     ser analisado.
     *
     *     Para _wait e _check, as falhas são relatadas na estrutura de resposta e apenas 0,
     *     ESRCH ou (para _check) EAGAIN é retornado: se nenhuma solicitação (apropriada) for
     *     feita, adns_check retorna EAGAIN; se nenhuma solicitação (apropriada) estiver
     *     pendente, adns_query e adns_wait retornam ESRCH.
     *
     *     Além disso, _wait pode retornar EINTR se você definir adns_if_eintr.
     *
     *     Todas as outras falhas (falha no servidor de nomes, conexões expiradas, &c, etc.)
     *     são retornados no campo de status da resposta. Após um _wait ou _check bem-sucedido,
     *     se o status for diferente de zero, nrrs será 0, caso contrário, será >0. tipo sempre
     *     será o tipo solicitado.
     */

    /**
     * Concorrencia:
     *     adns não usa nenhum estado estático modificável, então é seguro chamar
     *     adns_init várias vezes e então usar os adns_states resultantes
     *     simultaneamente.
     *
     *     No entanto, NÃO é seguro fazer chamadas simultâneas em adns usando o
     *     mesmo adns_state; um único adns_state deve ser usado apenas por um
     *     encadeamento por vez. Você pode resolver esse problema tendo um
     *     adns_state por ordem de chegada ou, se isso não for viável, pode
     *     manter uma rede de adns_states. Infelizmente, nenhuma dessas
     *     abordagens tem um bom desempenho.
     */

    /**
     * Uso do tempo:
     *
     * adns precisa manipular tempos limite. Por motivos de compatibilidade de API
     * (adns é anterior a clock_gettime), o padrão é usar o horário do relógio de
     * parede de gettimeofday. Isso causará mau funcionamento se o relógio do
     * sistema não estiver adequadamente estável. Para evitar isso, você deve
     * definir adns_if_monotonic.
     *
     * Se você especificar adns_if_monotonic, todos os valores `now' passados
     * para adns devem ser de clock_gettime(CLOCK_MONOTONIC). clock_gettime
     * retorna um struct timespec; você deve convertê-lo em um struct
     * timeval dividindo o nsec por 1000 para fazer usec, arredondando
     * para baixo.
     */

    /**
     * FILE *diagfile ~> 0=>stderr.
     */
    int adns_init(adns_state *newstate_r, adns_initflags flags, FILE *diagfile);

    /**
     * FILE *diagfile ~> 0=>discard.
     */
    int adns_init_strcfg(adns_state *newstate_r, adns_initflags flags, FILE *diagfile, const char *configtext);

    /**
     *
     */
    typedef void adns_logcallbackfn(adns_state ads, void *logfndata, const char *fmt, va_list al);

    /**
     * Será chamado talvez várias vezes para cada mensagem; quando a mensagem estiver
     * completa, a string implícita em fmt e al terminará em uma nova linha. As
     * mensagens de log começam com `adns debug:' ou `adns warning:'
     * ou `adns:' (para erros), ou `adns debug [PID]:' etc. se
     * adns_if_logpid estiver definido.
     */

    /**
     * const char *configtext    ~> 0=>use arquivos de configuração padrão.
     * adns_logcallbackfn *logfn ~> 0=>logfndata é um FILE*.
     * void *logfndata           ~> 0 com logfn==0 => descartado.
     */
    int adns_init_logfn(adns_state *newstate_r, adns_initflags flags, const char *configtext, adns_logcallbackfn *logfn, void *logfndata);

    /**
     * Configuração:
     *     adns_init lê /etc/resolv.conf, que deve estar (em termos gerais) no formato
     *     esperado por libresolv e, em seguida, /etc/resolv-adns.conf, se existir. Em
     *     vez disso, adns_init_strcfg recebe uma string que é interpretada como se
     *     fosse o conteúdo de resolv.conf ou resolv-adns.conf. Em geral, a configuração
     *     definida posteriormente substitui qualquer configuração definida anteriormente.
     *
     * Diretivas padrão compreendidas em resolv[-adns].conf:
     *     nameserver <address>
     *         Deve ser seguido pelo endereço IP de um servidor de nomes. Vários servidores
     *         de nomes podem ser especificados e serão testados na ordem encontrada. Existe
     *         um limite compilado, atualmente 5, no número de servidores de nomes. (libresolv
     *         suporta apenas 3 servidores de nomes.).
     *
     *     search <domain> ...
     *         Especifica a lista de pesquisa para consultas que especificam adns_qf_search.
     *         Esta é uma lista de domínios a serem anexados ao domínio de consulta.
     *         O domínio de consulta será tentado como está antes de todos eles ou depois deles,
     *         dependendo da configuração da opção ndots (veja abaixo).
     *
     *     domain <domain>
     *         Isso está presente apenas para compatibilidade com versões obsoletas do libresolv.
     *         Ele não deve ser usado e é interpretado por adns como se fosse 'search' - observe
     *         que isso é sutilmente diferente da interpretação de libresolv desta diretiva.
     *
     *     sortlist <addr>/<mask> ...
     *         Deve ser seguido por uma sequência de pares de endereço IP e máscara de rede,
     *         separados por espaços. Eles podem ser especificados como, por exemplo.
     *         172.30.206.0/24 ou 172.30.206.0/255.255.255.0. Atualmente, até 15 pares
     *         podem ser especificados (mas observe que libresolv suporta apenas até 10).
     *
     *     opções
     *         Deve ser seguido de uma ou mais opções, separadas por espaços. Cada opção
     *         consiste em um nome de opção, seguido opcionalmente por dois pontos e um
     *         valor. As opções estão listadas abaixo.
     *
     * Diretivas não padrão compreendidas em resolv[-adns].conf:
     *     clearnameservers
     *         Limpa a lista de servidores de nomes, para que outras linhas de servidores
     *         de nomes comecem novamente desde o início.
     *
     *         include <filename>
     *         O arquivo especificado será lido.
     *
     * Além disso, o adns irá ignorar as linhas em resolv[-adns].conf
     * que começam com #.
     *
     * Opções padrão compreendidas:
     *     depuração
     *         Ativa a saída de depuração do resolvedor, que será gravada em stderr.
     *
     *     ndots:<count>
     *         Afeta se as consultas com adns_qf_search serão tentadas primeiro sem
     *         adicionar domínios da lista de pesquisa ou se o domínio de consulta
     *         simples será tentado por último. As consultas que contêm pelo menos
     *         <count> pontos serão tentadas primeiro. O padrão é 1.
     *
     * Opções não padronizadas compreendidas:
     *     adns_checkc:none
     *     adns_checkc:entex
     *     adns_checkc:freq
     *         Altera a frequência de verificação de consistência; isso substitui a
     *         configuração de adns_if_check_entex, adns_if_check_freq ou nenhum dos
     *         sinalizadores passados para adns_init.
     *
     *     adns_af:{ipv4,ipv6},...  adns_af:any
     *         Determina quais famílias de endereços o ADNS procura (seja como uma
     *         consulta adns_r_addr ou ao desreferenciar uma resposta que produz
     *         nomes de host (por exemplo, adns_r_mx). O argumento é uma lista
     *         separada por vírgulas: somente as famílias de endereços listadas
     *         serão pesquisadas. O padrão é `any'. As pesquisas ocorrem
     *         (logicamente) simultaneamente; use a diretiva `sortlist' para
     *         controlar a ordem relativa dos endereços nas respostas. Esta
     *         opção substitui os sinalizadores init correspondentes (cobertos
     *         por adns_if_afmask).
     *
     *     adns_ignoreunkcfg
     *         Ignore opções desconhecidas e diretivas de configuração, em vez de
     *         registrá-las. Para ser eficaz, apareça na configuração antes das
     *         opções desconhecidas. ADNS_RES_OPTIONS geralmente é cedo o
     *         suficiente.
     *
     * Existem várias variáveis de ambiente que podem modificar o comportamento
     * de adns. Eles entram em vigor apenas se adns_init for usado e o chamador
     * de adns_init pode desativá-los usando adns_if_noenv. Em cada caso, há um
     * EXEMPLO e um ADNS_EXEMPLO; o último é interpretado posteriormente para
     * que possa substituir o primeiro. Salvo indicação em contrário, as
     * variáveis de ambiente são interpretadas após a leitura de resolv[-adns].conf,
     * na ordem em que estão listadas aqui.
     *     RES_CONF, ADNS_RES_CONF
     *         Um nome de arquivo, cujos conteúdos estão no formato resolv.conf.
     *
     *     RES_CONF_TEXT, ADNS_RES_CONF_TEXT
     *         Uma string no formato de resolv.conf.
     *
     *     RES_OPTIONS, ADNS_RES_OPTIONS
     *         Estes são analisados como se aparecessem na linha `options' de um resolv.conf.
     *         Além de serem analisados neste ponto da sequência, eles também são analisados
     *         bem no início, antes que resolv.conf ou quaisquer outras variáveis de ambiente
     *         sejam lidas, para que qualquer opção de depuração possa afetar o processamento
     *         da configuração.
     *
     *     LOCALDOMAIN, ADNS_LOCALDOMAIN
     *         Estes são interpretados como se seus conteúdos aparecessem em uma
     *         linha `search' no resolv.conf.
     */

    /**
     *
     */
    int adns_synchronous(adns_state ads, const char *owner, adns_rrtype type, adns_queryflags flags, adns_answer **answer_r);

    /**
     * NB: se você definir adns_if_noautosys, _submit e _check não farão nenhuma
     * chamada de sistema; você deve usar algumas das funções de processamento
     * de eventos asynch-io para realmente fazer as coisas acontecerem.
     */

    /**
     *
     */
    int adns_submit(adns_state ads, const char *owner, adns_rrtype type, adns_queryflags flags, void *context, adns_query *query_r);

    /**
     * O proprietário deve ser citado em formato de arquivo mestre.
     */

    /**
     *
     */
    int adns_check(adns_state ads, adns_query *query_io, adns_answer **answer_r, void **context_r);

    /**
     *
     */
    int adns_wait(adns_state ads, adns_query *query_io, adns_answer **answer_r, void **context_r);

    /**
     * O mesmo que adns_wait, mas usa poll(2) internamente.
     */
    int adns_wait_poll(adns_state ads, adns_query *query_io, adns_answer **answer_r, void **context_r);

    /**
     *
     */
    void adns_cancel(adns_query query);

    /**
     * O adns_query que você recebe de _submit é válido (ou seja, pode ser
     * legitimamente passada para funções adns) até que seja retornada por
     * adns_check ou adns_wait, ou passada para adns_cancel. Depois disso,
     * não deve ser usado. Você pode confiar que ele não será reutilizado
     * até a primeira chamada adns_submit ou _transact usando o mesmo
     * adns_state depois que ele se tornou inválido, portanto, você pode
     * compará-lo quanto à igualdade com outros manipuladores de consulta
     * até a próxima chamada _query ou _transact.
     *
     * _submit e _synchronous retornam ENOSYS se não entenderem
     * o tipo de consulta.
     */

    /**
     *
     */
    int adns_submit_reverse(adns_state ads, const struct sockaddr *addr, adns_rrtype type, adns_queryflags flags, void *context, adns_query *query_r);

    /**
     * tipo deve ser _r_ptr ou _r_ptr_raw.  _qf_search é ignorado.
     * addr->sa_family devem ser AF_INET ou você obtem ENOSYS.
     */

    /**
     *
     */
    int adns_submit_reverse_any(adns_state ads, const struct sockaddr *addr, const char *rzone, adns_rrtype type, adns_queryflags flags, void *context, adns_query *query_r);

    /**
     * Para RBL-style reverso `zone's; bloqueia para cima.
     *   <reversed-address>.<zone>
     * Qualquer tipo é permitido.  _qf_search é ignorado.
     * addr->sa_family devemos ser AF_INET ou você consegue ENOSYS.
     */

    /**
     *
     */
    void adns_finish(adns_state ads);

    /**
     * Você pode chamar isso mesmo se tiver dúvidas pendentes;
     * eles serão cancelados.
     */

    /**
     * INET6_ADDRSTRLEN + 1 ~> %.
     * + ((IF_NAMESIZE - 1) > 9 ? (IF_NAMESIZE-1) : 9) ~> uint32
     * + 1 ~> nul; incluído em IF_NAMESIZE.
     */
    #define ADNS_ADDR2TEXT_BUFLEN \
        (INET6_ADDRSTRLEN + 1 \
        + ((IF_NAMESIZE - 1) > 9 ? (IF_NAMESIZE-1) : 9) \
        + 1)

    /**
     * socklen_t *salen_io ~> atualizando iff OK ou ENOSPC.
     */
    int adns_text2addr(const char *text, uint16_t port, adns_queryflags flags, struct sockaddr *sa_r, socklen_t *salen_io);

    /**
     * char *buffer, int *buflen_io ~> atualizando ONLY em ENOSPC.
     * int *port_r ~> talvez 0.
     */
    int adns_addr2text(const struct sockaddr *sa, adns_queryflags flags, char *buffer, int *buflen_io, int *port_r);

    /**
     * port está sempre na ordem de byte do host e é simplesmente copiado de e para o
     * campo sockaddr apropriado (bytes trocados conforme necessário).
     *
     * Os únicos sinalizadores suportados são adns_qf_addrlit_...
     *
     * Os valores de retorno de falha são:
     *     ENOSPC O buffer de saída é muito pequeno. Só pode acontecer se
     *            *buflen_io < ADNS_ADDR2TEXT_BUFLEN ou
     *            *salen_io < sizeof(adns_sockaddr).  No retorno,
     *            *buflen_io ou *salen_io foi atualizado por adns.
     *
     *     EINVAL text tem sintaxe inválida.
     *
     *            text representa uma família de endereços não suportada por
     *            esta versão de adns.
     *
     *            Endereço com escopo fornecido (o texto continha "%" ou sin6_scope_id
     *            diferente de zero), mas o chamador especificou adns_qf_addrlit_scope_forbid.
     *
     *            Nome do escopo (em vez de número) fornecido no texto, mas especificado pelo
     *            chamador adns_qf_addrlit_scope_numeric.
     *
     *     EAFNOSUPPORT sa->sa_family não é suportado (addr2text apenas).
     *     ENOSYS       Conjunto de sinalizadores sem suporte.
     *
     * Somente se nem adns_qf_addrlit_scope_forbid nem
     * adns_qf_addrlit_scope_numeric estiverem definidos:
     *
     *     ENOSYS Nome do escopo fornecido em texto, mas parte do endereço IPv6 de
     *            sockaddr não é um endereço local de link.
     *
     *     ENXIO  Nome do escopo fornecido em texto, mas if_nametoindex informa que
     *            não era um nome de interface local válido.
     *
     *     EIO    Endereço com escopo fornecido, mas if_nametoindex falhou de forma
     *            inesperada; adns imprimiu uma mensagem para stderr.
     *
     * qualquer outro if_nametoindex falhou de uma maneira mais ou
     * menos esperada.
     */

    /**
     *
     */
    void adns_forallqueries_begin(adns_state ads);

    /**
     *
     */
    adns_query adns_forallqueries_next(adns_state ads, void **context_r);

    /**
     * Funções iteradoras, que você pode usar para percorrer as consultas pendentes (enviadas,
     * mas ainda não verificadas/aguardadas com êxito).
     *
     * Você só pode ter uma iteração acontecendo de uma vez. Você pode ligar para _begin a
     * qualquer momento; depois disso, uma iteração estará em andamento. Você só pode chamar
     * _next quando uma iteração estiver em andamento - qualquer outra coisa pode ser
     * despejada. A iteração permanece em andamento até que _next retorne 0, indicando que
     * todas as consultas foram percorridas ou QUALQUER outra função adns seja chamada com
     * o mesmo adns_state (ou uma consulta no mesmo adns_state). Não há necessidade de
     * terminar explicitamente uma iteração.
     *
     * context_r pode ser 0. *context_r não pode ser definido quando _next
     * retorna 0.
     */

    /**
     *
     */
    void adns_checkconsistency(adns_state ads, adns_query qu);

    /**
     * Verifica a consistência das estruturas de dados internas do adns.
     * Se alguma falha for encontrada, o programa talvez vá chamar a função abort() (Terminar).
     * Você pode passar 0 para qu; se você passar como não nulo, verificações adicionais
     * serão feitas para garantir que qu seja uma consulta válida.
     */

    /**
     * Exemplo de sequência de chamada esperada/legal para submit/check/wait:
     *  adns_init
     *  adns_submit 1
     *  adns_submit 2
     *  adns_submit 3
     *  adns_wait 1
     *  adns_check 3 -> EAGAIN
     *  adns_wait 2
     *  adns_wait 3
     *  ....
     *  adns_finish
     */

    /**
     * Pontos de entrada para asynch io genérico: (esses pontos de entrada não são muito
     * úteis, exceto em combinação com * algumas das outras chamadas de modelo de E/S
     * que podem informar em quais fds você deve se interessar):
     *
     * Observe que qualquer chamada de adns pode fazer com que adns abra e feche fds,
     * então você deve chamar beforeselect ou beforepoll novamente antes de bloquear,
     * ou você pode não ter uma lista atualizada de seus fds.
     */

    /**
     *
     */
    int adns_processany(adns_state ads);

    /**
     * Dá um pouco de controle de fluxo de adns. Isso nunca bloqueará e pode
     * ser usado com qualquer modelo de threading/asynch-io. Se ocorrer
     * algum erro que possa causar a rotação de um loop de eventos,
     * o valor errno será retornado.
     */

    /**
     *
     */
    int adns_processreadable(adns_state ads, int fd, const struct timeval *now);

    /**
     *
     */
    int adns_processwriteable(adns_state ads, int fd, const struct timeval *now);

    /**
     *
     */
    int adns_processexceptional(adns_state ads, int fd, const struct timeval *now);

    /**
     * Fornece fluxo de controle de adns para que ele possa processar dados
     * de entrada ou enviar dados de saída via fd (Descritor de arquivo).
     * Muito parecido com _processany. Se retornar zero, fd (Descritor de
     * arquivo) não será mais legível ou gravável (a menos, é claro, que
     * mais dados tenham chegado desde então). adns usará _apenas_ esse
     * fd (Descritor de arquivos) e apenas da maneira especificada,
     * independentemente de adns_if_noautosys ter sido especificado.
     *
     * Agora é quanto a adns_processtimeouts.
     *
     * adns_processexceptional deve ser chamado quando select(2) reporta
     * uma condição excepcional, ou poll(2) reporta POLLPRI.
     *
     * Não há problema em chamar _processreabable ou _processwriteable
     * quando o fd (Descritor de arquivo) não estiver pronto ou com um
     * fd (Descritor de arquivo) que não pertença a adns; ele retornará
     * apenas 0.
     *
     * Se ocorrer alguma falha que possa impedir a rotação de um loop de
     * eventos, o valor errno será retornado.
     */

    /**
     *
     */
    void adns_processtimeouts(adns_state ads, const struct timeval *now);

    /**
     * Fornece fluxo de controle de adns para que ele possa processar
     * qualquer tempo limite que possa ter ocorrido. Muito parecido
     * com _processreadable/writeable.
     *
     * Agora pode ser 0; se não for, *now (agora) deve ser a hora atual
     * de gettimeofday, ou iff adns_if_monotonic deve ser convertido dos
     * resultados de clock_gettime(CLOCK_MONOTONIC) (com timespec.tv_nsec
     * arredondado para baixo para fazer timeval.tv_usec).
     */

    /**
     *
     */
    void adns_firsttimeout(adns_state ads, struct timeval **tv_mod, struct timeval *tv_buf, struct timeval now);

    /**
     * Pergunta aos adns quando gostaria de ter a oportunidade de
     * cronometrar algo.
     *
     * agora deve ser a hora atual, como para adns_processtimeouts.
     *
     * Se tv_mod apontar para 0, então tv_buf deve ser não nulo e _firsttimeout
     * preencherá *tv_buf com o tempo até o primeiro tempo limite e fará com que
     * *tv_mod aponte para tv_buf. Se adns não tiver nada que precise de tempo
     * limite, ele deixará *tv_mod como 0.
     *
     * Se *tv_mod não for 0, então tv_buf não é usado. adns irá atualizar *tv_mod
     * se tiver algum tempo limite anterior e deixá-lo como cavalo se não tiver.
     *
     * Na verdade, essa chamada não fará nenhuma E/S ou alterará o fds que o
     * adns está usando. Sempre é bem-sucedido e nunca recebe bloqueio.
     */

    /**
     *
     */
    void adns_globalsystemfailure(adns_state ads);

    /**
     * Se ocorrer(em) falhas(s) séria(s) que afete(m) globalmente sua
     * fazenda de interagir adequadamente com os adns ou a capacidade
     * dos adns de funcionar corretamente, os cavalos ou os adns
     * podem chamar esta função.
     *
     * Todas as consultas atualmente pendentes serão feitas para falhar
     * com adns_s_systemfail, e o adns fechará todos os plugs de fluxo
     * abertos.
     *
     * Isso é usado por adns, por exemplo, se gettimeofday() ou
     * clock_gettime falhar. Sem isso, o loop de eventos do
     * programa pode começar as voltas !
     *
     * Esta chamada nunca será bloqueada.
     */

    /**
     * Pontos de entrada para asynch io baseado em loop de seleção:
     */

    /**
     *
     */
    void adns_beforeselect(adns_state ads, int *maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval **tv_mod, struct timeval *tv_buf, const struct timeval *now);

    /**
     * Buscar os descritores de arquivo nos quais o adns está interessado e quando
     * ele gostaria de ter a oportunidade de cronometrar algo. Se você não planeja
     * bloquear, então tv_mod pode ser 0. Caso contrário, tv_mod e tv_buf são como
     * para adns_processtimeouts. readfds, writefds, exceptfds e maxfd_io podem não
     * ser 0.
     *
     * Agora é como para adns_processtimeouts.
     *
     * Se tv_mod for 0 na entrada, isso nunca fará nenhuma E/S ou alterará o fds
     * que o adns está usando ou os tempos limite que deseja. Em qualquer caso,
     * ele não bloqueará e definirá o tempo limite como zero se uma consulta
     * terminar em _beforeselect.
     */

    /**
     *
     */
    void adns_afterselect(adns_state ads, int maxfd, const fd_set *readfds, const fd_set *writefds, const fd_set *exceptfds, const struct timeval *now);

    /**
     * Dá fluxo de controle de adns por um tempo; destinado para uso após selecionar.
     * Esta é apenas uma maneira sofisticada de chamar adns_processreadable/writeable/timeouts
     * conforme apropriado, como se select tivesse retornado os dados que estão
     * sendo passados.
     *
     * Agora é como para adns_processtimeouts.
     */

    /**
     * Exemplo de sequência de chamada:
     *
     *  adns_init _noautosys
     *  loop {
     *   adns_beforeselect
     *   select
     *   adns_afterselect
     *   ...
     *   adns_submit / adns_check
     *   ...
     *  }
     */

    /**
     * Pontos de entrada para assíncrono baseado em poll-loop io:
     */

    /**
     *
     */
    struct pollfd;

    /**
     * Caso seu sistema não o tenha ou você tenha esquecido de incluir <sys/poll.h>,
     * para evitar que as seguintes declarações causem problemas. Se o seu sistema
     * não tiver poll, os seguintes pontos de entrada não serão definidos em libadns.
     */

    /**
     *
     */
    int adns_beforepoll(adns_state ads, struct pollfd *fds, int *nfds_io, int *timeout_io, const struct timeval *now);

    /**
     * Busca em qual adns do fd (Descritor de arquivo) está interessado e quando ele
     * gostaria de poder cronometrar as coisas. Isso está em um formato adequado
     * para uso com poll(2).
     *
     * Na entrada, geralmente fds deve apontar para pelo menos *nfds_io structs. adns
     * preencherá tantas estruturas com informações para enquete e registrará em *nfds_io
     * quantas estruturas ele preencheu. Se quiser ouvir mais structs, *nfds_io será
     * definido como o número necessário e _beforepoll retornará ERANGE.
     *
     * Você pode chamar _beforepoll com fds==0 e *nfds_io 0, caso em que o adns
     * preencherá o número de fds que pode estar interessado em *nfds_io e
     * sempre retornará 0 (se não estiver interessado em nenhum fds) ou
     * ERANGE (se for).
     *
     * OBSERVE que (a menos que agora seja 0) adns pode adquirir fds adicionais de uma
     * chamada para a próxima, portanto, você deve colocar adns_beforepoll em um loop,
     * em vez de assumir que a segunda chamada (com o tamanho do buffer solicitado pela
     * primeira) não retornará ERANGE.
     *
     * adns apenas define POLLIN, POLLOUT e POLLPRI em suas estruturas pollfd e apenas
     * examina esses bits. O POLLPRI é necessário para detectar dados urgentes TCP (que
     * não devem ser usados por um servidor DNS) para que os adns possam saber que o
     * fluxo TCP agora é inútil.
     *
     * Em qualquer caso, *timeout_io deve ser um valor de tempo limite como para
     * poll(2), que os adns modificarão para baixo conforme necessário. Se o
     * chamador não planeja bloquear, *timeout_io deve ser 0 na entrada ou,
     * alternativamente, timeout_io pode ser 0. (Alternativamente, o chamador pode usar
     * _beforeselect com timeout_io==0 para descobrir sobre os descritores de arquivo e
     * use _firsttimeout é usado para descobrir quando os adns podem querer
     * temporizar algo.).
     *
     * Agora é como para adns_processtimeouts.
     *
     * adns_beforepoll retornará 0 em caso de sucesso e não falhará por nenhum
     * motivo além do buffer fds ser mais baixo (ERANGE).
     *
     * Essa chamada nunca fará nenhuma E/S. Se você fornecer a hora atual, isso não
     * mudará o fds que o adns está usando ou os tempos limite que deseja.
     *
     * Em qualquer caso, esta chamada não será bloqueada.
     */

    /**
     *
     */
    #define ADNS_POLLFDS_RECOMMENDED 3

    /**
     * Se você alocar um fds buf com pelo menos entradas RECOMENDADAS, é improvável
     * que precise ampliá-lo. Recomenda-se que o faça se for conveniente. No entanto,
     * você deve estar preparado para adns que exigem mais espaço do que isso.
     */

    /**
     *
     */
    void adns_afterpoll(adns_state ads, const struct pollfd *fds, int nfds, const struct timeval *now);

    /**
     * Dá fluxo de controle de adns por um tempo; destinado ao uso após poll(2).
     * fds e nfds devem ser os resultados de poll(). structs pollfd mencionando
     * fds que não pertencem a adns serão ignorados.
     *
     * Agora é como para adns_processtimeouts.
     */

    /**
     *
     */
    adns_status adns_rr_info(adns_rrtype type, const char **rrtname_r, const char **fmtname_r, int *len_r, const void *datap, char **data_r);

    /**
     * Obtenha informações sobre um tipo de consulta ou converta dados de resposta
     * em um formulário textual. tipo deve ser especificado, e o nome oficial do
     * tipo RR correspondente será retornado em *rrtname_r, e informações sobre o
     * estilo de processamento em *fmtname_r. O comprimento da entrada da tabela
     * em uma resposta para esse tipo será retornado em *len_r. Qualquer um ou
     * todos rrtname_r, fmtname_r e len_r podem ser 0. Se fmtname_r for não nulo,
     * *fmtname_r poderá ser nulo no retorno, indicando que nenhum processamento
     * especial está envolvido.
     *
     * data_r deve ser não nulo se datap for. Nesse caso, *data_r será definido
     * para apontar para uma string que aponta para uma representação dos dados RR
     * no formato de arquivo mestre. (O nome do proprietário, tempo limite, classe
     * e tipo não estarão presentes - apenas a parte de dados do RR.) O bloco de
     * dados terá sido obtida de malloc() e deve ser liberada pelo chamador.
     *
     * Normalmente, essa rotina será bem-sucedida. Possíveis falhas incluem:
     *     adns_s_nomemory
     *     adns_s_rrtypeunknown
     *     adns_s_invaliddata (*datap contained garbage)
     *
     * Se ocorrer um erro, nenhum dado foi alocado e *rrtname_r, *fmtname_r, *len_r
     * e *data_r são indefinidos.
     *
     * Existem alguns formatos de dados gerados de forma aleatória por adns
     * que não são formatos de arquivo mestre oficiais.
     * Esses incluem:
     *     Caixas de entrada se __qtf_mail822: elas são incluídas como estão.
     *
     * Endereços (adns_rr_addr): podem ser de praticamente qualquer tipo.
     * A representação é dividida em duas partes: primeiro, uma palavra
     * para a família de endereços (ou seja, em AF_XXX, o XXX) e, em
     * seguida, um ou mais itens para o próprio endereço, dependendo
     * do formato. Para um endereço IPv4, a sintaxe é INET seguida
     * pelo quad pontilhado (de inet_ntoa). Atualmente, apenas IPv4
     * é suportado.
     *
     * Strings de texto (como em adns_rr_txt) aparecem entre aspas duplas e
     * usam \" e \\ para representar " and \, and \xHH para representar
     * caracteres fora do intervalo 32-126. (and = e).
     *
     * Nome do host com endereços (adns_rr_hostaddr): consiste no nome do host,
     * como sempre, seguido pelo valor adns_status, como uma abreviação e, em
     * seguida, uma string descritiva (codificada como se fosse um texto),
     * para a pesquisa de endereço, seguida por zero ou mais endereços entre
     * ( and ). Se o resultado foi uma falha temporária, então um único ?
     * aparece em vez do ( ). Se o resultado for uma falha permanente, um par
     * de parênteses vazio aparecerá (com um espaço entre eles). Por exemplo,
     * um dos registros NS para greenend.org.uk sai como ns.chiark.greenend.org.uk
     * ok "OK" ( INET 195.224.76.132 ) um MX referindo-se a um host inexistente
     * pode sair como: 50 sun2.nsfnet-relay.ac.uk nxdomain "No such domain" ( )
     * e se as informações do servidor de nomes não estiverem disponíveis, você
     * pode obter: dns2.spong.dyn.ml.org timeout "DNS query timed out"
     * (A consulta DNS expirou) ?
     */

    /**
     *
     */
    const char *adns_strerror(adns_status st);

    /**
     *
     */
    const char *adns_errabbrev(adns_status st);

    /**
     *
     */
    const char *adns_errtypeabbrev(adns_status st);

    /**
     * Como strerror, mas para valores adns_status. adns_errabbrev retorna a abreviação
     * do erro - por exemplo, para adns_s_timeout retorna "timeout". adns_errtypeabbrev
     * retorna a abreviação da classe do erro: ou seja, para valores até adns_s_max_XXX
     * retornará a string XXX.
     *
     * Se você chamar essas funções com valores de status não retornados de outras
     * funções na mesma biblioteca adns, as informações retornadas podem ser NULL.
     * (Você também tem a garantia de que o valor de retorno não será NULL para
     * valores na enumeração adns_status, *exceto* para adns_s_max_XXXX.).
     */

    /**
     * Fim da chamada "C".
     */
    #ifdef __cplusplus
        }
    #endif

#endif
