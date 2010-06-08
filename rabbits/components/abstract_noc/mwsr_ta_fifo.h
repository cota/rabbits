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

#ifndef MWSR_TA_FIFO_H_
#define MWSR_TA_FIFO_H_

#include <systemc.h>

template <typename ITEM>
class mwsr_ta_fifo
{
public:
    mwsr_ta_fifo (int size);
    ~mwsr_ta_fifo ();

public:
    ITEM& Read ();
    void Write (ITEM& p);
    void Reset ();

private:
    bool IsEmpty ();
    bool IsFull ();

public:
	sc_event					canRead;
	sc_event					canWrite;

private:
    ITEM						*m_data;
    int							m_size;
    int							m_idxRead;
    int							m_idxWrite;
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
