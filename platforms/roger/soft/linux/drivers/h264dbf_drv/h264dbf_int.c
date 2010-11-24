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

#include <linux/interrupt.h>
#include <linux/io.h>
#include <mach/hardware.h>

#include "h264dbf_types.h"
#include "h264dbf_ioctl.h"
#include "h264dbf_regs.h"
#include "qemu_wrapper_cts.h"

#define DEBUG

#include "debug.h"

#define H264DBF_IRQ_NO   35

#define STATUS_IRQ_INIT      0x1
#define STATUS_IRQ_QUEUED    0x2
#define STATUS_IRQ_OVERFLOW  0x3

static irqreturn_t
h264dbf_int_service (int irq, void *d)
{
    h264dbf_device_t           *dev;
    h264dbf_int_t              *irq_desc;
    unsigned int               status;
    int                         i;

/*    status = readl (__io_address(QEMU_ADDR_BASE) + GET_SYSTEMC_INT_STATUS);*/
/*    if (!(status & QEMU_INT_DBF_MASK))*/
/*    {*/
/*        MSG ("dbf:IRQ called - not for me, irq status = 0x%X\n", status);*/
/*        return IRQ_NONE;*/
/*    }*/

    dev = (h264dbf_device_t *) d;

    // get status from dbf
    status = (unsigned int) readb (dev->base_addr + H264DBF_INT_STATUS);
    DMSG ("dbf:IRQ called dbf status = 0x%X\n", (unsigned int) status);
    if (status <= 0x80)
    {
        DMSG ("dbf:IRQ called - for me, but dbf status = 0x%X\n", (unsigned int) status);
        //writel (1, __io_address(QEMU_ADDR_BASE) + SYSTEMC_SHUTDOWN);
        return IRQ_HANDLED;
    }
    status &= 0x7F;

    // clear interrupt
    writeb (status, dev->base_addr + H264DBF_INT_ACK);

    irq_desc = dev->irqs;
    for (i = 0; i < dev->nb_irq_en; i++, irq_desc++)
    {
        unsigned int            stat = status & irq_desc->type;

        if (stat)
        {
            if (stat & irq_desc->pending)
            {
                irq_desc->status = STATUS_IRQ_OVERFLOW;
            }
            else
            {
                irq_desc->status   = STATUS_IRQ_QUEUED;
                irq_desc->pending |= stat;
            }

            // wake the interrupted task
            wake_up_interruptible (&irq_desc->wait_queue);
        }

        status &= ~stat;
        if (! status)
            break;
    }

    DMSG ("(----) Interrupt Handled (----)\n");

    return IRQ_HANDLED;
}

static int
h264dbf_int_enable (h264dbf_device_t *dev)
{
    int                         i = 0;
    h264dbf_int_t              *irq_desc = dev->irqs;
    u8                          irq = H264DBF_IRQ_NO;
    unsigned long               qemu_intr_enabled;

    if (!dev->irq_en)
    {
        // clear pending interrupts
        writeb (0x7F, dev->base_addr + H264DBF_INT_ACK);

        // initialize irq structure
        for (i = 0; i < H264DBF_MAXIRQ; i++, irq_desc++)
        {
            irq_desc->type    = 0;
            irq_desc->status  = 0;
            irq_desc->pending = 0;
            init_waitqueue_head (&irq_desc->wait_queue);
        }

        if (request_irq (irq,          // irq Number
                         h264dbf_int_service,  // ISR
                         IRQF_SHARED,           // flags -- Shared interrupt
                         "h264dbf",            // interrupt owner name
                         (void*) dev))          // reference pointer
        {
            EMSG("unable to request IRQ service\n");
            return -EINVAL;
        }

        dev->irq_en = 1;

        //enable interrupt in qemu
        qemu_intr_enabled = readl (__io_address(QEMU_ADDR_BASE) + GET_SYSTEMC_INT_ENABLE);
        DMSG ("dbf, qemu interrupts enabled before enabling dbf = %lu\n", qemu_intr_enabled);
        qemu_intr_enabled |= QEMU_INT_DBF_MASK;
        writel (qemu_intr_enabled, __io_address(QEMU_ADDR_BASE) + SET_SYSTEMC_INT_ENABLE);
    }
    dev->nb_irq_en++;

    return 0;
}

void h264dbf_disable_device_irq (h264dbf_device_t *dev)
{
    unsigned long               qemu_intr_enabled;

    //disable all interrupts in dbf
    writel (0, dev->base_addr + H264DBF_INT_ENABLE);

    //disable interrupt in qemu
    qemu_intr_enabled = readl (__io_address(QEMU_ADDR_BASE) + GET_SYSTEMC_INT_ENABLE);
    DMSG ("dbf, qemu interrupts enabled before disabling dbf = %lu\n", qemu_intr_enabled);
    qemu_intr_enabled &= ~QEMU_INT_DBF_MASK;
    writel (qemu_intr_enabled, __io_address(QEMU_ADDR_BASE) + SET_SYSTEMC_INT_ENABLE);
}

static int
h264dbf_int_disable (h264dbf_device_t *dev)
{
    u8                          irq = H264DBF_IRQ_NO;

    dev->nb_irq_en--;
    if (!dev->nb_irq_en)
    {
        h264dbf_disable_device_irq (dev);

        free_irq (irq, (void *) dev);
        dev->irq_en = 0;
    }

    return 0;
}

