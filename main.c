/* Libraries */
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "converts.h"

/* Constants definition */
#define VALUE_FIELDS    3
#define DELIM           ","

#define NUM_TYPE_PREFIX 2
#define DEC_STR_LEN_MIN 7 
#define DEC_STR_LEN_MAX 13
#define HEX_STR_LEN     8
#define OCT_STR_LEN_MIN 7
#define OCT_STR_LEN_MAX 13
#define BIN_STR_LEN     28

#define ANSI_START_CODE "\x1b["
#define ANSI_END_CODE   "\x1b[0m"

/* Function prototypes */
/* Check type */
c_rgb_t get_type(char* str);
/* Separate str to Red, Green and Blue fields */
char** sep_str_fields(char* str, char* delim, size_t nmemb, size_t memb_size);
/* Insert delim into hexadecimal str to separate red, green and blue values */
char* strinsrt(char* str, char c, size_t index);
/* Display the actual color in your terminal */
void dpy_color(uint8_t* rgb_cols, char* str);
/* Free the rgb color char arrays */
void free_cols_vals(char** cols_vals);
/* Count the number of times a char c is in string str */
int strcchar(const char* str, char c);
/* Usage function */
void usage(char* prg_name);

/* Main */
int 
main(int argc, char* argv[]) {
    rgb_val rgb_col_stuff;
    /* Allocating buffer for the char array for converted number */
    char buffer[8];
    /* Checks if right number of arguments is passed to the program */
    if (argc >= 3 && argv[1][0] == '-' && argv[2]) {
        char* rgb_str = strdup(argv[2]);
        /* Get an array of uint8_t [r,g,b] values from the user */
        rgb_col_stuff = get_rgb_val(rgb_str);
        if (rgb_col_stuff == NULL) {
            free(rgb_str);
            fprintf(stderr, "Invalid format passed as rgb value: %s\n", argv[2]);
            exit(EXIT_FAILURE);
        }
        /* Free the duplicated string */
        free(rgb_str);
        /* Switch on the second character in the flags */
        switch (argv[1][1]) {
            case 'd':
                /* Print decimal representation of rgb value provided by user */
                printf("0d%d,%d,%d\n", rgb_col_stuff[0], rgb_col_stuff[1],
                       rgb_col_stuff[2]);
                break;
            case 'b':
                /* Print binary representation of rgb value */
                printf("0b%08b,%08b,%08b\n", rgb_col_stuff[0], rgb_col_stuff[1], rgb_col_stuff[2]);
                break;
            case 'x':
                printf("0x");
                /* Get to buffer the hexadecimal representation of rgb values without 0x prefix */
                snprintf(buffer, sizeof(buffer), "%02x%02x%02x", rgb_col_stuff[0], rgb_col_stuff[1],
                       rgb_col_stuff[2]);
                /* Convert values to upper */
                for (size_t i = 0; i < strlen(buffer); ++i) {
                    buffer[i] = (char)toupper(buffer[i]);
                }
                /* Print rgb values */
                printf("%s\n", buffer);
                break;
            case 'o':
                /* Print octal representation of rgb values */
                printf("0o%o,%o,%o\n", rgb_col_stuff[0], rgb_col_stuff[1],
                        rgb_col_stuff[2]);
                break;
            case 'h':
                /* Print usage and exit */
                usage(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            default:
                /* Print error and exit with EXIT_FAILURE */
                fprintf(stderr, "Unknown option %s", argv[1]);
                exit(EXIT_FAILURE);
                break;
        }
        /* Checks if user asked for preview to be displayed and displays it */
        if (argc >= 5 && !strncmp(argv[3], "-p", 2)) {
            printf("\n");
            dpy_color(rgb_col_stuff, argv[4]);
        }
    } else {
        /* Print usage if no or incorrect number of options are provided */
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    /* Exits the program */
    return EXIT_SUCCESS;
}

rgb_val 
get_rgb_val(char* str) {
    /* Allocate memory for 2d array of strings to be converted to desired type */
    char** s_strs = malloc(VALUE_FIELDS * sizeof(char*));
    rgb_val d_rgb = malloc(VALUE_FIELDS * sizeof(uint8_t));
    /* Allocate memory for each row in 2d array of size 8 bytes */
    for (int i = 0; i < VALUE_FIELDS; ++i) {
        s_strs[i] = malloc(8 * sizeof(char));
    }
    /* Get type of passed rgb value from user and checks if it's valid */
    c_rgb_t type = get_type(str);
    if (type == ERROR) { 
        /* If not than free allocated items and return NULL */
        free_cols_vals(s_strs);
        free(d_rgb);
        return NULL; 
    }
    /* In the correct format user inputs string of chars with prefix of specified type 
     * like this '0d123,14,45' so we need to move the pointer to this strings by 2 to 
     * get '123,14,45'
     */
    str += 2;
    if (type == HEXADECIMAL) {
        /* Checks if parsed string is of type HEXADECIMAL and adds separators to separate 
         * individual R,G,B values and returns 2d array of them and converts any type
         * to decimal for easier convertion 
         */
        char* hex_str = strinsrt(str, *DELIM, 2);
        char* hex_str2 = strinsrt(hex_str, *DELIM, 5);
        s_strs = sep_str_fields(hex_str2, DELIM, VALUE_FIELDS, 8);
        for (int x = 0; x < VALUE_FIELDS; ++x) {
            d_rgb[x] = (col_val)any_to_dec(s_strs[x], type);
        }
        /* Free hex_strs */
        free(hex_str);
        free(hex_str2);
    } else {
        /* For any other type just separate the values and convert them to decimal */
        s_strs = sep_str_fields(str, DELIM, VALUE_FIELDS, 8);
        for (int x = 0; x < VALUE_FIELDS; ++x) {
            d_rgb[x] = (col_val)any_to_dec(s_strs[x], type);
        }
    }
    /* Free the 2d array and return pointer to uint8_t*/
    free_cols_vals(s_strs);
    return d_rgb;
}

char*
strinsrt(char* str, char c, size_t index) {
    /* Lenght of str */
    size_t len = strlen(str);
    /* New lenght (1 for char c, 1 for null terminator '\0') */
    size_t nlen = len + 1 + 1;
    /* Allocation */
    char* nstr = malloc(nlen * sizeof(char));
    if (nstr == NULL) {
        return NULL;
    }
    /* Checks if specified index is less than lenght of the orginal string */
    if (index < len) {
        /* Copies original string to the new string */
        memcpy(nstr, str, len);
        /* Move 1 character at a time to right by 1 until under the pointer to the
         * string is char c
         */
        for (size_t i = len; i > index-1; --i) {
            memmove(&nstr[i], &nstr[i-1], 1);
        }
        /* Set char c and null character to terminate the string */
        memset(&nstr[nlen - 1], '\0', 1);
        memset(&nstr[index], c, 1);
        return nstr;
    }
    /* Free the new string if index is bigger then the lenght of original string */
    free(nstr);
    return str;
}

void 
dpy_color(rgb_val rgb_cols, char* str) {
    /* Checks if program is running in terminal */
    if (!isatty(STDOUT_FILENO)) {
        fprintf(stderr, "You must run the program inside of terminal!\n");
        exit(EXIT_FAILURE);
    }
    /* Print foreground and background color preview using ANSI escape codes */
    printf("fg: ");
    printf(ANSI_START_CODE "38;2;%d;%d;%dm", rgb_cols[0], rgb_cols[1], rgb_cols[2]);
    printf(" %s ", str);
    printf(ANSI_END_CODE);
    printf(" bg: ");
    printf(ANSI_START_CODE "30m");
    printf(ANSI_START_CODE "48;2;%d;%d;%dm", rgb_cols[0], rgb_cols[1], rgb_cols[2]);
    printf(" %s ", str);
    printf(ANSI_END_CODE "\n");
}

/* Separtate string into fields by specified delim */
char**
sep_str_fields(char* str, char* delim, size_t nmemb, size_t memb_size) {
    char** str_fields = malloc(nmemb * sizeof(char*));
    for (size_t i = 0; i < nmemb; ++i) {
        str_fields[i] = malloc(memb_size * sizeof(char));
    }
    char* token = strtok(str, delim);
    size_t n = 0;
    while (token != NULL) {
        str_fields[n] = strdup(token);
        ++n;
        token = strtok(NULL, delim);
    }
    return str_fields;
}

/* Count the number of times char c is found in str */
int     
strcchar(const char* str, char c) {
    int count = 0;
    while (*str != '\0') {
        if (*str == c) {
            count++;
        }
        str++;
    }
    return count;
}

/* Return type of parsed rgb value */
c_rgb_t 
get_type(char* str) {
    size_t len = strlen(str);
    if (!strncmp(str, "0x", NUM_TYPE_PREFIX) && len == HEX_STR_LEN) {   /* len 10 -> 0x1424fe */
        for (size_t i = NUM_TYPE_PREFIX; i < len; ++i) {
            if (!(str[i] >= '0' && str[i] <= '9') && 
                !(str[i] >= 'a' && str[i] <= 'f') &&
                !(str[i] >= 'A' && str[i] <= 'F')) {
                return ERROR;
            }
        }
        return HEXADECIMAL;
    } else if (!strncmp(str, "0o", NUM_TYPE_PREFIX) && len >= OCT_STR_LEN_MIN && len <= OCT_STR_LEN_MAX) {  /* len =7 - =13 -> 0o1,3,4 - 0o102,104,203 */
        goto check_delim_count;
        if (!(any_to_dec(str += NUM_TYPE_PREFIX, OCTAL) >= 0 && 
            any_to_dec(str += NUM_TYPE_PREFIX, OCTAL) <= 255)) {
            return ERROR;
        }
        return OCTAL;
    } else if (!strncmp(str, "0d", NUM_TYPE_PREFIX) && len >= DEC_STR_LEN_MIN && len <= DEC_STR_LEN_MAX) {   /*  len =7 - =12 -> 0d1,2,3 - 0d123,230,124 */
        goto check_delim_count;
        if (!(any_to_dec(str += NUM_TYPE_PREFIX, DECIMAL) >= 0 && 
            any_to_dec(str += NUM_TYPE_PREFIX, DECIMAL) <= 255)) {
            return ERROR;
        }
        return DECIMAL;
    } else if (!strncmp(str, "0b", NUM_TYPE_PREFIX) && len == BIN_STR_LEN) { /* len == 28 -> 0b01011010,10110101,10100101*/
        goto check_delim_count;
        if (!(any_to_dec(str += NUM_TYPE_PREFIX, BINARY) >= 0 && 
            any_to_dec(str += NUM_TYPE_PREFIX, BINARY) <= 255)) {
            return ERROR;
        }
        return BINARY;
    }

    check_delim_count:
        if (strcchar(str, *DELIM) != 3) {
            return ERROR;
        }
    return ERROR;
}

void 
free_cols_vals(char** cols_vals) {
    /* Free individual rows */
    for (int i = 0; i < VALUE_FIELDS; ++i) {
        free(cols_vals[i]);
    }
    free(cols_vals);
}

/* Print usage */
void
usage(char* prg_name) {
    fprintf(stdout, "usage: %s [OPTIONS] <rgb_value> (-p) <str>\n", prg_name);
    fprintf(stdout, "OPTIONS specify to which type of numbering system you want to convert the rgb_value\n\n");
    fprintf(stdout, "OPTIONS:\n");
    fprintf(stdout, "\t-h\tprint this message\n");
    fprintf(stdout, "\t-d\tto decimal numbering system (base 10)\n");
    fprintf(stdout, "\t-x\tto hexadeciam numbering system (base 16)\n");
    fprintf(stdout, "\t-o\tto octal numbering system (base 8)\n");
    fprintf(stdout, "\t-b\tto binary numbering system (base 2)\n\n");
    fprintf(stdout, "-p option is optional\n");
    fprintf(stdout, "\t-p <str>\twill print the <str> in the color you've passed earlier\n\n");
    fprintf(stdout, "Correct format of <rgb_value>:\n");
    fprintf(stdout, "for -d -> 0dR_value%sG_value%sB_value (all values must be from 0 to 255)\n", DELIM, DELIM);
    fprintf(stdout, "for -x -> 0xR_value%sG_value%sB_value (all values must be from 00 to ff/FF)\n", DELIM, DELIM);
    fprintf(stdout, "for -o -> 0oR_value%sG_value%sB_value (all values must be from 0 to 377)\n", DELIM, DELIM);
    fprintf(stdout, "for -b -> 0bR_value%sG_value%sB_value (all values must be 00000000 to 11111111)\n", DELIM, DELIM);
}	
