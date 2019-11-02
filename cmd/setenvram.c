/*
 *******************************************************************************
 *
 * Read from RAM and store into an environment variable.
 *
 * (c) 2017 Copyright, Brendan Ta. All rights reserved.
 *
 *******************************************************************************
 */

#include <common.h>
#include <config.h>
#include <command.h>

//--------------------------------------------------------------------------
// Function: do_setenvram
//
// Usage:
// > setenvram.[arg1][arg2] [env_name] [memory_addr] {length}
//
// Example: Memory location 0x1800: 0800
// > setenvram.w return_var 0x1800
// > return_var = 0x8
//
// Example: Memory location 0x1900: 346A
// > setenvram.s return_str 0x1900 0x02
// > return_str = j4
//
// Argument[1] options:
// .b - byte (1 byte).
// .w - word (2 bytes). Host-Network byte converted.
// .l - long (4 bytes). Host-Network byte converted.
// .k - long (4 bytes).
// .s - string (num of bytes specified by user).
//      Note: {length} is required.
//
// Argument[2] options: (If arg2 is not supplied then the return will be in hex)
// ._d - Return in decimal format.
//
// Example: Memory location 0x1800: 0800
// > setenvram.wb return_dec 0x1800
// > return_dec = 8
//
//--------------------------------------------------------------------------

int do_setenvram(
    cmd_tbl_t *cmdtp,
    int flag,
    int argc,
    char *argv[]
) {

    unsigned int *addr;
    unsigned long len;
    uint32_t val;
    char buffer[200];
    char type;
    char returntype;

    if ( argc < 3 || argc > 4 )  {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    addr = (char *)simple_strtol(argv[2], NULL, 16);

    if ( !addr ) {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    if ( argc == 4 ) {
        len = simple_strtol(argv[3], NULL, 16);
        if ( !len || len >= buffer ) {
            printf ("Usage:\n%s\n", cmdtp->usage);
            return 1;
        }
    }

    int option = strlen(argv[0]);

    if ( option > 2 && argv[0][option-3] == '.' ) {
        type = argv[0][option-2];
        returntype = argv[0][option-1];
    } else if ( option > 2 && argv[0][option-2] == '.' ) {
        type = argv[0][option-1];
    } else {
        printf ("Usage:\n%s\n", cmdtp->usage);
        return 1;
    }

    switch ( type ) {
        case 'b':
            val = *((unsigned char *)(addr));
            break;
        case 'w':
            addr = (unsigned short *)simple_strtol(argv[2], NULL, 16);
            val = ntohs(*addr);
            break;
        case 'l':
            addr = (unsigned int *)simple_strtol(argv[2], NULL, 16);
            val = htonl(*addr);
            break;
        case 'k':
            addr = (unsigned int *)simple_strtol(argv[2], NULL, 16);
            val = (*addr);
            break;
        case 's':
            memset(buffer, 0, sizeof(buffer));
            memcpy(buffer, addr, len);
            break;
        default:
            return 0;
    }

    if ( returntype == 'd' ) {
        sprintf(buffer, "%d", val);
    } else if ( type != 's' ) {
        sprintf(buffer, "0x%x", val);
    }

    env_set(argv[1], buffer);
    return 0;
}

/* ------------------------------------------------------------------------- */

U_BOOT_CMD(
    setenvram, 4, 0, do_setenvram,
    "set environment variable from ram",
    "name addr {len}\n"
    "[.b, .w, .l, .s, .k] name address {max_length}\n"
    "    - set environment variable 'name' from memory address 'addr'\n"
    "    - .b=byte,.w=htons word,.l=htonl long,.s=string with length,.k=long\n"
    "    - default in hex format like 0xd, if with .d=decimal got 13\n"
    "    - example: setenvram.b myvar 0x10000000; setenvram.wd myword 0x10000000; setenvram.s mystr 0x10000000 16;\n"
);

