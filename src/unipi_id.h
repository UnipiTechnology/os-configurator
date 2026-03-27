/*
 *
 * Copyright (c) 2021  Faster CZ, ondra@faster.cz
 *
 * SPDX-License-Identifier: LGPL-2.1+
 *
 */

#ifndef UNIPI_ID_H_
#define UNIPI_ID_H_

#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "uniee.h"
#include "uniee_values.h"

#define UNIPI_ID_MAX_IDS     8
#define UNIPI_ID_FINGERPRINT_SIZE     20

struct unipi_id_family_data {
	uint16_t	family_id;
	char		name[32];
	uint8_t		i2caddr[UNIPI_ID_MAX_IDS];
	uint8_t		altaddr[UNIPI_ID_MAX_IDS];
	const char**iogroup;
};

struct unipi_id_data
{
	int slot_count;
	//struct i2c_client * loaded_clients[UNIPI_ID_MAX_IDS];
	const struct unipi_id_family_data *family_data;
	uniee_descriptor_area descriptor;
	uint8_t data_area[UNIEE_MAX_SPECDATA_AREA_SIZE];
	int active_slot[UNIPI_ID_MAX_IDS];
	uint8_t fingerprint[UNIPI_ID_FINGERPRINT_SIZE];
	uniee_descriptor_area *loaded_descriptor[UNIPI_ID_MAX_IDS];
	char model_fullname[64];
	char *main_eprom_path;
	char *eprom_path[UNIPI_ID_MAX_IDS];
};

const char* unipi_id_get_family_name(struct unipi_id_data *unipi_id);
bool export_unipi_id(struct unipi_id_data *unipi_id);
int checksum_calc(struct unipi_id_data *unipi_id);
bool load_product_info(const char *eprom_path, struct unipi_id_data *unipi_id);
void load_cards(struct unipi_id_data *unipi_id);
char* get_unipi_id_item2(struct unipi_id_data *unipi_id, const char* item, int trunc);
char* get_unipi_id_item(const char* item, int trunc);


#endif /* UNIPI_ID_H_*/