int
h264dbf_int_init (h264dbf_device_t *dev)
{
    spin_lock_init (&dev->irq_lock);

    return 0;
}

int
h264dbf_int_cleanup (h264dbf_device_t *dev)
{
    int                         i;
    h264dbf_int_t              *irq_desc = dev->irqs;
    unsigned long               irq_flags;
    int                         nirq = dev->nb_irq_en;

    spin_lock_irqsave (&dev->irq_lock, irq_flags);
    for (i = 0; i < nirq; i++, irq_desc++)
    {
        irq_desc->type = 0;

        // Disabling irq
        h264dbf_int_disable (dev);
    }

    spin_unlock_irqrestore (&dev->irq_lock, irq_flags);

    return 0;
}

int
h264dbf_int_register (h264dbf_device_t *dev, h264dbf_ioc_irq_t *irq)
{
    int                             ret = 0;
    unsigned long                   irq_flags, dbf_intr_enabled;
    h264dbf_int_t                  *irq_desc = NULL;
    uint32_t                        irq_id;
    uint32_t                        irq_type;

    if (irq->id < 0 || irq->id > H264DBF_MAXIRQ)
    {
        EMSG ("Invalid interrupt ID\n");
        return -EINVAL;
    }

    irq_id   = irq->id;
    irq_type = irq->type;
    irq_desc = &dev->irqs[irq_id];

    spin_lock_irqsave (&dev->irq_lock, irq_flags);

    // enabling irq
    h264dbf_int_enable (dev);

    irq_desc->type   = irq_type;
    irq_desc->status = STATUS_IRQ_INIT;

    //enable interrupt in dbf
    dbf_intr_enabled = readl (dev->base_addr + H264DBF_INT_ENABLE);
    MSG ("dbf, dbf interrupts enabled before enabling dbf = %lu\n", dbf_intr_enabled);
    dbf_intr_enabled |= irq_desc->type;
    writel (dbf_intr_enabled, dev->base_addr + H264DBF_INT_ENABLE);

    spin_unlock_irqrestore (&dev->irq_lock, irq_flags);

    return ret;
}

int
h264dbf_int_wait (h264dbf_device_t *dev, h264dbf_ioc_irq_t *irq)
{
    int                             ret = 0;
    unsigned long                   irq_flags;
    uint32_t                        irq_type;
    uint32_t                        irq_id;
    uint32_t                        wait_time;
    h264dbf_int_t                  *irq_desc = NULL;

    if (irq->id < 0 || irq->id > H264DBF_MAXIRQ)
    {
        EMSG ("Invalid interrupt ID\n");
        return -EINVAL;
    }

    irq_id    = irq->id;
    irq_type  = irq->type;
    irq_desc  = &dev->irqs[irq_id];
    wait_time = irq->timeout;

#ifdef PARANOIA_CHECK
    if (irq_type != irq_desc->type)
    {
        EMSG ("Non corresponding IRQ descriptor !!!\n");
        return -EINVAL;
    }
#endif

    DMSG ("Wait for interrupt %d\n", irq_id);

    // if we do not want to wait OR if irq already occured, just skip the wait thing
    if (wait_time && !(irq_desc->pending & irq_type))
    {
        DEFINE_WAIT (wait);

        prepare_to_wait (&irq_desc->wait_queue, &wait, TASK_INTERRUPTIBLE);
//        if(wait_time == -1) // wait for ever (wait_time == -1)
        schedule ();
//        else
//            schedule_timeout ((unsigned long) wait_time * HZ);
        finish_wait (&irq_desc->wait_queue, &wait);
    }

    spin_lock_irqsave (&dev->irq_lock, irq_flags);

    irq->type         = irq_desc->pending;
    irq_desc->pending = 0;

    spin_unlock_irqrestore (&dev->irq_lock, irq_flags);

    return ret;
}

int
h264dbf_int_unregister (h264dbf_device_t *dev, h264dbf_ioc_irq_t *irq)
{
    int                         ret = 0;
    unsigned long               irq_flags, dbf_intr_enabled;
    uint32_t                    irq_type;
    uint32_t                    irq_id;
    h264dbf_int_t              *irq_desc = NULL;

    if (irq->id < 0 || irq->id > H264DBF_MAXIRQ)
    {
        EMSG ("Invalid interrupt ID\n");
        return -EINVAL;
    }

    irq_id   = irq->id;
    irq_type = irq->type;
    irq_desc = &dev->irqs[irq_id];

#ifdef PARANOIA_CHECK
    if(irq_type != irq_desc->type)
    {
        EMSG ("Non corresponding IRQ descriptor !!!\n");
        return -EINVAL;
    }
#endif

    //disable interrupt in dbf
    dbf_intr_enabled = readl (dev->base_addr + H264DBF_INT_ENABLE);
    MSG ("dbf, dbf interrupts enabled before disabling dbf = %lu\n", dbf_intr_enabled);
    dbf_intr_enabled &= ~irq_desc->type;
    writel (dbf_intr_enabled, dev->base_addr + H264DBF_INT_ENABLE);

    spin_lock_irqsave (&dev->irq_lock, irq_flags);

    irq_desc->type = 0;

    spin_unlock_irqrestore (&dev->irq_lock, irq_flags);

    // disabling irq
    h264dbf_int_disable (dev);

    return ret;
}

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
