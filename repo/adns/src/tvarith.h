/**
 * tvarith.h
 *     - funções inline estáticas para fazer aritmética em timevals.
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

#ifndef ADNS_TVARITH_H_INCLUDED
#define ADNS_TVARITH_H_INCLUDED

    /**
     *
     */
    static inline void timevaladd(struct timeval *tv_io, long ms)
    {
        struct timeval tmp;
        assert(ms >= 0);

        tmp = *tv_io;
        tmp.tv_usec += (ms%1000)*1000;
        tmp.tv_sec += ms / 1000;

        if (tmp.tv_usec >= 1000000)
        {
            tmp.tv_sec++;
            tmp.tv_usec -= 1000000;
        }

        *tv_io= tmp;
    }

#endif
