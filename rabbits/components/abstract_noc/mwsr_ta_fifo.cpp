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

#include <mwsr_ta_fifo.h>

template <typename ITEM>
mwsr_ta_fifo<ITEM>::mwsr_ta_fifo (int size)
{
    Reset ();

    m_size = size  + 1;
    m_data = new ITEM[m_size];
}

template <typename ITEM>
mwsr_ta_fifo<ITEM>::~mwsr_ta_fifo ()
{
    if (m_data)
        delete [] m_data;
}

template <typename ITEM>
ITEM& mwsr_ta_fifo<ITEM>::Read ()
{
    while (IsEmpty ())
    {
        wait(canRead);
    }

    int             pos = m_idxRead;
    m_idxRead = (m_idxRead + 1) % m_size;

    canWrite.notify();

    return m_data[pos];
}

template <typename ITEM>
void mwsr_ta_fifo<ITEM>::Write (ITEM& p)
{
    while (IsFull ())
    {
        wait (canWrite);
    }

    m_data[m_idxWrite] = p;
    m_idxWrite = (m_idxWrite + 1) % m_size;

    canRead.notify();
}

template <typename ITEM>
void mwsr_ta_fifo<ITEM>::Reset ()
{
    m_idxRead = 0;
    m_idxWrite = 0;
}

template <typename ITEM>
bool mwsr_ta_fifo<ITEM>::IsEmpty ()
{
    return m_idxRead == m_idxWrite;
}

template <typename ITEM>
bool mwsr_ta_fifo<ITEM>::IsFull ()
{
    return m_idxRead == ((m_idxWrite + 1) % m_size);
}

#include <abstract_noc.h>
template class mwsr_ta_fifo<vci_request>;
template class mwsr_ta_fifo<vci_response>;

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
