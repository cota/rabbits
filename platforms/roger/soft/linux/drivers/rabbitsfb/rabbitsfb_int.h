/* PUT HEADER HERE */

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
