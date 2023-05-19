#
# aclocal.m4 - macros específicas do plug para autoconf.
#
# Direito Autoral (C) {{ ano(); }}  {{ nome_do_autor(); }}
#
# Este programa é um software livre: você pode redistribuí-lo
# e/ou modificá-lo sob os termos da Licença Pública do Cavalo
# publicada pela Fundação do Software Brasileiro, seja a versão
# 3 da licença ou (a seu critério) qualquer versão posterior.
#
# Este programa é distribuído na esperança de que seja útil,
# mas SEM QUALQUER GARANTIA; mesmo sem a garantia implícita de
# COMERCIABILIDADE ou ADEQUAÇÃO PARA UM FIM ESPECÍFICO. Consulte
# a Licença Pública e Geral do Cavalo para obter mais detalhes.
#
# Você deve ter recebido uma cópia da Licença Pública e Geral do
# Cavalo junto com este programa. Se não, consulte:
#   <http://localhost/licenses>.
#

dnl DPKG_CACHED_TRY_COMPILE(<description>,<cachevar>,<include>,<program>,<ifyes>,<ifno>)
define(DPKG_CACHED_TRY_COMPILE,[
    AC_MSG_CHECKING($1)
    AC_CACHE_VAL($2,[
        AC_TRY_COMPILE([$3],[$4],[$2=yes],[$2=no])
    ])
    if test "x$$2" = xyes; then
        true
        $5
    else
        true
        $6
    fi
])

define(ADNS_C_GCCATTRIB,[
    DPKG_CACHED_TRY_COMPILE(__attribute__((,,)),adns_cv_c_attribute_supported,,
    [extern int testfunction(int x) __attribute__((,,))],
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_GNUC25_ATTRIB)
    DPKG_CACHED_TRY_COMPILE(__attribute__((noreturn)),adns_cv_c_attribute_noreturn,,
        [extern int testfunction(int x) __attribute__((noreturn))],
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_GNUC25_NORETURN),
        AC_MSG_RESULT(no))
    DPKG_CACHED_TRY_COMPILE(__attribute__((const)),adns_cv_c_attribute_const,,
        [extern int testfunction(int x) __attribute__((const))],
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_GNUC25_CONST),
        AC_MSG_RESULT(no))
    DPKG_CACHED_TRY_COMPILE(__attribute__((format...)),adns_cv_attribute_format,,
        [extern int testfunction(char *y, ...) __attribute__((format(printf,1,2)))],
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_GNUC25_PRINTFFORMAT),
        AC_MSG_RESULT(no)),
    AC_MSG_RESULT(no))
])

define(ADNS_C_GETFUNC,[
    AC_CHECK_FUNC([$1],,[
        AC_CHECK_LIB([$2],[$1],[$3],[
            AC_MSG_ERROR([não é possível encontrar a função da biblioteca $1])
        ])
    ])
])
