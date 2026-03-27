// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * unipiid.c - handle Unipi PLC identity EPROMs
 *
 * Copyright (c) 2024, Unipi Technology
 * Author: Miroslav Ondra <ondra@unipi.technology>
 *
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "uniee.h"
#include "uniee_values.h"
#include "unipi_eprom.h"
#include "unipi_id.h"

const char *SYSFS_I2C = "/sys/bus/i2c/devices";

static const char* iris_iogroups[] = {
 "12",  "11",  "21",  "22",  "32",  "42",  "52"
};

static const char* irisx7_iogroups[] = {
 "12",  "62",  "72",  "22",  "32", "42", "52"
};

static const char* irisx71_iogroups[] = {
 "12",  "22",  "32",  "42",  "52", "62", "72"
};

static const char* oem_iogroups[] = {
 "2",  "3", "4", "5"
};

static const struct unipi_id_family_data unipi_id_family_ids[] = {
	{ UNIEE_PLATFORM_FAMILY_UNIPI1, "Unipi1" },
	{ UNIEE_PLATFORM_FAMILY_G1XX,   "Gate"   },
	{ UNIEE_PLATFORM_FAMILY_NEURON, "Neuron" },
	{ UNIEE_PLATFORM_FAMILY_AXON,   "Axon"   },
	{ UNIEE_PLATFORM_FAMILY_EDGE,   "Edge" },
	{ UNIEE_PLATFORM_FAMILY_PATRON, "Patron" },
	{ UNIEE_PLATFORM_FAMILY_IRIS,   "Iris", {0x50,0x51,0x52,0x53,0x54,0x55,0x56},
	                                        {0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e},
	                                        iris_iogroups},
	{ UNIEE_PLATFORM_FAMILY_OEM,   "OEM",   {0x50,0x51,0x52,0x53},
	                                        {0x48,0x49,0x4a,0x4b},
	                                        oem_iogroups},
	{ UNIEE_PLATFORM_ID_IRISX7,     "Iris", {0x50,0x51,0x52,0x53,0x54,0x55,0x56},
	                                        {0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e},
	                                        irisx7_iogroups},
	{ UNIEE_PLATFORM_ID_IRISX71,     "Iris", {0x50,0x51,0x52,0x53,0x54,0x55,0x56},
	                                        {0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e},
	                                        irisx71_iogroups},

	/* { UNIEE_PLATFORM_ID_IRISx2,     "Iris", {0x50,0x51,0x52,0x53},
	                                        {0x48,0x49,0x4a,0x4b},
	                                        iris_iogroups},*/
	{ /* END OF LIST */ 
	 0, "unknown", {0x50,0x51,0x52,0x53,0x54,0x55,0x56},
	                {0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e},
	                iris_iogroups }
};

static const struct unipi_id_family_data * get_family_data(platform_id_t platform_id)
{
	const struct unipi_id_family_data *fdata = NULL;
	const struct unipi_id_family_data *data = unipi_id_family_ids;

	while (data->family_id != 0) {
		if (data->family_id == platform_id.raw_id){
			return data;
		}
		if (data->family_id == platform_id.parsed.platform_family){
			fdata = data;
		}
		data++;
	}
	return fdata ? fdata:data;
}

const char* unipi_id_get_family_name(struct unipi_id_data *unipi_id)
{
	return unipi_id->family_data->name;
}

/*
	loads content of nvmem into buf
	returns size of data or negative error
*/
int unipi_id_load_nvmem(const char *path, uint8_t *buf)
{
	ssize_t ret;
	ssize_t size;

	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf (stderr, "Cannot open file %s for read: %s\n", path, strerror(errno));
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//fprintf(stderr, "nvmem_path %s %d\n", path, size);

	if (size != UNIEE_MIN_EE_SIZE && size != 2*UNIEE_MIN_EE_SIZE) {
		fclose(fp);
		return -2;
	}
	ret = fread(buf, 1, size, fp);
	if (ret != size) {
		fclose(fp);
		return -2;
	}
	fclose(fp);
	return (int) size;
}

