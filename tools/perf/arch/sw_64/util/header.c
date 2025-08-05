// SPDX-License-Identifier: GPL-2.0-only
/*
 * Implementation of get_cpuid().
 *
 * Author: Nikita Shubin <n.shubin@yadro.com>
 *         Bibo Mao <maobibo@loongson.cn>
 *         Huacai Chen <chenhuacai@loongson.cn>
 *         Chuyue He <hechuyue@wxiat.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <api/fs/fs.h>
#include <errno.h>
#include "util/debug.h"
#include "util/header.h"

/*
 * Output example from /proc/cpuinfo
 *   vendor_id	             : sunway
 *   cpu family              : 8
 *   model	             : 65
 */
#define CPUINFO_VENDOR	"vendor_id"
#define CPUINFO_FAMILY	"cpu family"
#define CPUINFO_MODEL	"model"
#define CPUINFO		"/proc/cpuinfo"

static char *_get_field(const char *line)
{
	char *line2, *nl;

	line2 = strrchr(line, ' ');
	if (!line2)
		return NULL;

	line2++;
	nl = strrchr(line, '\n');
	if (!nl)
		return NULL;

	return strndup(line2, nl - line2);
}

static char *_get_cpuid(void)
{
	unsigned long line_sz;
	char *line, *vendorid, *family, *model, *cpuid;
	FILE *file;

	file = fopen(CPUINFO, "r");
	if (file == NULL)
		return NULL;

	line = vendorid = family = model = cpuid = NULL;
	while (getline(&line, &line_sz, file) != -1) {
		if (!strncmp(line, CPUINFO_VENDOR, strlen(CPUINFO_VENDOR))) {
			vendorid = _get_field(line);
			if (!vendorid)
				goto out_free;
		} else if (!strncmp(line, CPUINFO_FAMILY, strlen(CPUINFO_FAMILY))) {
			family = _get_field(line);
			if (!family)
				goto out_free;
		} else if (!strncmp(line, CPUINFO_MODEL, strlen(CPUINFO_MODEL))) {
			model = _get_field(line);
			if (!model)
				goto out_free;
		break;
		}
	}

	if (!vendorid || !family || !model)
		goto out_free;

	if (asprintf(&cpuid, "%s-%s-%s", vendorid, family, model) < 0)
		cpuid = NULL;

out_free:
	fclose(file);
	free(vendorid);
	free(family);
	free(model);

	return cpuid;
}

int get_cpuid(char *buffer, size_t sz)
{
	int ret = 0;
	char *cpuid = _get_cpuid();

	if (!cpuid)
		return -EINVAL;

	if (sz < strlen(cpuid)) {
		ret = ENOBUFS;
		goto out_free;
	}

	scnprintf(buffer, sz, "%s", cpuid);

out_free:
	free(cpuid);
	return ret;
}

char *get_cpuid_str(struct perf_pmu *pmu __maybe_unused)
{
	return _get_cpuid();
}
