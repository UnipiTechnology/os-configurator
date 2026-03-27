
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libgen.h>

//#include "unipiutil.h"
#include "uniee_values.h"
#include "unipi_id.h"
#include "unipi_eprom.h"

bool hostname_in_args(int argc, char** argv)
{
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)))
		return false;
	for (int i = 1; i< argc; i++)
		if (strcmp(hostname, argv[i]) == 0)
			return true;
	return false;
}

int do_hostname(int argc, char** argv)
{
	int do_set = 0;
	struct unipi_id_data unipi_id;
	char hostname[256];
	char *unipi_model, *unipi_serial, *unipi_platform;

	if (!load_product_info(NULL, &unipi_id))
		return 0;
	if (argc > 1) {
		if (!hostname_in_args(argc, argv))
			return 1;
		do_set = 1;
	}

	unipi_model = get_unipi_id_item2(&unipi_id, "product_model", 1);
	if (unipi_model==NULL)
		return 0;
	unipi_serial = get_unipi_id_item2(&unipi_id, "product_serial", 1);
	if (unipi_serial==NULL) {
		free(unipi_model);
		return 0;
	}
	unipi_platform = get_unipi_id_item2(&unipi_id, "platform_id", 1);
	if (unipi_platform==NULL) {
		free(unipi_serial);
		free(unipi_model);
		return 0;
	}

	if(strncmp((char*)(unipi_platform + 2), "01", 2) == 0)
		snprintf(hostname, 255, "UNIPI1-sn%s", unipi_serial);
	else
		snprintf(hostname, 255, "%s-sn%s", unipi_model, unipi_serial);
	hostname[255] = '\0';
	if (do_set)
		sethostname(hostname,strlen(hostname));
	else
		printf("%s\n", hostname);

	free(unipi_platform);
	free(unipi_serial);
	free(unipi_model);
	return 0;
}

char* parse_nvmem_from_description(char* itemname)
{
	struct unipi_id_data unipi_id;

	if (!load_product_info(NULL, &unipi_id))
		return NULL;

	if (strcmp(itemname, "mainboard_description") == 0)
		return strdup(unipi_id.main_eprom_path);

	return NULL;
//		load_cards(&unipi_id);
//		return 0;
//	}
//	return 1;
}


struct uniee_map uniee_field_type_map[] = UNIEE_FIELD_TYPE_MAP;
#define map_length DIM(uniee_field_type_map)

int print_property(int property_type, int len, uint8_t data[])
{
	int i;
	const char *name = NULL;
	for (i=0; i<map_length; i++) {
		if (property_type == uniee_field_type_map[i].index) {
			name = uniee_field_type_map[i].name;
			break;
		}
	}
	printf("%-3d %-3d", property_type, len);
	if (len==1) printf(" %d", data[0]);
	else if (len ==2) printf(" %d", data[0]|(data[1]>>8));
	else if (len ==4) printf(" %d", data[0]|(data[1]>>8)|(data[1]>>16)|(data[1]>>24));
	else {
		for (i=0; i<len; i++) printf(" %02x", data[i]);
	}
	if (name) 
		printf("\t# %s\n", name);
	else
		printf("\n");
	return 0;
}

int do_attrs(char* itemname)
{
	struct unipi_id_data unipi_id;

	if (strcmp(itemname, "mainboard_description") != 0)
		return 0; /* ToDo: card_description */

	if (!load_product_info(NULL, &unipi_id))
		return 1;
	printf("typ len value\n");
	unipi_eeprom_enum_properties(unipi_id.data_area, &unipi_id.descriptor, print_property);
	return 0;
}

int do_attrs2(char* itemname, char* attrname)
{
	struct unipi_id_data unipi_id;
	int i, j, len;
	uint8_t *data;

	if (strcmp(itemname, "mainboard_description") != 0)
		return 0; /* ToDo: card_description */

	if (!load_product_info(NULL, &unipi_id))
		return 1;

	for (i=0; i<map_length; i++) {
		if (strcasecmp(attrname, uniee_field_type_map[i].name) == 0) {
			data = unipi_eeprom_find_property(unipi_id.data_area, &unipi_id.descriptor,
			                                  uniee_field_type_map[i].index, &len);
			//data = get_unipi_eeprom_property(nvmem, uniee_field_type_map[i].index, &len);
			if (data == NULL) return 1;
			if (len==1) printf("%d\n", data[0]);
			else if (len ==2) printf("%d\n", data[0]|(data[1]>>8));
			else if (len ==4) printf("%d\n", data[0]|(data[1]>>8)|(data[1]>>16)|(data[1]>>24));
			else {
				for (j=0; j<len; j++) printf("%s%02x", j?" ":"", data[j]);
				printf("\n");
			}
			return 0;
		}
	}
	return 2;
}

int export_dir(int argc, char** argv)
{
	char *eprom_path;
	struct unipi_id_data unipi_id;

	eprom_path = NULL;
	if (argc > 2)
		eprom_path = argv[2];

	if (load_product_info(eprom_path, &unipi_id)) {
		load_cards(&unipi_id);
		checksum_calc(&unipi_id);
		export_unipi_id(&unipi_id);
		return 0;
	}
	return 1;
}

int help(void)
{
	fprintf(stderr, "Usage:\n" \
	"    unipiid -d     - create and populate directory /run/unipi-plc/unipi-id\n" \
	"    unipiid NAME   - show value of parameter name\n" \
	"    unipiid mainboard_description attr [opt] - show list/value of optional parameters\n\n" \
	"Where NAME can be:\n" \
	"    product_description   product_model   product_model_full\n" \
	"    product_version       product_serial  product_code\n" \
	"    product_family        product_options\n" \
	"    platform_family       platform_id\n" \
	"    mainboard_description mainboard_id\n" \
	"    api_version\n\n");
	return 1;
}

int main(int argc, char** argv)
{
	char * unipi_item;
	int do_strip = 1;
	struct unipi_id_data unipi_id;

	if (strcmp(basename(argv[0]), "unipihostname") == 0)
		return do_hostname(argc, argv);

	if (argc <= 1)
		return help();

	if (strcmp(argv[1], "-d") == 0)
		return export_dir(argc, argv);

	if (strcmp(argv[1], "unipihostname") == 0 || strcmp(argv[1], "hostname") == 0)
		return do_hostname(argc-1, &argv[1]);

	if (strstr(argv[1], "description") != NULL) {
		do_strip = 0;
		if ((argc > 2) && (strcmp(argv[2], "attr") == 0)) {
			if (argc > 3) {
				return do_attrs2(argv[1], argv[3]);
			} else {
				return do_attrs(argv[1]);
			}
		}
	}
	if (strcmp(argv[1], "fingerprint") == 0) {
		/* use cached value */
		unipi_item = get_unipi_id_item(argv[1], 1);
		if (unipi_item) {
			printf("%s", unipi_item);
			free(unipi_item);
		}
		return 0;
	}

	if (load_product_info(NULL, &unipi_id)) {
		unipi_item = get_unipi_id_item2(&unipi_id, argv[1], do_strip);
		if (unipi_item) {
			printf("%s", unipi_item);
			free(unipi_item);
		}
	}
	return 0;
}
