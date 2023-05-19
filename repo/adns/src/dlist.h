/**
 * dlist.h - macros para lidar com listas duplamente sequênciais.
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

#ifndef ADNS_DLIST_H_INCLUDED
#define ADNS_DLIST_H_INCLUDED

    /**
     *
     */
    #define LIST_INIT(list) ((list).head = (list).tail = 0)

    /**
     *
     */
    #define LINK_INIT(link) ((link).next = (link).back = 0)

    /**
     *
     */
    #define LIST_UNLINK_PART(list, node, part) \
        do \
        { \
            if ((node)->part back) \
            { \
                (node)->part back->part next = (node)->part next; \
            } else \
            { \
                (list).head = (node)->part next; \
            } \
            if ((node)->part next) \
            { \
                (node)->part next->part back = (node)->part back; \
            } else \
            { \
                (list).tail = (node)->part back; \
            } \
        } while(0)

    /**
     *
     */
    #define LIST_LINK_TAIL_PART(list, node, part) \
        do \
        { \
            (node)->part next = 0; \
            (node)->part back = (list).tail; \
            if ((list).tail) \
            { \
                (list).tail->part next = (node); \
            } else \
            { \
                (list).head = (node); \
            } \
            (list).tail = (node); \
        } while(0)

    /**
     *
     */
    #define LIST_UNLINK(list, node) LIST_UNLINK_PART(list, node,)

    /**
     *
     */
    #define LIST_LINK_TAIL(list, node) LIST_LINK_TAIL_PART(list, node,)

#endif
