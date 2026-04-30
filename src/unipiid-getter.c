// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * unipiid.c - handle Unipi PLC identity EPROMs
 *
 * Copyright (c) 2024, Unipi Technology
 * Author: Miroslav Ondra <ondra@unipi.technology>
 *
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "uniee.h"
#include "uniee_values.h"
#include "unipi_eprom.h"

#include "unipi_id.h"

#define UNIPI_PLC_DIR "/run/unipi-plc"
#define UNIPI_ID_DIR  "/run/unipi-plc/unipi-id"

static ssize_t product_description_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t product_model_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t product_model_full_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t product_ver_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t product_serial_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t product_code_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t product_family_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t product_options_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t platform_family_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t platform_id_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t mainboard_description_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t mainboard_id_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t uboard_id_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t fingerprint_show(struct unipi_id_data *unipi_id, char *buf);
static ssize_t api_version_show(struct unipi_id_data *unipi_id, char *buf);

static ssize_t card_description_show(struct unipi_id_data *unipi_id, int nvmem_index, char *buf);
static ssize_t card_id_show(struct unipi_id_data *unipi_id, int nvmem_index, char *buf);

struct unipi_id_item {
	const char* item;
	ssize_t (*func)(struct unipi_id_data *unipi_id, char *buf);
};

struct unipi_card_item {
	const char* item;
	ssize_t (*func)(struct unipi_id_data *unipi_id, int nvmem_index, char *buf);
};

const struct unipi_id_item unipi_id_items[] = {
	{"product_description", product_description_show},
	{"product_model", product_model_show},
	{"product_model_full", product_model_full_show},
	{"product_version", product_ver_show},
	{"product_serial", product_serial_show},
	{"product_code", product_code_show},
	{"product_family", product_family_show},
	{"product_options", product_options_show},
	{"platform_family", platform_family_show},
	{"platform_id", platform_id_show},
	{"mainboard_description", mainboard_description_show},
	{"mainboard_id", mainboard_id_show},
	{"uboard_id", uboard_id_show},
	{"fingerprint", fingerprint_show},
	{"api_version", api_version_show},
	{NULL, NULL}
};

const struct unipi_card_item unipi_card_items[] = {
	{"card_description.%s", card_description_show},
	{"card_id.%s", card_id_show},
	{NULL, NULL}
};


static ssize_t product_model_full_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_3_t *bank3;
	if (unipi_id == NULL)
		return 0;
	if (unipi_id->model_fullname[0] != '\0')
		return snprintf(buf, 255, "%.64s\n", unipi_id->model_fullname);

	bank3 = &unipi_id->descriptor.product_info;
	return snprintf(buf, 255, "%.6s\n", bank3->model_str);
}

static ssize_t product_model_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_3_t *bank3;
	if (unipi_id == NULL)
		return 0;
	bank3 = &unipi_id->descriptor.product_info;
	return snprintf(buf, 255, "%.6s\n", bank3->model_str);
}

static ssize_t product_ver_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_3_t *bank3;
	if (unipi_id == NULL)
		return 0;
	bank3 = &unipi_id->descriptor.product_info;
	return snprintf(buf, 255, "%d.%d", bank3->product_version.major, bank3->product_version.minor);
}

static ssize_t product_code_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_3_t *bank3;
	if (unipi_id == NULL)
		return 0;
	bank3 = &unipi_id->descriptor.product_info;
	if (unipi_id->model_fullname[0] != '\0')
		return snprintf(buf, 255, "#%u;%.64s_%u.%u;%u#", bank3->sku,
		                unipi_id->model_fullname,
		                bank3->product_version.major, bank3->product_version.minor,
		                bank3->product_serial);
	return snprintf(buf, 255, "#%u;%.6s_%u.%u;%u#", bank3->sku,
	                bank3->model_str,
	                bank3->product_version.major, bank3->product_version.minor,
	                bank3->product_serial);
}

static ssize_t product_serial_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_3_t *bank3;
	if (unipi_id == NULL)
		return 0;
	bank3 = &unipi_id->descriptor.product_info;
	return snprintf(buf, 255, "%u", bank3->product_serial);
}

static ssize_t product_family_show(struct unipi_id_data *unipi_id, char *buf)
{
	if (unipi_id == NULL)
		return 0;
	return snprintf(buf, 255, "%s", unipi_id_get_family_name(unipi_id));
}

static ssize_t product_options_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_3_t *bank3;
	if (unipi_id == NULL)
		return 0;
	bank3 = &unipi_id->descriptor.product_info;
	return snprintf(buf, 255, "%02X", bank3->mervis_license.bitmask);
}

