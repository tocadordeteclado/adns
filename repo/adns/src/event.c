/**
 * event.c
 *     - núcleo do loop de eventos.
 *     - gerenciamento de conexão TCP.
 *     - funções de verificação/espera visíveis ao usuário e
 *       relacionadas ao loop de eventos.
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

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "internal.h"
#include "tvarith.h"


/**
 * Gerenciamento de conexão TCP.
 */


/**
 *
 */
static void tcp_close(adns_state ads)
{
    close(ads->tcpsocket);
    ads->tcpsocket = -1;
    ads->tcprecv.used = ads->tcprecv_skip = ads->tcpsend.used = 0;
}

/**
 *
 */
void adns__tcp_broken(adns_state ads, const char *what, const char *why)
{
    int serv;

    adns_query qu;
    assert(ads->tcpstate == server_connecting || ads->tcpstate == server_ok);
    serv = ads->tcpserver;

    if (what)
    {
        adns__warn(ads, serv, 0, "TCP connection failed: %s: %s", what, why);
    }

    if (ads->tcpstate == server_connecting)
    {
        /**
         * Contador como uma nova tentativa para todas as consultas
         * aguardando TCP.
         */
        for (qu = ads->tcpw.head; qu; qu = qu->next)
        {
            qu->retries++;
        }
    }

    tcp_close(ads);
    ads->tcpstate = server_broken;
    ads->tcpserver = (serv + 1)%ads->nservers;
}

/**
 *
 */
static void tcp_connected(adns_state ads, struct timeval now)
{
    adns_query qu, nqu;

    adns__debug(ads, ads->tcpserver, 0, "TCP connected");
    ads->tcpstate = server_ok;

    for (qu = ads->tcpw.head; qu && ads->tcpstate == server_ok; qu = nqu)
    {
        nqu = qu->next;
        assert(qu->state == query_tcpw);
        adns__querysend_tcp(qu, now);
    }
}

/**
 *
 */
static void tcp_broken_events(adns_state ads)
{
    adns_query qu, nqu;
    assert(ads->tcpstate == server_broken);

    for (qu = ads->tcpw.head; qu; qu = nqu)
    {
        nqu = qu->next;
        assert(qu->state == query_tcpw);

        if (qu->retries > ads->nservers)
        {
            LIST_UNLINK(ads->tcpw, qu);
            adns__query_fail(qu, adns_s_allservfail);
        }
    }

    ads->tcpstate = server_disconnected;
}

/**
 *
 */
void adns__tcp_tryconnect(adns_state ads, struct timeval now)
{
    int r, fd, tries;
    adns_rr_addr *addr;
    struct protoent *proto;

    for (tries = 0; tries < ads->nservers; tries++)
    {
        switch (ads->tcpstate)
        {
            case server_connecting:
            case server_ok:
            case server_broken:
                return;

            case server_disconnected:
                break;

            default:
                abort();
        }

        assert(!ads->tcpsend.used);
        assert(!ads->tcprecv.used);
        assert(!ads->tcprecv_skip);
        proto = getprotobyname("tcp");

        if (!proto)
        {
            adns__diag(ads, -1, 0, "unable to find protocol no. for TCP !");

            return;
        }

        addr = &ads->servers[ads->tcpserver];
        fd = socket(addr->addr.sa.sa_family, SOCK_STREAM, proto->p_proto);

        if (fd < 0)
        {
            adns__diag(ads, -1, 0, "cannot create TCP socket: %s", strerror(errno));

            return;
        }

        r = adns__setnonblock(ads, fd);

        if (r)
        {
            adns__diag(ads, -1, 0, "cannot make TCP socket nonblocking: %s", strerror(r));
            close(fd);

            return;
        }

        r = connect(fd, &addr->addr.sa, addr->len);
        ads->tcpsocket = fd;
        ads->tcpstate = server_connecting;

        if (r == 0)
        {
            tcp_connected(ads, now);

            return;
        }

        if (errno == EWOULDBLOCK || errno == EINPROGRESS)
        {
            ads->tcptimeout = now;
            timevaladd(&ads->tcptimeout, TCPCONNMS);

            return;
        }

        adns__tcp_broken(ads, "connect", strerror(errno));
        tcp_broken_events(ads);
    }
}

/**
 * Funções de tratamento de tempo limite.
 */

/**
 *
 */
