/*
 *  Copyright (c) 2010 TIMA Laboratory
 *
 *  This file is part of Rabbits.
 *
 *  Rabbits is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Rabbits is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Rabbits.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QEMU_WRAPPER_REQUEST_
#define _QEMU_WRAPPER_REQUEST_

#include <systemc.h>

class qemu_wrapper_request
{
public:
    qemu_wrapper_request (unsigned id);

public:
    unsigned char               tid;
    unsigned char               ropcode;
    unsigned char               bDone;
    unsigned char               bWrite;
    sc_event                    evDone;
    unsigned long               rcv_data;

    qemu_wrapper_request        *m_next;
};

class qemu_wrapper_requests
{
public:
    qemu_wrapper_requests (int count);
    ~qemu_wrapper_requests ();

    qemu_wrapper_request* GetNewRequest (int bWaitEmpty);
    qemu_wrapper_request* GetRequestByTid (unsigned char tid);
    void FreeRequest (qemu_wrapper_request *rq);
    void WaitWBEmpty ();

private:
    qemu_wrapper_request        *m_headFree;
    qemu_wrapper_request        *m_headBusy;
    sc_event                    m_evEmpty;
};

#endif

/*
 * Vim standard variables
 * vim:set ts=4 expandtab tw=80 cindent syntax=c:
 *
 * Emacs standard variables
 * Local Variables:
 * mode: c
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