static ssize_t product_description_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_3_t *bank3;

	if (unipi_id == NULL)
		return 0;
	bank3 = &unipi_id->descriptor.product_info;

	return snprintf(buf, 2048,
				"Product model:   %.6s\n"
				"Product version: %u.%u\n"
				"Product serial:  %08u\n"
				"SKU:             %u\n"
				"Options:         0x%02X\n\n"
				"Platform family: %s (%d)\n"
				"Platform series: 0x%02X\n"
				"RAW platform ID: 0x%04X\n", 
				bank3->model_str,
				bank3->product_version.major, bank3->product_version.minor,
				bank3->product_serial,
				bank3->sku,
				bank3->mervis_license.bitmask,
				unipi_id_get_family_name(unipi_id),
				bank3->platform_id.parsed.platform_family,
				bank3->platform_id.parsed.platform_series,
				bank3->platform_id.raw_id);
}

static ssize_t platform_family_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_3_t *bank3;
	if (unipi_id == NULL)
		return 0;
	bank3 = &unipi_id->descriptor.product_info;
	return snprintf(buf, 255, "%d", bank3->platform_id.parsed.platform_family);
}

static ssize_t platform_id_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_3_t *bank3;
	if (unipi_id == NULL)
		return 0;
	bank3 = &unipi_id->descriptor.product_info;
	return snprintf(buf, 255, "%04x", bank3->platform_id.raw_id);
}

static ssize_t mainboard_id_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_2_t *bank2;
	if (unipi_id == NULL)
		return 0;
	bank2 = &unipi_id->descriptor.board_info;
	return snprintf(buf, 255, "%04x", bank2->board_model);
}

static ssize_t mainboard_description_show(struct unipi_id_data *unipi_id, char *buf)
{
	uniee_bank_2_t *bank2;
	char sversion[256];
	int ret;

	if (unipi_id == NULL)
		return 0;
	bank2 = &unipi_id->descriptor.board_info;

	if (bank2->board_version.minor >= 'A')
		snprintf(sversion, sizeof(sversion), "%u%c", bank2->board_version.major, bank2->board_version.minor);
	else
		snprintf(sversion, sizeof(sversion), "%u.%u", bank2->board_version.major, bank2->board_version.minor);
	ret = snprintf(buf, 2048,
				"Model:   Mainboard\n"
				"Version: %s\n"
				"Serial:  %08u\n"
				"ID:      0x%04X\n"
				"Nvmem:   %s\n",
				sversion,
				bank2->board_serial,
				bank2->board_model,
				unipi_id->main_eprom_path);
	return ret;
}

static ssize_t uboard_id_show(struct unipi_id_data *unipi_id, char *buf)
{
	uint8_t *data;
	int len = 0;

	if (unipi_id == NULL)
		return 0;

	data = unipi_eeprom_find_property(unipi_id->data_area, &unipi_id->descriptor,
	                                  UNIEE_FIELD_TYPE_UPPER_BOARD, &len);
	if (data == NULL || len != 2)
		return 0;
	return snprintf(buf, 255, "%04x", data[0]|(data[1]>>8));
}

static ssize_t card_id_show(struct unipi_id_data *unipi_id, int nvmem_index, char *buf)
{
	if (unipi_id == NULL)
		return 0;
	uniee_descriptor_area *descriptor = unipi_id->loaded_descriptor[nvmem_index];
	if (!descriptor)
		return 0;
	return snprintf(buf, 255, "%04x", descriptor->board_info.board_model);
}

static ssize_t card_description_show(struct unipi_id_data *unipi_id, int nvmem_index, char *buf)
{
	if (unipi_id == NULL)
		return 0;
	int ret;
	uniee_descriptor_area *descriptor = unipi_id->loaded_descriptor[nvmem_index];

	if (!descriptor)
		return 0;

	ret = snprintf(buf, 2048,
				"Model:   %.6s\n"
				"Version: %u.%u\n"
				"Serial:  %08u\n"
				"ID:      0x%04X\n"
				"Nvmem:   %s\n"
				"Slot:    %s\n",
				descriptor->product_info.model_str,
				descriptor->board_info.board_version.major, descriptor->board_info.board_version.minor,
				descriptor->board_info.board_serial,
				descriptor->board_info.board_model,
				unipi_id->eprom_path[nvmem_index],
				unipi_id->family_data->iogroup[unipi_id->active_slot[nvmem_index-1]]);
	return ret;
}