int adns__gettimeofday(adns_state ads, struct timeval *tv)
{
    if (!(ads->iflags & adns_if_monotonic))
    {
        return gettimeofday(tv, 0);
    }

    struct timespec ts;
    int r = clock_gettime(CLOCK_MONOTONIC, &ts);

    if (r)
    {
        return r;
    }

    tv->tv_sec =  ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;

    return 0;
}

/**
 *
 */
void adns__must_gettimeofday(adns_state ads, const struct timeval **now_io, struct timeval *tv_buf)
{
    const struct timeval *now;
    int r;

    now = *now_io;

    if (now)
    {
        return;
    }

    r = adns__gettimeofday(ads, tv_buf);

    if (!r)
    {
        *now_io = tv_buf;

        return;
    }

    adns__diag(ads, -1, 0, "gettimeofday/clock_gettime failed: %s", strerror(errno));
    adns_globalsystemfailure(ads);

    return;
}

/**
 *
 */
static void inter_immed(struct timeval **tv_io, struct timeval *tvbuf)
{
    struct timeval *rbuf;

    if (!tv_io)
    {
        return;
    }

    rbuf = *tv_io;

    if (!rbuf)
    {
        *tv_io = rbuf = tvbuf;
    }

    timerclear(rbuf);
}

/**
 *
 */
static void inter_maxto(struct timeval **tv_io, struct timeval *tvbuf, struct timeval maxto)
{
    struct timeval *rbuf;

    if (!tv_io)
    {
        return;
    }

    rbuf = *tv_io;

    if (!rbuf)
    {
        *tvbuf = maxto;
        *tv_io = tvbuf;
    } else
    {
        if (timercmp(rbuf, &maxto,>))
        {
            *rbuf = maxto;
        }
    }

    /**
     * fprintf(stderr,"inter_maxto maxto=%ld.%06ld result=%ld.%06ld\n",
     * maxto.tv_sec,maxto.tv_usec,(**tv_io).tv_sec,(**tv_io).tv_usec);
     */
}

/**
 *
 */
static void inter_maxtoabs(struct timeval **tv_io, struct timeval *tvbuf, struct timeval now, struct timeval maxtime)
{
    /**
     * tv_io possivelmente 0.
     */
    ldiv_t dr;

    /**
     * fprintf(stderr,"inter_maxtoabs now=%ld.%06ld maxtime=%ld.%06ld\n",
     * now.tv_sec,now.tv_usec,maxtime.tv_sec,maxtime.tv_usec);
     */
    if (!tv_io)
    {
        return;
    }

    maxtime.tv_sec -= (now.tv_sec + 2);
    maxtime.tv_usec -= (now.tv_usec - 2000000);

    dr= ldiv(maxtime.tv_usec, 1000000);
    maxtime.tv_sec += dr.quot;
    maxtime.tv_usec -= dr.quot * 1000000;

    if (maxtime.tv_sec < 0)
    {
        timerclear(&maxtime);
    }

    inter_maxto(tv_io, tvbuf, maxtime);
}

/**
 *
 */
static void timeouts_queue(adns_state ads, int act, struct timeval **tv_io, struct timeval *tvbuf, struct timeval now, struct query_queue *queue)
{
    adns_query qu, nqu;
    struct timeval expires;

    for (qu = queue->head; qu; qu = nqu)
    {
        nqu = qu->next;

        /**
         * relógio rebobinado.
         */
        if (timercmp(&now, &qu->timeout_started,<))
        {
            qu->timeout_started = now;
        }

        expires = qu->timeout_started;
        timevaladd(&expires, qu->timeout_ms);

        if (!timercmp(&now, &expires,>))
        {
            inter_maxtoabs(tv_io, tvbuf, now, expires);
        } else
        {
            if (!act)
            {
                inter_immed(tv_io, tvbuf);

                return;
            }

            LIST_UNLINK(*queue, qu);

            if (qu->state != query_tosend)
            {
                adns__query_fail(qu, adns_s_timeout);
            } else
            {
                adns__query_send(qu, now);
            }

            nqu = queue->head;
        }
    }
}

/**
 *
 */
