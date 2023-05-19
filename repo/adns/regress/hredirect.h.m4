m4_dnl
m4_dnl hredirect.h.m4
m4_dnl (parte de uma estrutura de teste complexa, não da biblioteca)
m4_dnl - redefinições de chamadas do sistema.
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

#ifndef HREDIRECT_H_INCLUDED
#define HREDIRECT_H_INCLUDED

    #include "hsyscalls.h"

hm_create_nothing
m4_define(`hm_syscall', `#undef $1
#define $1 H$1')
m4_define(`hm_specsyscall',`#undef $2
#define $2 H$2')
m4_include(`hsyscalls.i4')

m4_dnl único site de uso é a definição de principal.
    #define main(C, V) Hmain(C, V); int Hmain(C, V)

#endif
