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

#ifndef _H264DBF_INT_H
#define _H264DBF_INT_H

int h264dbf_int_init      (h264dbf_device_t *dev);
int h264dbf_int_cleanup   (h264dbf_device_t *dev);

int h264dbf_int_register  (h264dbf_device_t *dev, h264dbf_ioc_irq_t *irq);
int h264dbf_int_wait      (h264dbf_device_t *dev, h264dbf_ioc_irq_t *irq);
int h264dbf_int_unregister(h264dbf_device_t *dev, h264dbf_ioc_irq_t *irq);

#endif /* _H264DBF_INT_H */

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
