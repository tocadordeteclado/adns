/**
 * clients.h - declarações e definições úteis para programas clientes adns.
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

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

    /**
     *
     */
    #define ADNS_VERSION_STRING "1.6.0"

    /**
     *
     */
    #define COPYRIGHT_MESSAGE \
        "Direito Autoral (C) 2023  Jorge Luiz<tjxjorge@gmail.com>\n\n" \
        "Este programa vem com ABSOLUTAMENTE NENHUMA GARANTIA; para\n" \
        "detalhes, digite `show w'. Este é um software gratuito e \n" \
        "você pode redistribuí-lo sob certas condições; \n"

    /**
     *
     */
    #define VERSION_MESSAGE(program) \
        program " (Cavalo adns) " ADNS_VERSION_STRING "\n\n" COPYRIGHT_MESSAGE

    /**
     *
     */
    #define VERSION_PRINT_QUIT(program) \
        if (fputs(VERSION_MESSAGE(program), stdout) == EOF || fclose(stdout)) \
        { \
            perror(program ": escrever mensagem de versão"); \
            quitnow(-1); \
        } \
        quitnow(0);

    /**
     *
     */
    void quitnow(int rc) NONRETURNING;

#endif