static void tcp_events(adns_state ads, int act, struct timeval **tv_io, struct timeval *tvbuf, struct timeval now)
{
    for (;;)
    {
        switch (ads->tcpstate)
        {
            case server_broken:
                if (!act)
                {
                    inter_immed(tv_io, tvbuf);

                    return;
                }

                tcp_broken_events(ads);

            case server_disconnected:
                if (!ads->tcpw.head)
                {
                    return;
                }

                if (!act)
                {
                    inter_immed(tv_io, tvbuf);

                    return;
                }

                adns__tcp_tryconnect(ads, now);
                break;

            case server_ok:
                if (ads->tcpw.head)
                {
                    return;
                }

                if (!ads->tcptimeout.tv_sec)
                {
                    assert(!ads->tcptimeout.tv_usec);
                    ads->tcptimeout = now;

                    timevaladd(&ads->tcptimeout, TCPIDLEMS);
                }

            case server_connecting:
                if (!act || !timercmp(&now, &ads->tcptimeout,>))
                {
                    inter_maxtoabs(tv_io, tvbuf, now, ads->tcptimeout);

                    return;
                }

                {
                    /**
                     * O tempo limite do protocolo de rede TCP ocorreu.
                     */
                    switch (ads->tcpstate)
                    {
                        /**
                         * Falhou ao conectar.
                         */
                        case server_connecting:
                            adns__tcp_broken(ads, "unable to make connection", "timed out");
                            break;

                        /**
                         * tempo limite parado.
                         */
                        case server_ok:
                            tcp_close(ads);
                            ads->tcpstate = server_disconnected;

                            return;

                        default:
                            abort();
                    }
                }

                break;

            default:
                abort();
        }
    }

    return;
}

/**
 *
 */
void adns__timeouts(adns_state ads, int act, struct timeval **tv_io, struct timeval *tvbuf, struct timeval now)
{
    timeouts_queue(ads, act, tv_io, tvbuf, now, &ads->udpw);
    timeouts_queue(ads, act, tv_io, tvbuf, now, &ads->tcpw);
    tcp_events(ads, act, tv_io, tvbuf, now);
}

/**
 *
 */
void adns_firsttimeout(adns_state ads, struct timeval **tv_io, struct timeval *tvbuf, struct timeval now)
{
    adns__consistency(ads, 0, cc_enter);
    adns__timeouts(ads, 0, tv_io, tvbuf, now);
    adns__returning(ads, 0);
}

/**
 *
 */
void adns_processtimeouts(adns_state ads, const struct timeval *now)
{
    struct timeval tv_buf;

    adns__consistency(ads, 0, cc_enter);
    adns__must_gettimeofday(ads, &now, &tv_buf);

    if (now)
    {
        adns__timeouts(ads, 1, 0, 0, *now);
    }

    adns__returning(ads, 0);
}

/**
 * Funções de manipulação de fd (Descritor de arquivos). Estes são o nível mais alto
 * do verdadeiro trabalho de recepção e, muitas vezes, de transmissão.
 */

/**
 *
 */
int adns__pollfds(adns_state ads, struct pollfd pollfds_buf[MAX_POLLFDS])
{
    /**
     * Retorna o número de entradas preenchidas. Sempre zera revents.
     */
    int nwanted = 0;

    #define ADD_POLLFD(wantfd, wantevents) \
        do \
        { \
            pollfds_buf[nwanted].fd = (wantfd); \
            pollfds_buf[nwanted].events = (wantevents); \
            pollfds_buf[nwanted].revents = 0; \
            nwanted++; \
        } while(0)

    int i;

    assert(MAX_POLLFDS == MAXUDP + 1);

    for (i = 0; i < ads->nudpsockets; i++)
    {
        ADD_POLLFD(ads->udpsockets[i].fd, POLLIN);
    }

    switch (ads->tcpstate)
    {
        case server_disconnected:
        case server_broken:
            break;

        case server_connecting:
            ADD_POLLFD(ads->tcpsocket, POLLOUT);
            break;

        case server_ok:
            ADD_POLLFD(ads->tcpsocket, ads->tcpsend.used ? POLLIN|POLLOUT|POLLPRI : POLLIN|POLLPRI);
            break;

        default:
            abort();
    }

    assert(nwanted <= MAX_POLLFDS);

    #undef ADD_POLLFD

    return nwanted;
}

/**
 *
 */
