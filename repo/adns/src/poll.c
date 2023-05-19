/**
 * poll.c - wrappers para poll(2).
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

#include <limits.h>
#include <string.h>

#include "internal.h"


/**
 *
 */
#ifdef HAVE_POLL

    /**
     *
     */
    int adns_beforepoll(adns_state ads, struct pollfd *fds, int *nfds_io, int *timeout_io, const struct timeval *now)
    {
        struct timeval tv_nowbuf;
        struct timeval tv_tobuf;
        struct timeval *tv_to;
        int space;
        int found;
        int timeout_ms;
        int r;
        struct pollfd fds_tmp[MAX_POLLFDS];

        adns__consistency(ads, 0, cc_enter);

        if (timeout_io)
        {
            adns__must_gettimeofday(ads, &now, &tv_nowbuf);

            if (!now)
            {
                *nfds_io = 0;
                r = 0;

                /**
                 *
                 */
                goto xit;
            }

            timeout_ms = *timeout_io;

            if (timeout_ms == -1)
            {
                tv_to = 0;
            } else
            {
                tv_tobuf.tv_sec = timeout_ms / 1000;
                tv_tobuf.tv_usec = (timeout_ms % 1000) * 1000;
                tv_to = &tv_tobuf;
            }

            adns__timeouts(ads, 0, &tv_to, &tv_tobuf, *now);

            if (tv_to)
            {
                assert(tv_to == &tv_tobuf);
                timeout_ms = (tv_tobuf.tv_usec + 999) / 1000;

                assert(tv_tobuf.tv_sec < (INT_MAX - timeout_ms) / 1000);
                timeout_ms += tv_tobuf.tv_sec * 1000;
            } else
            {
                timeout_ms = -1;
            }

            *timeout_io = timeout_ms;
        }

        space = *nfds_io;

        if (space >= MAX_POLLFDS)
        {
            found = adns__pollfds(ads, fds);
            *nfds_io = found;
        } else
        {
            found = adns__pollfds(ads, fds_tmp);
            *nfds_io = found;

            if (space < found)
            {
                r = ERANGE;

                /**
                 * Pular lá embaixo.
                 */
                goto xit;
            }

            memcpy(fds, fds_tmp, sizeof(struct pollfd) * found);
        }

        r = 0;

        xit:
            adns__returning(ads, 0);
            return r;
    }

    /**
     *
     */
    void adns_afterpoll(adns_state ads, const struct pollfd *fds, int nfds, const struct timeval *now)
    {
        struct timeval tv_buf;

        adns__consistency(ads, 0, cc_enter);
        adns__must_gettimeofday(ads, &now, &tv_buf);

        if (now)
        {
            adns__timeouts(ads, 1, 0, 0, *now);

            /**
             * fdevents chama adns_processwriteable.
             */
            adns__intdone_process(ads);

            adns__fdevents(ads, fds, nfds, 0, 0, 0, 0, *now, 0);
        }

        adns__returning(ads, 0);
    }

    /**
     *
     */
    int adns_wait_poll(adns_state ads, adns_query *query_io, adns_answer **answer_r, void **context_r)
    {
        int r, nfds, to;
        struct pollfd fds[MAX_POLLFDS];

        adns__consistency(ads, 0, cc_enter);

        for (;;)
        {
            r = adns__internal_check(ads, query_io, answer_r, context_r);

            if (r != EAGAIN)
            {
                goto xit;
            }

            nfds = MAX_POLLFDS;
            to = -1;

            adns_beforepoll(ads, fds, &nfds, &to, 0);
            r = poll(fds, nfds, to);

            if (r == -1)
            {
                if (errno == EINTR)
                {
                    if (ads->iflags & adns_if_eintr)
                    {
                        r = EINTR;

                        /**
                         * Pular lá embaixo.
                         */
                        goto xit;
                    }
                } else
                {
                    adns__diag(ads, -1, 0, "poll failed in wait: %s", strerror(errno));
                    adns_globalsystemfailure(ads);
                }
            } else
            {
                assert(r >= 0);
                adns_afterpoll(ads, fds, nfds, 0);
            }
        }

        xit:
            adns__returning(ads, 0);
            return r;
    }

#endif