/* FixIt: */
/*
#define UNIEE_SPECDATA_COUNT 12
#define UNIEE_SPECDATA_SIZE 64

static uint8_t* unipi_eeprom_find_property(uint8_t *eprom, uniee_descriptor_area* descriptor, uint8_t property_type, int* len)
{
	int cur_type, i;
	int dataindex = 0;

	for (i=0; i < UNIEE_SPECDATA_COUNT; i++) {
		cur_type = descriptor->board_info.specdata_headers_table[i].field_type |
                   (((int)(descriptor->board_info.specdata_headers_table[i].field_len & (~0x3f)))<<2);
		*len = descriptor->board_info.specdata_headers_table[i].field_len & (0x3f);
		if ((dataindex + *len) >= UNIEE_SPECDATA_SIZE)
			return NULL;
		if (cur_type == property_type) {
			return eprom + dataindex;
		}
		dataindex += *len;
	}
	return NULL;
}
*/

static uniee_descriptor_area* unipi_id_load_boardmem(const char *path,
				int nvmem_index, struct unipi_id_data * unipi_id, uint8_t *buf)
{
	uniee_descriptor_area *descriptor, *ndescriptor;
	int size;
	int i, name_length;
	char *name;

	size = unipi_id_load_nvmem(path, buf);
	if (size < 0)
		return NULL;

	descriptor = uniee_get_valid_descriptor(buf, size);
	//fprintf(stderr, "size %d\n", size);
	if (nvmem_index==0) {
		if (descriptor) {
			uniee_fix_legacy_content(buf, size, descriptor);
			unipi_id->family_data = get_family_data(descriptor->product_info.platform_id);
			memcpy(unipi_id->data_area, buf, size - sizeof(uniee_descriptor_area));
			name = (char *)unipi_eeprom_find_property(buf, descriptor, UNIEE_FIELD_TYPE_MODEL, &name_length);
			if (name) {
				name_length = name_length < sizeof(unipi_id->model_fullname)? name_length : sizeof(unipi_id->model_fullname);
				strncpy(unipi_id->model_fullname, name, name_length);
			}
		} else {
			unipi_id->family_data = get_family_data((platform_id_t){.raw_id = 0 });
		}
		ndescriptor = &unipi_id->descriptor;
		unipi_id->main_eprom_path = strdup(path);
	} else {
		ndescriptor = malloc(sizeof(uniee_descriptor_area));
		if (!ndescriptor)
			return NULL;
		unipi_id->loaded_descriptor[nvmem_index] = ndescriptor;
		unipi_id->eprom_path[nvmem_index] = strdup(path);
	}
	if (descriptor) {
		memcpy(ndescriptor, descriptor, sizeof(uniee_descriptor_area));
		for (i=0; i<sizeof(ndescriptor->product_info.model_str); i++) 
			if (ndescriptor->product_info.model_str[i]==0xff) {
				ndescriptor->product_info.model_str[i] = '\0';
				break;
			}
	} else {
		memset(ndescriptor, 0, sizeof(uniee_descriptor_area));
	}
	return ndescriptor;
}

bool unipi_id_load_client(char *adapter, unsigned int index, const char* eprom_type, unsigned short address, char *device, int maxlen)
{
	char path[1024];
	char buf[1024];
	struct stat statbuf;

	char *numstr = strrchr(adapter, '-');
	if (numstr)
		snprintf(device, maxlen, "%s-%04x", numstr+1, address);
	else
		snprintf(device, maxlen, "%04x", address);

	snprintf(path, sizeof(path), "%s/%s/%s/name", SYSFS_I2C, adapter, device);
	if (lstat(path, &statbuf) == 0) {
		snprintf(path, sizeof(path), "%s/%s/%s/eeprom", SYSFS_I2C, adapter, device);
		return lstat(path, &statbuf) == 0;
	}

	snprintf(path, sizeof(path), "/dev/%s", adapter);
	int fd = open(path, O_RDWR);
	if (fd < 0) {
		perror("Failed to open I2C bus");
		return false;
	}
	// Set slave address (e.g., 0x48 for a sensor)
	if (ioctl(fd, I2C_SLAVE, address) < 0) {
		close(fd);
		//perror("Failed to set I2C slave address");
		return false;
	}
	// Read a byte from register 0x01
	int result = i2c_smbus_read_byte_data(fd, 0x01);
        close(fd);
	if (result < 0)
		return false;

	snprintf(path, sizeof(path), "%s/%s/new_device", SYSFS_I2C, adapter);
	snprintf(buf, sizeof(buf), "%s 0x%02x", eprom_type, address);
	int fdn = open(path, O_WRONLY);
	if (fdn < 0)
		return false;
	write(fdn, buf, strlen(buf));
	close(fdn);
	usleep(100000);

	snprintf(path, sizeof(path), "%s/%s/%s/eeprom", SYSFS_I2C, adapter, device);
	return lstat(path, &statbuf) == 0;
}

