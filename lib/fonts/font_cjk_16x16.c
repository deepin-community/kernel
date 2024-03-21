// SPDX-License-Identifier: GPL-2.0
/***************************************************************/
/*                                                             */
/* Font file modified from                                     */
/* http://blog.chinaunix.net/u/13265/showart.php?id=1008020    */
/* microcaicai@gmail modifiy it to use in-kernel font solution */
/*                                                             */
/***************************************************************/

#include <linux/font.h>

#define FONTDATAMAX (65536 * 32)

static const struct font_data fontdata_cjk_16x16 = {
	{ 0, 0, FONTDATAMAX, 0 }, {
	#include "font_cjk_16x16.h"
}};

const struct font_desc font_cjk_16x16 = {
	.idx	= CJK16x16_IDX,
	.name	= "CJK16x16",
	.width	= 8, /* make cursor appear 8 dot width */
	.height	= 16,
	.charcount	= 65536 * 2,
	.data	= fontdata_cjk_16x16.data,
	.pref	= 20, /* make it big enough to be chosen */
};