static ssize_t fingerprint_show(struct unipi_id_data *unipi_id, char *buf)
{
	if (unipi_id == NULL)
		return 0;
	for (int i=0; i<UNIPI_ID_FINGERPRINT_SIZE; i++)
		snprintf(buf+2*i, 3, "%02x", unipi_id->fingerprint[i]);
	return 2*UNIPI_ID_FINGERPRINT_SIZE+1;
}

static ssize_t api_version_show(struct unipi_id_data *unipi_id, char *buf)
{
	return snprintf(buf, 255, "%d", 2);
}

bool create_empty_dir(const char *dir)
{
	char buf[2048];

	if (snprintf(buf, sizeof(buf), "mkdir -p %s", dir) >= sizeof(buf)-1)
		return false;
	if (system(buf))
		return false;
	if (snprintf(buf, sizeof(buf), "rm -f %s/*", dir) >= sizeof(buf)-1)
		return false;
	if (system(buf))
		return false;
	return true;
}

bool rename_dir(const char *src, const char *dst)
{
	char buf[2048];

	if (snprintf(buf, sizeof(buf), "rm -rf %s", dst) >= sizeof(buf)-1)
		return false;
	if (system(buf))
		return false;
	if (snprintf(buf, sizeof(buf), "mv %s %s", src, dst) >= sizeof(buf)-1)
		return false;
	if (system(buf))
		return false;
	return true;
}

void write_item(const char *item, const char* buf)
{
	int ret;
	FILE *fp = fopen(item, "w");
	if (fp == NULL) {
		fprintf (stderr, "Cannot create file %s: %s\n", item, strerror(errno));
		return;
	}
	ret = fprintf(fp, "%s", buf);
	fclose(fp);
	if (ret < 0)
		unlink(item);
}

bool export_unipi_id(struct unipi_id_data *unipi_id)
{
	const struct unipi_id_item *uid;
	const struct unipi_card_item *cid;
	char buf[2048];
	char name[1024];
	char tmpdir[2048] = UNIPI_PLC_DIR "/XXXXXX";

	if (!create_empty_dir(tmpdir)) {
		fprintf(stderr, "Cannot create directory %s\n", UNIPI_ID_DIR);
	}
	if (chdir(tmpdir) < 0) {
		fprintf(stderr, "Cannot change directory %s: %s\n", UNIPI_ID_DIR, strerror(errno));
		return false;
	}

	for (uid = unipi_id_items; uid->item; uid++)
		if (uid->func(unipi_id, buf) > 0)
			write_item(uid->item, buf);

	for (int i=0; i<unipi_id->slot_count; i++) {
		int nvmem_index = i+1;
		const char *slot = unipi_id->family_data->iogroup[unipi_id->active_slot[i]];
		for (cid = unipi_card_items; cid->item; cid++) {
			snprintf(name, sizeof(name), cid->item, slot);
			if (cid->func(unipi_id, nvmem_index, buf) > 0) {
				write_item(name, buf);
			}
		}
	}
	rename_dir(tmpdir, UNIPI_ID_DIR);
	return true;
}

char* get_unipi_id_item2(struct unipi_id_data *unipi_id, const char* item, int trunc)
{
	const struct unipi_id_item *uid;
	char buf[2048];

	for (uid = unipi_id_items; uid->item; uid++) {
		if (strcmp(uid->item, item))
			continue;
		int len = uid->func(unipi_id, buf);
		if (len > 0) {
			if (trunc && buf[len-1] == '\n')
				buf[len-1] = '\0';
			return strdup(buf);
		}
	}
	return NULL;
}

/* Get data from cache in /run/unipi-plc/unipi-id */
#define MAX_DATA 2048
char* get_unipi_id_item(const char* item, int trunc)
{
	int res;
	char *data, *lf;
	char fname[2048];
	snprintf(fname, sizeof(fname)-1, "%s/%s", UNIPI_ID_DIR, item);
	fname[sizeof(fname)-1] = '\0';
	//printf("%s\n", fname);
	int f = open(fname, O_RDONLY);
	if (f < 0)
		return NULL;
	data = malloc(MAX_DATA);
	if (data == NULL)
		goto err;
	res = read(f, data, MAX_DATA-1);
	if (res <= 0)
		goto err1;
	if (trunc && res > 0 && data[res-1] == '\n')
		res = res -1;
	data[res] = '\0';
	close(f);
	return data;
err1:
	free(data);
err:
	close(f);
	return NULL;
}