int adns_processreadable(adns_state ads, int fd, const struct timeval *now)
{
    int want, dgramlen, r, i, serv, old_skip;
    socklen_t udpaddrlen;
    byte udpbuf[DNS_MAXUDP];

    char addrbuf[ADNS_ADDR2TEXT_BUFLEN];
    struct udpsocket *udp;
    adns_sockaddr udpaddr;

    adns__consistency(ads, 0, cc_enter);

    switch (ads->tcpstate)
    {
        case server_disconnected:
        case server_broken:
        case server_connecting:
            break;

        case server_ok:
            if (fd != ads->tcpsocket)
            {
                break;
            }

            assert(!ads->tcprecv_skip);

            do
            {
                if (ads->tcprecv.used >= ads->tcprecv_skip + 2)
                {
                    dgramlen = ((ads->tcprecv.buf[ads->tcprecv_skip] << 8) | ads->tcprecv.buf[ads->tcprecv_skip + 1]);

                    if (ads->tcprecv.used >= ads->tcprecv_skip + 2 + dgramlen)
                    {
                        old_skip = ads->tcprecv_skip;
                        ads->tcprecv_skip += 2 + dgramlen;

                        adns__procdgram(ads, ads->tcprecv.buf + old_skip + 2, dgramlen, ads->tcpserver, 1, *now);
                        continue;
                    } else
                    {
                        want = 2 + dgramlen;
                    }
                } else
                {
                    want = 2;
                }

                ads->tcprecv.used -= ads->tcprecv_skip;
                memmove(ads->tcprecv.buf, ads->tcprecv.buf + ads->tcprecv_skip, ads->tcprecv.used);
                ads->tcprecv_skip = 0;

                if (!adns__vbuf_ensure(&ads->tcprecv, want))
                {
                    r = ENOMEM;

                    /**
                     * Pular lá em baixo.
                     */
                    goto xit;
                }

                assert(ads->tcprecv.used <= ads->tcprecv.avail);

                if (ads->tcprecv.used == ads->tcprecv.avail)
                {
                    continue;
                }

                r = read(ads->tcpsocket, ads->tcprecv.buf + ads->tcprecv.used, ads->tcprecv.avail - ads->tcprecv.used);

                if (r > 0)
                {
                    ads->tcprecv.used += r;
                } else
                {
                    if (r)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            r = 0;

                            /**
                             * Pular lá em baixo.
                             */
                            goto xit;
                        }

                        if (errno == EINTR)
                        {
                            continue;
                        }

                        if (errno_resources(errno))
                        {
                            r = errno;

                            /**
                             * Pular lá em baixo.
                             */
                            goto xit;
                        }
                    }

                    adns__tcp_broken(ads, "read", r ? strerror(errno) : "closed");
                }
            } while (ads->tcpstate == server_ok);

            r = 0;

            /**
             * Pular lá em baixo.
             */
            goto xit;

        default:
            abort();
    }

    for (i = 0; i < ads->nudpsockets; i++)
    {
        udp = &ads->udpsockets[i];

        if (fd != udp->fd)
        {
            continue;
        }

        for (;;)
        {
            udpaddrlen = sizeof(udpaddr);
            r = recvfrom(fd, udpbuf, sizeof(udpbuf), 0, &udpaddr.sa, &udpaddrlen);

            if (r < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    r = 0;

                    /**
                     * Pular lá em baixo.
                     */
                    goto xit;
                }

                if (errno == EINTR)
                {
                    continue;
                }

                if (errno_resources(errno))
                {
                    r = errno;

                    /**
                     * Pular lá em baixo.
                     */
                    goto xit;
                }

                adns__warn(ads, -1, 0, "datagram receive error: %s", strerror(errno));
                r = 0;

                /**
                 * Pular lá em baixo.
                 */
                goto xit;
            }

            for (serv = 0; serv < ads->nservers && !adns__sockaddrs_equal(&udpaddr.sa, &ads->servers[serv].addr.sa); serv++);

            if (serv >= ads->nservers)
            {
                adns__warn(ads, -1, 0, "datagram received from unknown nameserver %s", adns__sockaddr_ntoa(&udpaddr.sa, addrbuf));
                continue;
            }

            adns__procdgram(ads, udpbuf, r, serv, 0, *now);
        }

        break;
    }

    r = 0;

    xit:
        adns__returning(ads, 0);

        return r;
}

/**
 *
 */
