/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <signal.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>

#include "fcitx/fcitx.h"
#include "fcitx/instance-internal.h"
#include "fcitx/ime-internal.h"
#include "fcitx/configfile.h"
#include "fcitx/instance.h"
#include "fcitx-utils/log.h"
#include "fcitx-config/xdg.h"
#include "errorhandler.h"

#ifndef SIGUNUSED
#define SIGUNUSED 32
#endif

extern FcitxInstance* instance;
extern int selfpipe[2];

void SetMyExceptionHandler(void)
{
    int             signo;

    for (signo = SIGHUP; signo < SIGUNUSED; signo++) {
        if (signo != SIGALRM
            && signo != SIGPIPE
            && signo != SIGUSR2
            && signo != SIGWINCH
        )
            signal(signo, OnException);
        else
            signal(signo, SIG_IGN);
    }
}

void OnException(int signo)
{
    if (signo == SIGCHLD)
        return;

    FcitxLog(INFO, "FCITX -- Get Signal No.: %d", signo);

    void *array[20];

    size_t size;
    char **strings = NULL;
    size_t i;

    size = backtrace(array, 20);
    strings = backtrace_symbols(array, size);

    if (strings) {
        FILE *fp = NULL;

        if (signo == SIGSEGV || signo == SIGABRT || signo == SIGKILL)
            fp = FcitxXDGGetFileWithPrefix("log", "crash.log", "w", NULL);

        printf("Obtained %zd stack frames.\n", size);

        if (fp) {
            fprintf(fp, "FCITX -- Get Signal No.: %d\n", signo);
            fprintf(fp, "Obtained %zd stack frames.\n", size);
        }

        for (i = 0; i < size; i++) {
            printf("%s\n", strings[i]);

            if (fp)
                fprintf(fp, "%s\n", strings[i]);
        }

        if (fp)
            fclose(fp);

        free(strings);
    }

    switch (signo) {
    case SIGKILL:
        break;
    case SIGABRT:
    case SIGSEGV:
        exit(1);
        break;

    default:
        {
            if (!instance->initialized) {
                exit(1);
                break;
            }

            uint8_t sig = 0;
            if (signo < 0xff)
                sig = (uint8_t)(signo & 0xff);
            write(selfpipe[1], &sig, 1);
            signal(signo, OnException);
        }
        break;
    }
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