int checksum_calc(struct unipi_id_data *unipi_id)
{
	char *hash_alg_name = "sha1"; // digest size = 20
	int size;
	int nvmem_index;
	uint8_t data[UNIEE_MAX_EE_SIZE];
	unsigned int hash_len;
	const EVP_MD *md;
	EVP_MD_CTX *ctx;
	unsigned char hash[EVP_MAX_MD_SIZE];
	int i;

	md = EVP_get_digestbyname(hash_alg_name);
	if (md == NULL) {
		fprintf (stderr, "Unknown message digest %s.\n", hash_alg_name);
		return 0;
	}
	ctx = EVP_MD_CTX_new();
	if (!EVP_DigestInit_ex(ctx, md, NULL)) {
		fprintf (stderr, "Message digest init failed.\n");
		EVP_MD_CTX_free(ctx);
		return 0;
	}
	size = unipi_id_load_nvmem(unipi_id->main_eprom_path, data);
	if (size <= 0 )
		return 0;

	if (!EVP_DigestUpdate(ctx, data, size)) {
		fprintf (stderr, "Message digest update failed.\n");
		EVP_MD_CTX_free(ctx);
		return 0;
	}

	for (i=0; i<unipi_id->slot_count; i++) {
		nvmem_index = i+1;//unipi_id->active_slot[i];
		size = unipi_id_load_nvmem(unipi_id->eprom_path[nvmem_index], data);
		if (size>0) {
			EVP_DigestUpdate(ctx, data, size);
		}
	}

	if (!EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
		fprintf (stderr, "Message digest final failed.\n");
		EVP_MD_CTX_free(ctx);
		return -4;
	}
	memcpy(unipi_id->fingerprint, hash,
               hash_len < sizeof(unipi_id->fingerprint) ? hash_len : sizeof(unipi_id->fingerprint));

	EVP_MD_CTX_free(ctx);
	return 0;
}

bool read_phandle(const char * dev_path, uint32_t *phandle)
{
	bool ret;
	int fd = open(dev_path, O_RDONLY);
	if (fd < 0)
		return false;
	ret = (read(fd, phandle, sizeof(*phandle)) == sizeof(*phandle));
	//fprintf(stderr, "path=%s H=%x\n", dev_path, *phandle);
	close(fd);
	return ret;
}