int adns_processwriteable(adns_state ads, int fd, const struct timeval *now)
{
    int r;

    adns__consistency(ads, 0, cc_enter);

    switch (ads->tcpstate)
    {
        case server_disconnected:
        case server_broken:
            break;

        case server_connecting:
            if (fd != ads->tcpsocket)
            {
                break;
            }

            assert(ads->tcprecv.used == 0);
            assert(ads->tcprecv_skip == 0);

            for (;;)
            {
                /**
                 * Essa função pode ser chamada mesmo que o fd (Descritor de arquivos) não tenha
                 * sido sinalizado como gravável. Para conexão tcp asynch, temos que usar a capacidade
                 * de gravação para nos dizer que a conexão foi concluída (ou falhou), portanto, precisamos
                 * verificar novamente.
                 */
                fd_set writeable;

                struct timeval timeout =
                {
                    0,
                    0
                };

                FD_ZERO(&writeable);
                FD_SET(ads->tcpsocket, &writeable);
                r = select(ads->tcpsocket + 1, 0, &writeable, 0, &timeout);

                if (r == 0)
                {
                    break;
                }

                if (r < 0)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    adns__tcp_broken(ads, "select", "failed connecting writeability check");
                    r = 0;

                    /**
                     * Pular lá embaixo.
                     */
                    goto xit;
                }

                assert(FD_ISSET(ads->tcpsocket, &writeable));

                if (!adns__vbuf_ensure(&ads->tcprecv, 1))
                {
                    r = ENOMEM;

                    /**
                     * Pular lá embaixo.
                     */
                    goto xit;
                }

                r = read(ads->tcpsocket, ads->tcprecv.buf, 1);

                if (r == 0 || (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)))
                {
                    tcp_connected(ads, *now);
                    r = 0;

                    /**
                     * Pular lá embaixo.
                     */
                    goto xit;
                }

                if (r > 0)
                {
                    adns__tcp_broken(ads, "connect/read", "sent data before first request");
                    r = 0;

                    /**
                     * Pular lá embaixo.
                     */
                    goto xit;
                }

                if (errno == EINTR)
                {
                    continue;
                }

                if (errno_resources(errno))
                {
                    r = errno;

                    /**
                     * Pular lá embaixo.
                     */
                    goto xit;
                }

                adns__tcp_broken(ads, "connect/read", strerror(errno));
                r = 0;

                /**
                 * Pular lá embaixo.
                 */
                goto xit;

                /**
                 * DNS não alcançado.
                 */
            }

        case server_ok:
            if (fd != ads->tcpsocket)
            {
                break;
            }

            while (ads->tcpsend.used)
            {
                adns__sigpipe_protect(ads);
                r = write(ads->tcpsocket, ads->tcpsend.buf, ads->tcpsend.used);
                adns__sigpipe_unprotect(ads);

                if (r < 0)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        r = 0;

                        /**
                         * Pular lá embaixo.
                         */
                        goto xit;
                    }

                    if (errno_resources(errno))
                    {
                        r = errno;

                        /**
                         * Pular lá embaixo.
                         */
                        goto xit;
                    }

                    adns__tcp_broken(ads, "write", strerror(errno));
                    r = 0;

                    /**
                     * Pular lá embaixo.
                     */
                    goto xit;
                } else if (r > 0)
                {
                    assert(r <= ads->tcpsend.used);
                    ads->tcpsend.used -= r;
                    memmove(ads->tcpsend.buf, ads->tcpsend.buf + r, ads->tcpsend.used);
                }
            }

            r = 0;

            /**
             * Pular lá embaixo.
             */
            goto xit;

        default:
            abort();
    }

    r = 0;

    xit:
        adns__returning(ads, 0);

        return r;
}

/**
 *
 */
int adns_processexceptional(adns_state ads, int fd, const struct timeval *now)
{
    adns__consistency(ads, 0, cc_enter);

    switch (ads->tcpstate)
    {
        case server_disconnected:
        case server_broken:
            break;

        case server_connecting:
        case server_ok:
            if (fd != ads->tcpsocket)
            {
                break;
            }

            adns__tcp_broken(ads, "poll/select", "exceptional condition detected");
            break;

        default:
            abort();
    }

    adns__returning(ads, 0);

    return 0;
}

/**
 *
 */
