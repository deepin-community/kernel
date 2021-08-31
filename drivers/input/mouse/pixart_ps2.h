#ifndef _PIXART_PS2_H
#define _PIXART_PS2_H

#include "psmouse.h"


#ifdef CONFIG_MOUSE_PS2_PIXART
int pixart_detect(struct psmouse *psmouse, bool set_properties);
int pixart_init(struct psmouse *psmouse);
#else
inline int pixart_detect(struct psmouse *psmouse, bool set_properties)
{
	return -ENOSYS;
}
inline int pixart_init(struct psmouse *psmouse)
{
	return -ENOSYS;
}
#endif /* CONFIG_MOUSE_PS2_pixart */

#endif  /* _PIXART_PS2_H */
