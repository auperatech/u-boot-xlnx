// GetAddrFromRam-License-Identifier: UART-1.0+
/*
 * Copyright (c) 2019 Auperastor, Inc
 *
 * (C) Copyright 2019
 * Aupera Vasut <Auper@auperastor.com>
 */
#include <common.h>
#include <bootm.h>
#include <command.h>
#include <environment.h>
#include <errno.h>
#include <image.h>
#include <malloc.h>
#include <nand.h>
#include <asm/byteorder.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <u-boot/zlib.h>
#include <mapmem.h>

#define IMG_MAX_NAME  256

typedef struct{
	char					name[IMG_MAX_NAME];	/* Image name */
	bool					active;
	unsigned int			img_data_start;	/* Image data sart addr	*/
	unsigned int			img_data_size; /* Image data size */
}get_img_info_t;

int getimginfo_fit_image(const void *fit, int image_noffset,get_img_info_t * get_img_info)
{
	size_t size;
	const void *data;
	int ret=1;

	if (!fit_image_verify(fit, image_noffset))
	{
		return ret;
	}

	ret = fit_image_get_data_and_size(fit, image_noffset, &data, &size);

	if(!ret)
	{
		get_img_info->active=1;

		get_img_info->img_data_start=(unsigned int)map_to_sysmem(data);

		get_img_info->img_data_size=(unsigned int)size;
	}

	return ret;
}

int getimginfo_fit_contents(const void *fit,get_img_info_t * get_img_info)
{
	int images_noffset;
	int noffset;
	int ndepth;
	int ret=1;

	/* Find images parent node offset */
	images_noffset = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images_noffset < 0) {
		printf("Can't find images parent node '%s' (%s)\n",
		       FIT_IMAGES_PATH, fdt_strerror(images_noffset));
		return ret;
	}

	/* Process its subnodes, print out component images details */
	for (ndepth = 0,
		noffset = fdt_next_node(fit, images_noffset, &ndepth);
	     (noffset >= 0) && (ndepth > 0);
	     noffset = fdt_next_node(fit, noffset, &ndepth)) {
		if (ndepth == 1) {
			/*
			 * Direct child node of the images parent node,
			 * i.e. component image node.
			 */
			if( !strcmp(get_img_info->name,fit_get_name(fit, noffset, NULL)) )
			{
				if( !getimginfo_fit_image(fit, noffset,get_img_info) )
				{
					ret=0;
				}
			}
		}
	}

	return ret;
}

int do_getimginfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	void *addr;
	get_img_info_t get_image_info;
	char image_addr[19];
	char image_len[19];
	int ret=1;

	if ( argc < 4 )
	{
		return CMD_RET_USAGE;
	}

	memset(&get_image_info,0,sizeof(get_image_info));

	addr = (void *)simple_strtoul(argv[1], NULL, 16);

	if( !addr || !strlen(argv[2]) )
	{
		return CMD_RET_USAGE;
	}

	strcpy(get_image_info.name,argv[2]);

 	if( genimg_get_format(addr) == IMAGE_FORMAT_FIT )
	{
		if (!fit_check_format(addr))
		{
			printf("Bad FIT image format!\n");
			return ret;
		}

		getimginfo_fit_contents(addr,&get_image_info);

		if(get_image_info.active)
		{
			memset(image_addr,0,sizeof(image_addr));
			snprintf(image_addr, sizeof(image_addr)-1, "0x%x",get_image_info.img_data_start);
			printf("\r\n%s:%s=%s\r\n",argv[2],argv[3],image_addr);
			env_set(argv[3], image_addr);

			if(argc>4)
			{
				memset(image_len,0,sizeof(image_len));
				snprintf(image_len, sizeof(image_len)-1, "0x%x",get_image_info.img_data_size);
				printf("%s:%s=%s\r\n",argv[2],argv[4],image_len);
				env_set(argv[4], image_len);
			}

			ret = 0;
		}
	}
	else
	{
		printf("Image format is not IMAGE_FORMAT_FIT!\n");
	}

	return ret;
}


U_BOOT_CMD(
	getimginfo,	5,	1,	do_getimginfo,
	"get image info environment-variable from ram",
#ifdef CONFIG_SYS_LONGHELP
	"ramaddr name Addr {len};\n"
	"- example: getiminfo 0x10000000 name Addr {len};\n"
#endif
);