static void fd_event(adns_state ads, int fd, int revent, int pollflag, int maxfd, const fd_set *fds, int (*func)(adns_state, int fd, const struct timeval *now), struct timeval now, int *r_r)
{
    int r;

    if (!(revent & pollflag))
    {
        return;
    }

    if (fds && !(fd < maxfd && FD_ISSET(fd, fds)))
    {
        return;
    }

    r = func(ads, fd, &now);

    if (r)
    {
        if (r_r)
        {
            *r_r = r;
        } else
        {
            adns__diag(ads,-1,0,"process fd failed after select: %s",strerror(errno));
            adns_globalsystemfailure(ads);
        }
    }
}

/**
 *
 */
void adns__fdevents(adns_state ads, const struct pollfd *pollfds, int npollfds, int maxfd, const fd_set *readfds, const fd_set *writefds, const fd_set *exceptfds, struct timeval now, int *r_r)
{
    int i, fd, revents;

    for (i = 0; i < npollfds; i++)
    {
        fd = pollfds[i].fd;

        if (fd >= maxfd)
        {
            maxfd = fd + 1;
        }

        revents = pollfds[i].revents;

        #define EV(pollfl, fds, how) fd_event(ads, fd, revents, pollfl, maxfd, fds, adns_process##how, now, r_r)

        EV(POLLIN, readfds, readable);
        EV(POLLOUT, writefds, writeable);
        EV(POLLPRI, exceptfds, exceptional);

        #undef EV
    }
}

/**
 * Wrappers para select(2).
 */

void adns_beforeselect(adns_state ads, int *maxfd_io, fd_set *readfds_io, fd_set *writefds_io, fd_set *exceptfds_io, struct timeval **tv_mod, struct timeval *tv_tobuf, const struct timeval *now)
{
    struct timeval tv_nowbuf;
    struct pollfd pollfds[MAX_POLLFDS];
    int i, fd, maxfd, npollfds;

    adns__consistency(ads,0,cc_enter);

    if (tv_mod && (!*tv_mod || (*tv_mod)->tv_sec || (*tv_mod)->tv_usec))
    {
        /**
         * O chamador de cavalos está planejando pausar (sleep).
         */
        adns__must_gettimeofday(ads, &now, &tv_nowbuf);

        if (!now)
        {
            inter_immed(tv_mod, tv_tobuf); 

            /**
             * Pular lá embaixo.
             */
            goto xit;
        }

        adns__timeouts(ads, 0, tv_mod, tv_tobuf, *now);
    }

    npollfds = adns__pollfds(ads, pollfds);
    maxfd = *maxfd_io;

    for (i = 0; i < npollfds; i++)
    {
        fd = pollfds[i].fd;

        if (fd >= maxfd)
        {
            maxfd = fd + 1;
        }

        if (pollfds[i].events & POLLIN)
        {
            FD_SET(fd, readfds_io);
        }

        if (pollfds[i].events & POLLOUT)
        {
            FD_SET(fd, writefds_io);
        }

        if (pollfds[i].events & POLLPRI)
        {
            FD_SET(fd, exceptfds_io);
        }
    }

    *maxfd_io = maxfd;

    xit:
        adns__returning(ads, 0);
}

/**
 *
 */
void adns_afterselect(adns_state ads, int maxfd, const fd_set *readfds, const fd_set *writefds, const fd_set *exceptfds, const struct timeval *now)
{
    struct timeval tv_buf;
    struct pollfd pollfds[MAX_POLLFDS];
    int npollfds, i;

    adns__consistency(ads,0,cc_enter);
    adns__must_gettimeofday(ads,&now,&tv_buf);

    if (!now)
    {
        /**
         * Pular lá embaixo.
         */
        goto xit;
    }

    adns_processtimeouts(ads, now);
    npollfds = adns__pollfds(ads, pollfds);

    for (i = 0; i < npollfds; i++)
    {
        pollfds[i].revents = POLLIN | POLLOUT | POLLPRI;
    }

    adns__fdevents(ads, pollfds, npollfds, maxfd, readfds, writefds, exceptfds, *now, 0);

    xit:
        adns__returning(ads, 0);
}

/**
 * Funções úteis gerais.
 */

/**
 *
 */
