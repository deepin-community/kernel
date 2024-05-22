// SPDX-License-Identifier: GPL-2.0
/***************************************************************/
/*                                                             */
/* Font file modified from                                     */
/* http://blog.chinaunix.net/u/13265/showart.php?id=1008020    */
/* microcaicai@gmail modifiy it to use in-kernel font solution */
/*                                                             */
/***************************************************************/

#include <linux/font.h>

#define FONTDATAMAX (65536 * 128)

static const struct font_data fontdata_cjk_32x32 = {
	{ 0, 0, FONTDATAMAX, 0 }, {
	#include "font_cjk_32x32.h"
}};

const struct font_desc font_cjk_32x32 = {
	.idx	= CJK32x32_IDX,
	.name	= "CJK32x32",
	.width	= 16,
	.height	= 32,
	.charcount	= 65536 * 2,
	.data	= fontdata_cjk_32x32.data,
	.pref	= -1,
};
