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

#include "rabbitsfb_types.h"
#include "rabbitsfb_ioctl.h"
#include "rabbitsfb_regs.h"
#include "qemu_wrapper_cts.h"

#if 0
#define DEBUG
#endif

#include "debug.h"

#define RABBITSFB_IRQ_NO   34

#define STATUS_IRQ_INIT      0x1
#define STATUS_IRQ_QUEUED    0x2
#define STATUS_IRQ_OVERFLOW  0x3

static irqreturn_t
rabbitsfb_int_service(int irq, void *d)
{
    rabbitsfb_device_t *dev;
    rabbitsfb_int_t    *irq_desc;
    unsigned long       status;
    int                 i;

    dev = (rabbitsfb_device_t *)d;
    irq_desc = dev->irqs;

    /* Get status */
    status = readl(dev->base_addr + RABBITSFB_IRQ_STAT);
    if(!(status & 1)){
        MSG("IRQ called - not for me, irq status = 0x%X\n", (unsigned int)status);
        return IRQ_NONE;
    }
    DMSG("IRQ called - for me, irq status = 0x%X\n", (unsigned int)status);
    status &= 1;

    /* Clear interrupt */
    writel(0x1, (dev->base_addr + RABBITSFB_IRQCLEAR));

    for(i = 0; i < dev->nb_irq_en; i++, irq_desc++){
        unsigned long  stat = status & irq_desc->type;

        if(stat){
            if(stat & irq_desc->pending){
                irq_desc->status = STATUS_IRQ_OVERFLOW;
            }else{
                irq_desc->status   = STATUS_IRQ_QUEUED;
                irq_desc->pending |= stat;
            }

            /* Wake the interrupted task */
            wake_up_interruptible(&irq_desc->wait_queue);
        }

        status &= ~stat;
        if(!status)
            break;
    }

    DMSG("(----) Interrupt Handled (----)\n");
    return IRQ_HANDLED;
}

static int
rabbitsfb_int_enable(rabbitsfb_device_t *dev)
{
    rabbitsfb_int_t *irq_desc = dev->irqs;
    u8               irq = RABBITSFB_IRQ_NO;
    unsigned long    qemu_intr_enabled, fb_old_status;
    int              i;

    if(!dev->irq_en){
        /* Clear pending interrupts */
        writel(1, dev->base_addr + RABBITSFB_IRQCLEAR);

        /* Initialize irq structure */
        for(i = 0; i < RABBITSFB_MAXIRQ; i++, irq_desc++){
            irq_desc->type    = 0;
            irq_desc->status  = 0;
            irq_desc->pending = 0;
            init_waitqueue_head(&irq_desc->wait_queue);
        }

        if(request_irq(irq,                   /* Irq Number                    */
                       rabbitsfb_int_service, /* ISR                           */
                       IRQF_SHARED,           /* Flags -- Shared interrupt     */
                       "rabbitsfb",           /* Interrupt owner name          */
                       (void*)dev)){          /* Reference pointer             */
            EMSG("unable to request IRQ service\n");
            return -EINVAL;
        }

        //enable interrupt in qemu  /* TO BE REMOVED FROME HERE */
        qemu_intr_enabled = readl(__io_address(QEMU_ADDR_BASE) + GET_SYSTEMC_INT_ENABLE);
        DMSG ("ramdac, qemu interrupts enabled before enabling ramdac = %lu\n", qemu_intr_enabled);
        qemu_intr_enabled |= QEMU_INT_RAMDAC_MASK;
        writel (qemu_intr_enabled, __io_address(QEMU_ADDR_BASE) + SET_SYSTEMC_INT_ENABLE);

        //enable interrupt in ramdac
        fb_old_status  = readl (dev->base_addr + RABBITSFB_STAT);
        fb_old_status |= RABBITSFB_CTRL_IRQEN;
        writel(fb_old_status, dev->base_addr + RABBITSFB_CTRL);

        dev->irq_en = 1;
    }
    dev->nb_irq_en++;

    return 0;
}

void rabbitsfb_disable_device_irq(rabbitsfb_device_t *dev)
{
    unsigned long qemu_intr_enabled;
    unsigned long fb_old_status;

     //disable interrupt in ramdac & shut off the display
    fb_old_status = readl (dev->base_addr + RABBITSFB_STAT);
    fb_old_status &= ~RABBITSFB_CTRL_IRQEN;
    writel(fb_old_status, dev->base_addr + RABBITSFB_CTRL);

    //disable interrupt in qemu
    qemu_intr_enabled = readl(__io_address(QEMU_ADDR_BASE) + GET_SYSTEMC_INT_ENABLE);
    DMSG ("ramdac, qemu interrupts enabled before disabling ramdac = %lu\n", qemu_intr_enabled);
    qemu_intr_enabled &= ~QEMU_INT_RAMDAC_MASK;
    writel (qemu_intr_enabled, __io_address(QEMU_ADDR_BASE) + SET_SYSTEMC_INT_ENABLE);
}

static int
rabbitsfb_int_disable(rabbitsfb_device_t *dev)
{
    u8 irq = RABBITSFB_IRQ_NO;

    dev->nb_irq_en--;
    if(!dev->nb_irq_en){

        rabbitsfb_disable_device_irq(dev);
        free_irq (irq, (void *) dev);
        dev->irq_en = 0;
    }

    return 0;
}

