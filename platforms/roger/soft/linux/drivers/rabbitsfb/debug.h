/* PUT HEADER HERE */

#ifndef _DEBUG_H
#define _DEBUG_H

#define DBG_HDR "rabbitsfb:"

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

