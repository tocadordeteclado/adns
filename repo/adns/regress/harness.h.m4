m4_dnl
m4_dnl harness.h.m4
m4_dnl (parte de uma estrutura de teste complexa, não da biblioteca)
m4_dnl - função e outras declarações
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

#ifndef HARNESS_H_INCLUDED
#define HARNESS_H_INCLUDED

    #include "internal.h"
    #include "hsyscalls.h"

    /**
     * Existe uma função Q (Q para Question) para cada chamada de sistema;
     * ele constrói uma string representando a chamada e chama Q_str nela,
     * ou constrói em vb e chama Q_vb;
     */

hm_create_proto_q
m4_define(`hm_syscall', `void Q$1(hm_args_massage($3,void));')
m4_define(`hm_specsyscall', `')
m4_include(`hsyscalls.i4')
hm_stdsyscall_close

    /**
     *
     */
    void Q_vb(void);

    /**
     *
     */
    extern void Tshutdown(void);

    /**
     * Funções gerais de ajuda.
     */

    /**
     *
     */
    void Tfailed(const char *why);

    /**
     *
     */
    void Toutputerr(void);

    /**
     *
     */
    void Tnomem(void);

    /**
     *
     */
    void Tfsyscallr(const char *fmt, ...) PRINTFFORMAT(1,2);

    /**
     *
     */
    void Tensuresetup(void);

    /**
     *
     */
    void Tmust(const char *call, const char *arg, int cond);

    /**
     *
     */
    void Tvbf(const char *fmt, ...) PRINTFFORMAT(1,2);

    /**
     *
     */
    void Tvbvf(const char *fmt, va_list al);

    /**
     *
     */
    void Tvbfdset(int max, const fd_set *set);

    /**
     *
     */
    void Tvbpollfds(const struct pollfd *fds, int nfds);

    /**
     *
     */
    void Tvbaddr(const struct sockaddr *addr, int addrlen);

    /**
     *
     */
    void Tvbbytes(const void *buf, int len);

    /**
     *
     */
    void Tvberrno(int e);

    /**
     *
     */
    void Tvba(const char *str);

    /**
     * Compartilhamentos de escopo global.
     */

    /**
     *
     */
    extern vbuf vb;

    /**
     *
     */
    extern struct timeval currenttime;

    /**
     *
     */
    extern const struct Terrno { const char *n; int v; } Terrnos[];

    /**
     *
     */
    extern const int Tnerrnos;

    /**
     * Casos especiais.
     */

    /**
     *
     */
    void Texit(int rv) NONRETURNING;

    /**
     *
     */
    void Tcommonshutdown(void);

    /**
     *
     */
    void Tmallocshutdown(void);

    /**
     *
     */
    int Ttestinputfd(void);

    /**
     *
     */
    void T_gettimeofday_hook(void);

#endif
