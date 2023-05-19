/**
 * acconfig.h
 *     - arquivo de entrada para autoheader/autoconf/configure: material
 *       extra para config.h
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


/**
 * Defina se as funções inline a la do compilador estão disponíveis.
 */
#undef HAVE_INLINE

/**
 * Defina se os atributos de função a la do compilador mais antigo e
 * novo estão disponíveis.
 */
#undef HAVE_GNUC25_ATTRIB

/**
 * Defina se as funções constantes a la do compilador mais antigo e
 * novo estão disponíveis.
 */
#undef HAVE_GNUC25_CONST

/**
 * Defina se funções sem retorno a la do compilador mais antigo e
 * novo estão disponíveis.
 */
#undef HAVE_GNUC25_NORETURN

/**
 * Defina se as listas de argumentos em formato printf à la do
 * compilador estão disponíveis.
 */
#undef HAVE_GNUC25_PRINTFFORMAT

/**
 * Defina se queremos incluir rpc/types.h. possivelmente uma
 * versão desse sistema coloca INADDR_LOOPBACK lá.
 */
#undef HAVEUSE_RPCTYPES_H

@BOTTOM@

/**
 * Use as definições:
 */

#ifndef HAVE_INLINE
    /**
     *
     */
    #define inline
#endif

#ifdef HAVE_POLL
    /**
     *
     */
    #include <sys/poll.h>
#else
    /**
     *
     */
    struct pollfd
    {
        /**
         *
         */
        int fd;

        /**
         *
         */
        short events;

        /**
         *
         */
        short revents;
    };

    /**
     *
     */
    #define POLLIN 1

    /**
     *
     */
    #define POLLPRI 2

    /**
     *
     */
    #define POLLOUT 4
#endif

/**
 * Atributos ".c".
 */
#ifndef FUNCATTR
    /**
     *
     */
    #ifdef HAVE_GNUC25_ATTRIB
        /**
         *
         */
        #define FUNCATTR(x) __attribute__(x)
    #else
        /**
         *
         */
        #define FUNCATTR(x)
    #endif
#endif

/**
 * Formatos de printf ".c", ou null.
 */
#ifndef ATTRPRINTF
    /**
     *
     */
    #ifdef HAVE_GNUC25_PRINTFFORMAT
        /**
         *
         */
        #define ATTRPRINTF(si,tc) format(printf, si, tc)
    #else
        /**
         *
         */
        #define ATTRPRINTF(si,tc)
    #endif
#endif

/**
 *
 */
#ifndef PRINTFFORMAT
    /**
     *
     */
    #define PRINTFFORMAT(si, tc) FUNCATTR((ATTRPRINTF(si, tc)))
#endif

/**
 * funções sem retorno ".c", ou null.
 */
#ifndef ATTRNORETURN
    /**
     *
     */
    #ifdef HAVE_GNUC25_NORETURN
        /**
         *
         */
        #define ATTRNORETURN noreturn
    #else
        /**
         *
         */
        #define ATTRNORETURN
    #endif
#endif

/**
 *
 */
#ifndef NONRETURNING
    /**
     *
     */
    #define NONRETURNING FUNCATTR((ATTRNORETURN))
#endif

/**
 * Combinação de ambos os itens acima.
 */
#ifndef NONRETURNPRINTFFORMAT
    /**
     *
     */
    #define NONRETURNPRINTFFORMAT(si, tc) FUNCATTR((ATTRPRINTF(si, tc),ATTRNORETURN))
#endif

/**
 * funções ".c" constantes, ou null.
 */
#ifndef ATTRCONST
    /**
     *
     */
    #ifdef HAVE_GNUC25_CONST
        /**
         *
         */
        #define ATTRCONST const
    #else
        /**
         *
         */
        #define ATTRCONST
    #endif
#endif

/**
 *
 */
#ifndef CONSTANT
    /**
     *
     */
    #define CONSTANT FUNCATTR((ATTRCONST))
#endif

/**
 *
 */
#ifdef HAVEUSE_RPCTYPES_H
    /**
     *
     */
    #include <rpc/types.h>
#endif
