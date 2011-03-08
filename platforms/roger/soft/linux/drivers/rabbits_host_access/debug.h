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

#ifndef _DEBUG_HADRV_H_
#define _DEBUG_HADRV_H_

#define DBG_HDR "rabbitsha:"

#define EMSG(fmt, args...)  printk( KERN_INFO DBG_HDR" " fmt, ## args)

#if defined(DEBUG)
#   define MSG(fmt, args...) printk( KERN_INFO DBG_HDR" " fmt, ## args)
#   if defined(HDEBUG)
#      define DMSG(fmt, args...) printk( KERN_INFO DBG_HDR"dbg: " fmt, ## args)
#   else
#      define DMSG(fmt, args...) do { } while(0)  /* not debugging: nothing */
#   endif
#else
#   define MSG(fmt, args...)  do { } while(0)
#   define DMSG(fmt, args...)  do { } while(0)
#endif


#endif /* _DEBUG_H */

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
 