void adns_globalsystemfailure(adns_state ads)
{
    /**
     * Não deve ser chamado por adns durante o processamento real de uma consulta
     * específica, pois ele reinsere adns. Só é seguro chamar em situações em que
     * seria seguro chamar adns_returning.
     */
    adns__consistency(ads, 0, cc_enter);

    for (;;)
    {
        adns_query qu;

        #define GSF_QQ(QQ) \
            if ((qu = ads->QQ.head)) \
            { \
                LIST_UNLINK(ads->QQ, qu); \
                adns__query_fail(qu, adns_s_systemfail); \
                continue; \
            }

        GSF_QQ(udpw);
        GSF_QQ(tcpw);

        #undef GSF_QQ
        break;
    }

    switch (ads->tcpstate)
    {
        case server_connecting:
        case server_ok:
            adns__tcp_broken(ads, 0, 0);
            break;

        case server_disconnected:
        case server_broken:
            break;

        default:
            abort();
    }

    adns__returning(ads, 0);
}

/**
 *
 */
int adns_processany(adns_state ads)
{
    int r, i;
    struct timeval now;
    struct pollfd pollfds[MAX_POLLFDS];
    int npollfds;

    adns__consistency(ads, 0, cc_enter);
    r = adns__gettimeofday(ads, &now);

    if (!r)
    {
        adns_processtimeouts(ads, &now);
    }

    /**
     * Nós apenas usamos adns__fdevents para fazer um loop sobre os fd's que estão continuando.
     * Isso parece mais sensato do que chamar select, já que estamos. mas provavelmente só quer
     * fazer uma leitura em um ou dois fds de qualquer maneira.
     */
    npollfds = adns__pollfds(ads, pollfds);

    for (i = 0; i < npollfds; i++)
    {
        pollfds[i].revents = pollfds[i].events & ~POLLPRI;
    }

    adns__fdevents(ads, pollfds, npollfds, 0, 0, 0, 0, now, &r);
    adns__returning(ads, 0);

    return 0;
}

/**
 *
 */
void adns__autosys(adns_state ads, struct timeval now)
{
    if (ads->iflags & adns_if_noautosys)
    {
        return;
    }

    adns_processany(ads);
}

/**
 *
 */
int adns__internal_check(adns_state ads, adns_query *query_io, adns_answer **answer, void **context_r)
{
    adns_query qu;
    qu = *query_io;

    if (!qu)
    {
        if (ads->output.head)
        {
            qu = ads->output.head;
        } else if (ads->udpw.head || ads->tcpw.head)
        {
            return EAGAIN;
        } else
        {
            return ESRCH;
        }
    } else
    {
        if (qu->id >= 0)
        {
            return EAGAIN;
        }
    }

    LIST_UNLINK(ads->output, qu);
    *answer = qu->answer;

    if (context_r)
    {
        *context_r = qu->ctx.ext;
    }

    *query_io = qu;
    free(qu);

    return 0;
}

/**
 *
 */
int adns_wait(adns_state ads, adns_query *query_io, adns_answer **answer_r, void **context_r)
{
    int r, maxfd, rsel;
    fd_set readfds, writefds, exceptfds;
    struct timeval tvbuf, *tvp;
    adns__consistency(ads, *query_io, cc_enter);

    for (;;)
    {
        r = adns__internal_check(ads, query_io,answer_r, context_r);

        if (r != EAGAIN)
        {
            break;
        }

        maxfd = 0;
        tvp = 0;

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);

        adns_beforeselect(ads, &maxfd, &readfds, &writefds, &exceptfds, &tvp, &tvbuf, 0);
        assert(tvp);

        rsel = select(maxfd, &readfds, &writefds, &exceptfds, tvp);

        if (rsel == -1)
        {
            if (errno == EINTR)
            {
                if (ads->iflags & adns_if_eintr)
                {
                    r = EINTR;

                    break;
                }
            } else
            {
                adns__diag(ads, -1, 0, "select failed in wait: %s", strerror(errno));
                adns_globalsystemfailure(ads);
            }
        } else
        {
            assert(rsel >= 0);
            adns_afterselect(ads, maxfd, &readfds, &writefds, &exceptfds, 0);
        }
    }

    adns__returning(ads,0);

    return r;
}

/**
 *
 */
int adns_check(adns_state ads, adns_query *query_io, adns_answer **answer_r, void **context_r)
{
    struct timeval now;
    int r;

    adns__consistency(ads, *query_io, cc_enter);
    r = adns__gettimeofday(ads, &now);

    if (!r)
    {
        adns__autosys(ads, now);
    }

    r = adns__internal_check(ads, query_io, answer_r, context_r);
    adns__returning(ads, 0);

    return r;
}
