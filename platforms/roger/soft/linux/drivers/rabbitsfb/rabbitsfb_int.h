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

#ifndef _RABBITSFB_INT_H
#define _RABBITSFB_INT_H

int rabbitsfb_int_init      (rabbitsfb_device_t *dev);
int rabbitsfb_int_cleanup   (rabbitsfb_device_t *dev);

int rabbitsfb_int_register  (rabbitsfb_device_t *dev, rabbitsfb_ioc_irq_t *irq);
int rabbitsfb_int_wait      (rabbitsfb_device_t *dev, rabbitsfb_ioc_irq_t *irq);
int rabbitsfb_int_unregister(rabbitsfb_device_t *dev, rabbitsfb_ioc_irq_t *irq);

#endif /* _RABBITSFB_INT_H */

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