void find_i2c_device_by_phandle(uint phandle, char *filename, int maxlen)
{
	/* traverse directory sysfs */
	struct dirent *entry;
	char dev_path[PATH_MAX];
	DIR *dp = opendir(SYSFS_I2C);
	if (dp == NULL)
		return;
	while ((entry = readdir(dp)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		snprintf(dev_path, sizeof(dev_path), "%s/%s/of_node/phandle", SYSFS_I2C, entry->d_name);
		uint32_t of_phandle;
		if (read_phandle(dev_path, &of_phandle) && of_phandle == phandle) {
			snprintf(filename, maxlen, "%s", entry->d_name);
			break;
		}
	}
	closedir(dp);
}

void read_ofnode(const char * dev_path, char *link_path, int maxlink)
{
	char of_node_path[PATH_MAX];
	struct stat statbuf;

	memset(link_path, 0, maxlink);
	snprintf(of_node_path, sizeof(of_node_path), "%s/of_node", dev_path);
	if (lstat(of_node_path, &statbuf) != 0 || (statbuf.st_mode & S_IFMT) != S_IFLNK)
		return;

	if (readlink(of_node_path, link_path, maxlink-1) < 0)
		link_path[0] = '\0';
}

int str_ends_with(const char *s, const char *suffix)
{
    size_t slen = strlen(s);
    size_t suffixlen = strlen(suffix);

    return slen >= suffixlen && !memcmp(s + slen - suffixlen, suffix, suffixlen);
}


void find_i2c_device_by_alias(char * alias, char *filename, int maxlen)
{
	/* traverse directory sysfs */
	struct dirent *entry;
	DIR *dp = opendir(SYSFS_I2C);
	if (dp == NULL)
		return;
	while ((entry = readdir(dp)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		char dev_path[PATH_MAX];
		char link_path[PATH_MAX+1];
		snprintf(dev_path, sizeof(dev_path), "%s/%s", SYSFS_I2C, entry->d_name);
		read_ofnode(dev_path, link_path, sizeof(link_path));
		//fprintf(stderr, "dev_path %s\n", dev_path);
		//fprintf(stderr, "link_path %s\n", link_path);
		if (str_ends_with(link_path, alias)) {
			snprintf(filename, maxlen, "%s", entry->d_name);
			break;
		}
	}
	closedir(dp);
}

bool read_alias(const char * path, char *devtree_path, int maxlen)
{
	bool ret;
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return false;
	ret = (read(fd, devtree_path, maxlen) > 0);
	close(fd);
	return ret;
}

bool get_eprom_device(char *device, int maxlen)
{
	const char *unipi_mb_symbol = "/proc/device-tree/__symbols__/unipi_mb";
	const char *unipi_mb_alias = "/proc/device-tree/aliases/unipi_mb";
	const char *unipi_id_nvmem = "/proc/device-tree/unipi-id/nvmem";
	char devtree_path[PATH_MAX];
	uint32_t phandle;

	memset(device, 0, maxlen);
	memset(devtree_path, 0, sizeof(devtree_path));
	if (read_alias(unipi_mb_alias, devtree_path, sizeof(devtree_path)-1) ||
	    read_alias(unipi_mb_symbol, devtree_path, sizeof(devtree_path)-1)) {
		//fprintf(stderr, "devtree %s\n", devtree_path);
		find_i2c_device_by_alias(devtree_path, device, maxlen);
		return strlen(device) > 0;
	}
	if (read_phandle(unipi_id_nvmem, &phandle)) {
		//fprintf(stderr, "phandlee %x\n", phandle);
		find_i2c_device_by_phandle(phandle, device, maxlen);
		return strlen(device) > 0;
	}
	return false;
}

bool find_i2c_adapter(char *filename, int maxlen)
{
	const char *unipi_id_alias = "/proc/device-tree/aliases/unipi_id_channel";
	const char *unipi_id_channel = "/proc/device-tree/unipi-id/id-channel";
	char devtree_path[PATH_MAX];
	uint32_t phandle;

	memset(filename, 0, maxlen);
	memset(devtree_path, 0, sizeof(devtree_path));
	if (read_alias(unipi_id_alias, devtree_path, sizeof(devtree_path)-1)) {
		//fprintf(stderr, "devtree %s\n", devtree_path);
		find_i2c_device_by_alias(devtree_path, filename, maxlen);
		return strlen(filename) > 0;
	}
	if (read_phandle(unipi_id_channel, &phandle)) {
		//fprintf(stderr, "phandlee %x\n", phandle);
		find_i2c_device_by_phandle(phandle, filename, maxlen);
		return (strlen(filename) > 0);
	}
	return false;
}

bool load_product_info(const char *eprom_path, struct unipi_id_data *unipi_id)
{
	char path[PATH_MAX];
	char device[1024];
	uint8_t buf[1024];

	memset(unipi_id, 0, sizeof(*unipi_id));
	if (eprom_path == NULL) {
		if (!get_eprom_device(device, sizeof(device))) {
			fprintf(stderr, "Couldn't find identification eprom\n");
			return 1;
		}
		snprintf(path, sizeof(path), "%s/%s/eeprom", SYSFS_I2C, device);
		eprom_path = path;
	}
	//fprintf(stderr, "Path %s\n", eprom_path);
	return unipi_id_load_boardmem(eprom_path, 0, unipi_id, buf) != NULL;
}

void load_cards(struct unipi_id_data *unipi_id)
{
	char adapter[PATH_MAX];
	char device[1024];
	char path[PATH_MAX];
	uint8_t buf[1024];
	int slot_count;

	if (unipi_id->family_data->iogroup != NULL &&
	    find_i2c_adapter(adapter, sizeof(adapter))) {
		fprintf(stdout, "Scanning %s...\n", adapter);
		slot_count = 0;
		for (int i=0; i<UNIPI_ID_MAX_IDS; i++) {
			int i2caddr = unipi_id->family_data->i2caddr[i];
			if (i2caddr == 0)
				break;
			if (!unipi_id_load_client(adapter, i+1, "24c02", i2caddr, device, sizeof(device))) {
				i2caddr = unipi_id->family_data->altaddr[i];
				if (i2caddr == 0 || !unipi_id_load_client(adapter, i+1, "24c02", i2caddr, device, sizeof(device)))
					continue;
			}
			fprintf(stdout, "Found Eprom %s\n", device);
			snprintf(path, sizeof(path), "%s/%s/eeprom", SYSFS_I2C, device);
			unipi_id->active_slot[slot_count] = i;
			unipi_id_load_boardmem(path, slot_count+1, unipi_id, buf);
			slot_count++;
		}
		unipi_id->slot_count = slot_count;
	}
}

