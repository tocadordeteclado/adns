/**
 * reply.c.
 *     - Rotina principal de manipulação e análise para
 *       datagramas recebidos.
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

#include "internal.h"


/**
 *
 */
void adns__procdgram(adns_state ads, const byte *dgram, int dglen, int serv, int viatcp, struct timeval now)
{
    int cbyte;
    int rrstart;
    int wantedrrs;
    int rri;
    int foundsoa;
    int foundns;
    int cname_here;
    int id;
    int f1;
    int f2;
    int qdcount;
    int ancount;
    int nscount;
    int arcount;
    int flg_ra;
    int flg_rd;
    int flg_tc;
    int flg_qr;
    int opcode;
    int rrtype;
    int rrclass;
    int rdlength;
    int rdstart;
    int anstart;
    int nsstart;
    int ownermatched;
    int l;
    int nrrs;
    int restartfrom;
    unsigned long ttl;
    unsigned long soattl;
    const typeinfo *typei;

    adns_query qu, nqu;
    dns_rcode rcode;
    adns_status st;
    vbuf tempvb;
    byte *newquery, *rrsdata;
    parseinfo pai;

    if (dglen < DNS_HDRSIZE)
    {
        adns__diag(ads, serv, 0, "received datagram too short for message header (%d)", dglen);

        return;
    }

    cbyte = 0;

    GET_W(cbyte, id);
    GET_B(cbyte, f1);
    GET_B(cbyte, f2);
    GET_W(cbyte, qdcount);
    GET_W(cbyte, ancount);
    GET_W(cbyte, nscount);
    GET_W(cbyte, arcount);

    assert(cbyte == DNS_HDRSIZE);

    flg_qr = f1&0x80;
    opcode = (f1&0x78) >> 3;
    flg_tc = f1&0x02;
    flg_rd = f1&0x01;
    flg_ra = f2&0x80;
    rcode = (f2&0x0f);
    cname_here = 0;

    if (!flg_qr)
    {
        adns__diag(ads, serv, 0, "server sent us a query, not a response");

        return;
    }

    if (opcode)
    {
        adns__diag(ads, serv, 0, "server sent us unknown opcode %d (wanted 0=QUERY)", opcode);

        return;
    }

    qu = 0;

    /**
     * Veja se podemos encontrar a consulta relevante ou deixe qu = 0
     * caso contrário ...
     */
    if (qdcount == 1)
    {
        for (qu = viatcp ? ads->tcpw.head : ads->udpw.head; qu; qu = nqu)
        {
            nqu = qu->next;

            if (qu->id != id)
            {
                continue;
            }

            if (dglen < qu->query_dglen)
            {
                continue;
            }

            if (memcmp(qu->query_dgram + DNS_HDRSIZE, dgram + DNS_HDRSIZE, qu->query_dglen - DNS_HDRSIZE))
            {
                continue;
            }

            if (viatcp)
            {
                assert(qu->state == query_tcpw);
            } else
            {
                assert(qu->state == query_tosend);

                if (!(qu->udpsent & (1 << serv)))
                {
                    continue;
                }
            }

            break;
        }

        if (qu)
        {
            /**
             * Definitivamente, faremos algo com essa consulta agora.
             */
            if (viatcp)
            {
                LIST_UNLINK(ads->tcpw, qu);
            } else
            {
                LIST_UNLINK(ads->udpw, qu);
            }
        }
    }

    /**
     * Se vamos ignorar o plug, retornamos assim que tivermos falhado na
     * consulta (se houver) e impresso a mensagem de aviso (se houver).
     */
    switch (rcode)
    {
        case rcode_noerror:
        case rcode_nxdomain:
            break;

        case rcode_formaterror:
            adns__warn(ads, serv, qu, "server cannot understand our query (Format Error)");

            if (qu)
            {
                adns__query_fail(qu, adns_s_rcodeformaterror);
            }

            return;

        case rcode_servfail:
            if (qu)
            {
                adns__query_fail(qu, adns_s_rcodeservfail);
            } else
            {
                adns__debug(ads, serv, qu, "server failure on unidentifiable query");
            }

            return;

        case rcode_notimp:
            adns__warn(ads, serv, qu, "server claims not to implement our query");

            if (qu)
            {
                adns__query_fail(qu, adns_s_rcodenotimplemented);
            }

            return;

        case rcode_refused:
            adns__debug(ads, serv, qu, "server refused our query");

            if (qu)
            {
                adns__query_fail(qu, adns_s_rcoderefused);
            }

            return;

        default:
            adns__warn(ads, serv, qu, "server gave unknown response code %d", rcode);

            if (qu)
            {
                adns__query_fail(qu, adns_s_rcodeunknown);
            }

            return;
    }

    if (!qu)
    {
        if (!qdcount)
        {
            adns__diag(ads, serv, 0, "server sent reply without quoting our question");
        } else if (qdcount > 1)
        {
            adns__diag(ads, serv, 0, "server claimed to answer %d questions with one message", qdcount);
        } else if (ads->iflags & adns_if_debug)
        {
            adns__vbuf_init(&tempvb);
            adns__debug(ads, serv, 0, "reply not found, id %02x, query owner %s", id, adns__diag_domain(ads, serv, 0, &tempvb, dgram, dglen, DNS_HDRSIZE));
            adns__vbuf_free(&tempvb);
        }

        return;
    }

    /**
     * Nós definitivamente faremos algo com este plug e esta
     * consulta agora.
     */
    anstart = qu->query_dglen;

    /**
     * Agora, dê uma olhada na seção de respostas e veja se está completo.
     * Se tiver algum CNAMEs, nós os colocamos na resposta.
     */
    wantedrrs = 0;
    restartfrom = -1;
    cbyte = anstart;

    for (rri = 0; rri < ancount; rri++)
    {
        rrstart = cbyte;
        st = adns__findrr(qu,
            serv,
            dgram,
            dglen,
            &cbyte,
            &rrtype,
            &rrclass,
            &ttl,
            &rdlength,
            &rdstart,
            &ownermatched);

        if (st)
        {
            adns__query_fail(qu, st);

            return;
        }

        if (rrtype == -1)
        {
            /**
             * Pular lá embaixo.
             */
            goto x_truncated;
        }

        if (rrclass != DNS_CLASS_IN)
        {
            adns__diag(ads, serv, qu, "ignoring answer RR with wrong class %d (expected IN=%d)", rrclass, DNS_CLASS_IN);

            continue;
        }

        if (!ownermatched)
        {
            if (ads->iflags & adns_if_debug)
            {
                adns__debug(ads, serv, qu, "ignoring RR with an unexpected owner %s", adns__diag_domain(ads, serv, qu, &qu->vb, dgram, dglen, rrstart));
            }

            continue;
        }

        if (rrtype == adns_r_cname && (qu->answer->type & adns_rrt_typemask) != adns_r_cname)
        {
            if (qu->flags & adns_qf_cname_forbid)
            {
                adns__query_fail(qu, adns_s_prohibitedcname);

                return;
            } else if (qu->cname_dgram)
            {
                /**
                 * Ignorar o segundo e subsequente CNAME(s).
                 */
                adns__debug(ads, serv, qu, "allegedly canonical name %s is actually alias for %s", qu->answer->cname, adns__diag_domain(ads, serv, qu, &qu->vb, dgram, dglen, rdstart));
                adns__query_fail(qu,adns_s_prohibitedcname);

                return;
            } else if (wantedrrs)
            {
                /**
                 * Ignore CNAME(s) após RR(s).
                 */
                adns__debug(ads, serv, qu, "ignoring CNAME (to %s) coexisting with RR", adns__diag_domain(ads, serv, qu, &qu->vb, dgram, dglen, rdstart));
            } else
            {
                qu->cname_begin = rdstart;
                qu->cname_dglen = dglen;
                st = adns__parse_domain(ads,
                    serv,
                    qu,
                    &qu->vb,
                    qu->flags & adns_qf_quotefail_cname ? 0 : pdf_quoteok,
                    dgram,
                    dglen,
                    &rdstart,
                    rdstart + rdlength);

                if (!qu->vb.used)
                {
                    /**
                     * Pular lá embaixo.
                     */
                    goto x_truncated;
                }

                if (st)
                {
                    adns__query_fail(qu, st);

                    return;
                }

                l = strlen(qu->vb.buf) + 1;
                qu->answer->cname = adns__alloc_preserved(qu, l);

                if (!qu->answer->cname)
                {
                    adns__query_fail(qu, adns_s_nomemory);

                    return;
                }

                qu->cname_dgram = adns__alloc_mine(qu, dglen);
                memcpy(qu->cname_dgram, dgram, dglen);
                memcpy(qu->answer->cname, qu->vb.buf, l);

                cname_here = 1;
                adns__update_expires(qu, ttl, now);

                /**
                 * Se encontrarmos a seção de resposta truncada após esse ponto, reiniciamos
                 * a consulta no CNAME; se de antemão, obviamente temos que usar o TCP. Se não
                 * houver truncamento, podemos usar a resposta inteira se ela contiver as
                 * informações relevantes.
                 */
            }
        } else if (rrtype == (qu->answer->type & adns_rrt_typemask))
        {
            if (restartfrom == -1)
            {
                restartfrom = rri;
            }

            wantedrrs++;
        } else
        {
            adns__debug(ads,serv,qu,"ignoring answer RR with irrelevant type %d",rrtype);
        }
    }

    /**
     * Adiamos o tratamento de respostas truncadas aqui, caso haja um CNAME
     * que possamos usar.
     */
    if (flg_tc)
    {
        goto x_truncated;
    }

    nsstart= cbyte;

    if (!wantedrrs)
    {
        /**
         * NODATA ou NXDOMAIN ou talvez uma referência (o que poderia ser inválido).
         *
         * RFC2308: NODATA tem _either_ um SOA _or_ _no_ NS registros na seção de
         * autorização.
         */
        foundsoa = 0;
        soattl = 0;
        foundns = 0;

        for (rri = 0; rri < nscount; rri++)
        {
            rrstart = cbyte;
            st = adns__findrr(qu,
                serv,
                dgram,
                dglen,
                &cbyte,
                &rrtype,
                &rrclass,
                &ttl,
                &rdlength,
                &rdstart, 0);

            if (st)
            {
                adns__query_fail(qu, st);

                return;
            }

            if (rrtype == -1)
            {
                /**
                 * Pular lá embaixo.
                 */
                goto x_truncated;
            }

            if (rrclass != DNS_CLASS_IN)
            {
                adns__diag(ads, serv, qu, "ignoring authority RR with wrong class %d (expected IN=%d)", rrclass, DNS_CLASS_IN);

                continue;
            }

            if (rrtype == adns_r_soa_raw)
            {
                foundsoa = 1;
                soattl = ttl;

                break;
            } else if (rrtype == adns_r_ns_raw)
            {
                foundns = 1;
            }
        }

        if (rcode == rcode_nxdomain)
        {
            /**
             * Ainda queríamos procurar o SOA para encontrar o TTL.
             */
            adns__update_expires(qu,soattl,now);

            if (qu->flags & adns_qf_search && !qu->cname_dgram)
            {
                adns__search_next(ads, qu, now);
            } else
            {
                adns__query_fail(qu, adns_s_nxdomain);
            }

            return;
        }

        if (foundsoa || !foundns)
        {
            /**
             * Uma resposta NODATA.
             */
            adns__update_expires(qu, soattl, now);
            adns__query_fail(qu, adns_s_nodata);

            return;
        }

        /**
         * Nenhuma resposta relevante, nenhum SOA e pelo menos alguns NS's.
         * Parece uma referência. Apenas uma última volta desse ciclo ... se
         * encontrarmos um CNAME neste datagrama, provavelmente devemos fazer
         * nossa própria busca de CNAME, para não falhar novamente.
         */
        if (cname_here)
        {
            /**
             * Pular lá embaixo.
             */
            goto x_restartquery;
        }

        /**
         * Nesse caso, uma falha evitou uma recursão.
         */
        if (!flg_ra)
        {
            adns__diag(ads, serv, qu, "server is not willing to do recursive lookups for us");
            adns__query_fail(qu, adns_s_norecurse);
        } else
        {
            if (!flg_rd)
            {
                adns__diag(ads, serv, qu, "server thinks we didn't ask for recursive lookup");
            } else
            {
                adns__debug(ads, serv, qu, "server claims to do recursion, but gave us a referral");
            }

            adns__query_fail(qu, adns_s_invalidresponse);
        }

        return;
    }

    /**
     * Agora, temos alguns RRs que queríamos.
     */

    qu->answer->rrs.untyped = adns__alloc_interim(qu, qu->answer->rrsz * wantedrrs);

    if (!qu->answer->rrs.untyped)
    {
        adns__query_fail(qu, adns_s_nomemory);

        return;
    }

    typei = qu->typei;
    cbyte = anstart;
    rrsdata = qu->answer->rrs.bytes;

    pai.ads = qu->ads;
    pai.qu = qu;
    pai.serv = serv;
    pai.dgram = dgram;
    pai.dglen = dglen;
    pai.nsstart = nsstart;
    pai.nscount = nscount;
    pai.arcount = arcount;
    pai.now = now;

    assert(restartfrom>=0);

    for (rri = 0, nrrs = 0; rri < ancount; rri++)
    {
        st = adns__findrr(qu,
            serv,
            dgram,
            dglen,
            &cbyte,
            &rrtype,
            &rrclass,
            &ttl,
            &rdlength,
            &rdstart,
            &ownermatched);

        assert(!st);
        assert(rrtype != -1);

        if (rri < restartfrom || rrtype != (qu->answer->type & adns_rrt_typemask) || rrclass != DNS_CLASS_IN || !ownermatched)
        {
            continue;
        }

        adns__update_expires(qu, ttl, now);
        st = typei->parse(&pai,
            rdstart,
            rdstart + rdlength,
            rrsdata + nrrs * qu->answer->rrsz);

        if (st)
        {
            adns__query_fail(qu, st);

            return;
        }

        if (rdstart == -1)
        {
            /**
             * Pular lá embaixo.
             */
            goto x_truncated;
        }

        nrrs++;
    }

    assert(nrrs == wantedrrs);
    qu->answer->nrrs = nrrs;

    /**
     * Isso pode ter gerado algumas consultas de sub processo.
     */
    if (qu->children.head)
    {
        qu->state = query_childw;
        LIST_LINK_TAIL(ads->childw, qu);

        return;
    }

    adns__query_done(qu);
    return;

    x_truncated:
        if (!flg_tc)
        {
            adns__diag(ads, serv, qu, "server sent datagram which points outside itself");
            adns__query_fail(qu, adns_s_invalidresponse);

            return;
        }

        if (qu->flags & adns_qf_usevc)
        {
            adns__diag(ads, serv, qu, "server sent datagram with TC over TCP");
            adns__query_fail(qu, adns_s_invalidresponse);

            return;
        }

        qu->flags |= adns_qf_usevc;

    x_restartquery:
        if (qu->cname_dgram)
        {
            st = adns__mkquery_frdgram(qu->ads,
                &qu->vb,
                &qu->id,
                qu->cname_dgram,
                qu->cname_dglen,
                qu->cname_begin,
                qu->answer->type,
                qu->flags);

            if (st)
            {
                adns__query_fail(qu, st);

                return;
            }

            newquery = realloc(qu->query_dgram, qu->vb.used);

            if (!newquery)
            {
                adns__query_fail(qu, adns_s_nomemory);

                return;
            }

            qu->query_dgram = newquery;
            qu->query_dglen = qu->vb.used;
            memcpy(newquery, qu->vb.buf, qu->vb.used);
        }

        if (qu->state == query_tcpw)
        {
            qu->state = query_tosend;
        }

        qu->retries = 0;
        adns__reset_preserved(qu);
        adns__query_send(qu, now);
}
