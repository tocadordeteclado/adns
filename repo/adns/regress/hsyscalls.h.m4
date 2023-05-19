m4_dnl
m4_dnl hsyscalls.h.m4
m4_dnl (parte de uma estrutura de teste complexa, não da biblioteca)
m4_dnl - protótipos de redefinições de chamadas de sistema.
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

#ifndef HSYSCALLS_H_INCLUDED
#define HSYSCALLS_H_INCLUDED

    #include <sys/types.h>
    #include <sys/time.h>
    #include <sys/socket.h>
    #include <sys/uio.h>
    #include <unistd.h>

    #ifdef HAVE_POLL
    #include <sys/poll.h>
    #endif

hm_create_proto_h
m4_define(`hm_syscall', `int H$1(hm_args_massage($3,void));')
m4_define(`hm_specsyscall', `$1 H$2($3)$4;')
m4_include(`hsyscalls.i4')

#endif
