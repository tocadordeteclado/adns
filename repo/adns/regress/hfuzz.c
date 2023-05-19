/**
 * nfuzz.c - parte de uma estrutura de teste complexa, não da biblioteca.
 *   rotinas usadas para fuzzing (uma espécie de reprodução)
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
 * Nós lemos de stdin:
 *  - Argumentos de linha de comando.
 *  - Fluxos de chamada de sistema.
 *  - stdin.
 */

#include <stdio.h>

#include "harness.h"


/**
 *
 */
extern int Hmain(int argc, char **argv);

/**
 *
 */
FILE *Hfopen(const char *path, const char *mode)
{
    /**
     * Não permitimos que adns abra nenhum arquivo.
     */
    errno = EPERM;

    return 0;
}

/**
 *
 */
static int t_argc;

/**
 *
 */
static char **t_argv;

/**
 *
 */
static FILE *t_stdin, *stdoutcopy;

/**
 *
 */
static int t_sys_fd;

/**
 *
 */
static int bail(const char *msg)
{
    fprintf(stderr,"adns fuzz client: %s\n", msg);
    exit(-1);
}

/**
 *
 */
static int baile(const char *msg)
{
    fprintf(stderr,"adns fuzz client: %s: %s\n", msg, strerror(errno));
    exit(-1);
}

/**
 *
 */
static void chkin(void)
{
    if (ferror(stdin))
    {
        baile("read stdin");
    }

    if (feof(stdin))
    {
        bail("eof on stdin");
    }
}

/**
 *
 */
static int getint(int max)
{
    int val;
    char c;

    chkin();

    int r = scanf("%d%c", &val, &c);
    chkin();

    if (r != 2 || c != '\n')
    {
        bail("bad input format: not integer");
    }

    if (val < 0 || val > max)
    {
        bail("bad input format: wrong value");
    }

    return val;
}

/**
 *
 */
static void getnl(void)
{
    chkin();
    int c = getchar();
    chkin();

    if (c != '\n')
    {
        bail("bad input format: expected newline");
    }
}

/**
 *
 */
int Ttestinputfd(void)
{
    return t_sys_fd;
}

/**
 *
 */
void Texit(int rv)
{
    fprintf(stdoutcopy,"rc=%d\n",rv);

    if (ferror(stdoutcopy) || fclose(stdoutcopy))
    {
        baile("flush rc msg");
    }

    Tcommonshutdown();
    exit(0);
}

/**
 *
 */
int main(int argc, char **argv)
{
    int i, l;

    if (argc != 1)
    {
        bail("usage: *_fuzz  (no arguments)");
    }

    int stdoutcopyfd = dup(1);

    if (stdoutcopyfd < 0)
    {
        baile("dup 1 again");
    }

    stdoutcopy = fdopen(stdoutcopyfd, "w");

    if (!stdoutcopy)
    {
        baile("fdopen 1 again");
    }

    t_argc = getint(50);

    if (!t_argc)
    {
        bail("too few arguments");
    }

    t_argv = calloc(t_argc + 1, sizeof(*t_argv));

    for (i = 0; i < t_argc; i++)
    {
        l = getint(1000);
        t_argv[i] = calloc(1, l + 1);

        fread(t_argv[i], 1, l, stdin);
        t_argv[i][l] = 0;
        getnl();
    }

    t_stdin = tmpfile();
    l = getint(100000);

    while (l > 0)
    {
        int c = getchar();

        if (c == EOF)
        {
            break;
        }

        fputc(c, t_stdin);

        l--;
    }

    getnl();

    if (ferror(t_stdin) || fflush(t_stdin))
    {
        baile("write/flush t_stdin");
    }

    if (fseek(stdin, 0, SEEK_CUR))
    {
        baile("seek-flush stdin");
    }

    t_sys_fd = dup(0);

    if (t_sys_fd < 0)
    {
        baile("dup stdin");
    }

    if (dup2(fileno(t_stdin), 0))
    {
        baile("dup2 t_stdin");
    }

    if (fseek(stdin, 0, SEEK_SET))
    {
        baile("rewind t_stdin");
    }

    int estatus = Hmain(t_argc, t_argv);
    Texit(estatus);
}

/**
 *
 */
void Tmallocshutdown(void)
{
}

/**
 *
 */
void *Hmalloc(size_t s)
{
    assert(s);

    return malloc(s);
}

/**
 *
 */
void *Hrealloc(void *p, size_t s)
{
    assert(s);

    return realloc(p, s);
}

/**
 *
 */
void Hfree(void *p)
{
    free(p);
}
