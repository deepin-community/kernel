// SPDX-License-Identifier: GPL-2.0
#include <linux/termios_internal.h>

/*
 * Translate a "termio" structure into a "termios". Ugh.
 */

int user_termio_to_kernel_termios(struct ktermios *a_termios, struct termio __user *u_termio)
{
	struct ktermios *k_termios = (a_termios);
	struct termio k_termio;
	int canon, ret;

	ret = copy_from_user(&k_termio, u_termio, sizeof(k_termio));
	if (!ret) {
		/* Overwrite only the low bits.  */
		*(unsigned short *)&k_termios->c_iflag = k_termio.c_iflag;
		*(unsigned short *)&k_termios->c_oflag = k_termio.c_oflag;
		*(unsigned short *)&k_termios->c_cflag = k_termio.c_cflag;
		*(unsigned short *)&k_termios->c_lflag = k_termio.c_lflag;
		canon = k_termio.c_lflag & ICANON;

		k_termios->c_cc[VINTR]  = k_termio.c_cc[_VINTR];
		k_termios->c_cc[VQUIT]  = k_termio.c_cc[_VQUIT];
		k_termios->c_cc[VERASE] = k_termio.c_cc[_VERASE];
		k_termios->c_cc[VKILL]  = k_termio.c_cc[_VKILL];
		k_termios->c_cc[VEOL2]  = k_termio.c_cc[_VEOL2];
		k_termios->c_cc[VSWTC]  = k_termio.c_cc[_VSWTC];
		k_termios->c_cc[canon ? VEOF : VMIN]  = k_termio.c_cc[_VEOF];
		k_termios->c_cc[canon ? VEOL : VTIME] = k_termio.c_cc[_VEOL];
	}
	return ret;
}

/*
 * Translate a "termios" structure into a "termio". Ugh.
 *
 * Note the "fun" _VMIN overloading.
 */
int kernel_termios_to_user_termio(struct termio __user *u_termio, struct ktermios *a_termios)
{
	struct ktermios *k_termios = (a_termios);
	struct termio k_termio;
	int canon;

	k_termio.c_iflag = k_termios->c_iflag;
	k_termio.c_oflag = k_termios->c_oflag;
	k_termio.c_cflag = k_termios->c_cflag;
	canon = (k_termio.c_lflag = k_termios->c_lflag) & ICANON;

	k_termio.c_line = k_termios->c_line;
	k_termio.c_cc[_VINTR]  = k_termios->c_cc[VINTR];
	k_termio.c_cc[_VQUIT]  = k_termios->c_cc[VQUIT];
	k_termio.c_cc[_VERASE] = k_termios->c_cc[VERASE];
	k_termio.c_cc[_VKILL]  = k_termios->c_cc[VKILL];
	k_termio.c_cc[_VEOF]   = k_termios->c_cc[canon ? VEOF : VMIN];
	k_termio.c_cc[_VEOL]   = k_termios->c_cc[canon ? VEOL : VTIME];
	k_termio.c_cc[_VEOL2]  = k_termios->c_cc[VEOL2];
	k_termio.c_cc[_VSWTC]  = k_termios->c_cc[VSWTC];

	return copy_to_user(u_termio, &k_termio, sizeof(k_termio));
}