int
rabbitsfb_int_init(rabbitsfb_device_t *dev)
{
    spin_lock_init(&dev->irq_lock);

    return 0;
}

int
rabbitsfb_int_cleanup(rabbitsfb_device_t *dev)
{
    int              i;
    rabbitsfb_int_t *irq_desc = dev->irqs;
    unsigned long    irq_flags;
    int              nirq = dev->nb_irq_en;

    /* Critical section : begin */
    spin_lock_irqsave(&dev->irq_lock, irq_flags);

    for(i = 0; i < nirq; i++, irq_desc++){
        /* Update IRQ descriptor */
        irq_desc->type = 0;

        /* Disabling Irq */
        rabbitsfb_int_disable(dev);
    }

    /* Critical section : end */
    spin_unlock_irqrestore(&dev->irq_lock, irq_flags);

    return 0;
}

int
rabbitsfb_int_register(rabbitsfb_device_t *dev, rabbitsfb_ioc_irq_t *irq)
{
    int              ret = 0;
    unsigned long    irq_flags;
    rabbitsfb_int_t *irq_desc = NULL;
    uint32_t         irq_id;
    uint32_t         irq_type;

    if( (irq->id < 0) || (irq->id > RABBITSFB_MAXIRQ) ){
        EMSG("Invalid interrupt ID %lu\n", (unsigned long)irq->id);
        return -EINVAL;
    }

    irq_id   = irq->id;
    irq_type = irq->type;
    irq_desc = &dev->irqs[irq_id];

    /* Critical section : begin */
    spin_lock_irqsave(&dev->irq_lock, irq_flags);

    /* Enabling Irq */
    rabbitsfb_int_enable(dev);

    /* Update IRQ descriptor */
    irq_desc->type   = irq_type;
    irq_desc->status = STATUS_IRQ_INIT;

    /* Update IRQ enable register  */
    /* writel (type, ENABLE_REG); */

    /* Critical section : end */
    spin_unlock_irqrestore(&dev->irq_lock, irq_flags);

    return ret;
}

int
rabbitsfb_int_wait(rabbitsfb_device_t *dev, rabbitsfb_ioc_irq_t *irq)
{
    rabbitsfb_int_t *irq_desc = NULL;
    unsigned long    irq_flags;
    uint32_t         irq_type;
    uint32_t         irq_id;
    uint32_t         wait_time;
    int              ret = 0;

    if( (irq->id < 0) || (irq->id > RABBITSFB_MAXIRQ) ){
        EMSG("Invalid interrupt ID %lu\n", (unsigned long)irq->id);
        return -EINVAL;
    }

    irq_id    = irq->id;
    irq_type  = irq->type;
    irq_desc  = &dev->irqs[irq_id];
    wait_time = irq->timeout;

    #ifdef PARANOIA_CHECK
    if(irq_type != irq_desc->type){
        EMSG("Non corresponding IRQ descriptor !!!\n");
        return -EINVAL;
    }
    #endif /* PARANOIA_CHECK */

    DMSG("Wait for interrupt %d\n", irq_id);

    /* if we do not want to wait OR */
    /* if IRQ already occured       */
    /* just skip the wait thing     */
    if( wait_time && !(irq_desc->pending & irq_type) ){

        DEFINE_WAIT(wait);

        prepare_to_wait(&irq_desc->wait_queue, &wait, TASK_INTERRUPTIBLE);
        /* if (wait_time == -1) /\* wait for ever (wait_time == -1) *\/ */
        schedule();
        /* else { */
        /*  schedule_timeout ((unsigned long) wait_time * HZ); */
        /* } */
        finish_wait(&irq_desc->wait_queue, &wait);

    }

    /* Critical section : begin */
    spin_lock_irqsave(&dev->irq_lock, irq_flags);

    /* Update IRQ descriptor */
    /* irq_desc->type   = irq_type; */
    irq->type         = irq_desc->pending;
    irq_desc->pending = 0;

    /* Critical section : end */
    spin_unlock_irqrestore(&dev->irq_lock, irq_flags);

    return ret;
}

int
rabbitsfb_int_unregister(rabbitsfb_device_t *dev, rabbitsfb_ioc_irq_t *irq)
{
    rabbitsfb_int_t *irq_desc = NULL;
    unsigned long    irq_flags;
    uint32_t         irq_type;
    uint32_t         irq_id;
    int              ret = 0;

    if( (irq->id < 0) || (irq->id > RABBITSFB_MAXIRQ) ){
        EMSG("Invalid interrupt ID %lu\n", (unsigned long)irq->id);
        return -EINVAL;
    }

    irq_id   = irq->id;
    irq_type = irq->type;
    irq_desc = &dev->irqs[irq_id];

    #ifdef PARANOIA_CHECK
    if(irq_type != irq_desc->type){
        EMSG("Non corresponding IRQ descriptor !!!\n");
        return -EINVAL;
    }
    #endif /* PARANOIA_CHECK */

    /* Update IRQ enable register  */
    /* writel (type, ENABLE_REG); */

    /* Critical section : begin */
    spin_lock_irqsave(&dev->irq_lock, irq_flags);

    /* Update IRQ descriptor */
    irq_desc->type = 0;

    /* Critical section : end */
    spin_unlock_irqrestore(&dev->irq_lock, irq_flags);

    /* Disabling Irq */
    rabbitsfb_int_disable(dev);

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
