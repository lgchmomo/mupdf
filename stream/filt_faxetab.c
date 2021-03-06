/* Tables for CCITTFaxEncode filter */

#include "filt_faxe.h"

/* Define the end-of-line code. */
const cfe_code cf_run_eol = {1, 12};

/* Define the 1-D code that signals uncompressed data. */
const cfe_code cf1_run_uncompressed = {0xf, 12};

/* Define the 2-D run codes. */
const cfe_code cf2_run_pass = {0x1, 4};
const cfe_code cf2_run_vertical[7] =
{
	{0x3, 7},
	{0x3, 6},
	{0x3, 3},
	{0x1, 1},
	{0x2, 3},
	{0x2, 6},
	{0x2, 7}
};
const cfe_code cf2_run_horizontal = {1, 3};
const cfe_code cf2_run_uncompressed = {0xf, 10};

/* EOL codes for Group 3 2-D. */
const cfe_code cf2_run_eol_1d = { (1 << 1) + 1, 12 + 1};
const cfe_code cf2_run_eol_2d = { (1 << 1) + 0, 12 + 1};

/* White run codes. */
const cf_runs cf_white_runs =
{
	/* Termination codes */
	{
	{0x35, 8}, {0x7, 6}, {0x7, 4}, {0x8, 4},
	{0xb, 4}, {0xc, 4}, {0xe, 4}, {0xf, 4},
	{0x13, 5}, {0x14, 5}, {0x7, 5}, {0x8, 5},
	{0x8, 6}, {0x3, 6}, {0x34, 6}, {0x35, 6},
	{0x2a, 6}, {0x2b, 6}, {0x27, 7}, {0xc, 7},
	{0x8, 7}, {0x17, 7}, {0x3, 7}, {0x4, 7},
	{0x28, 7}, {0x2b, 7}, {0x13, 7}, {0x24, 7},
	{0x18, 7}, {0x2, 8}, {0x3, 8}, {0x1a, 8},
	{0x1b, 8}, {0x12, 8}, {0x13, 8}, {0x14, 8},
	{0x15, 8}, {0x16, 8}, {0x17, 8}, {0x28, 8},
	{0x29, 8}, {0x2a, 8}, {0x2b, 8}, {0x2c, 8},
	{0x2d, 8}, {0x4, 8}, {0x5, 8}, {0xa, 8},
	{0xb, 8}, {0x52, 8}, {0x53, 8}, {0x54, 8},
	{0x55, 8}, {0x24, 8}, {0x25, 8}, {0x58, 8},
	{0x59, 8}, {0x5a, 8}, {0x5b, 8}, {0x4a, 8},
	{0x4b, 8}, {0x32, 8}, {0x33, 8}, {0x34, 8}
	},

	/* Make-up codes */
	{
	{0, 0} /* dummy */ , {0x1b, 5}, {0x12, 5}, {0x17, 6},
	{0x37, 7}, {0x36, 8}, {0x37, 8}, {0x64, 8},
	{0x65, 8}, {0x68, 8}, {0x67, 8}, {0xcc, 9},
	{0xcd, 9}, {0xd2, 9}, {0xd3, 9}, {0xd4, 9},
	{0xd5, 9}, {0xd6, 9}, {0xd7, 9}, {0xd8, 9},
	{0xd9, 9}, {0xda, 9}, {0xdb, 9}, {0x98, 9},
	{0x99, 9}, {0x9a, 9}, {0x18, 6}, {0x9b, 9},
	{0x8, 11}, {0xc, 11}, {0xd, 11}, {0x12, 12},
	{0x13, 12}, {0x14, 12}, {0x15, 12}, {0x16, 12},
	{0x17, 12}, {0x1c, 12}, {0x1d, 12}, {0x1e, 12},
	{0x1f, 12}
	}
};

/* Black run codes. */
const cf_runs cf_black_runs =
{
	/* Termination codes */
	{
	{0x37, 10}, {0x2, 3}, {0x3, 2}, {0x2, 2},
	{0x3, 3}, {0x3, 4}, {0x2, 4}, {0x3, 5},
	{0x5, 6}, {0x4, 6}, {0x4, 7}, {0x5, 7},
	{0x7, 7}, {0x4, 8}, {0x7, 8}, {0x18, 9},
	{0x17, 10}, {0x18, 10}, {0x8, 10}, {0x67, 11},
	{0x68, 11}, {0x6c, 11}, {0x37, 11}, {0x28, 11},
	{0x17, 11}, {0x18, 11}, {0xca, 12}, {0xcb, 12},
	{0xcc, 12}, {0xcd, 12}, {0x68, 12}, {0x69, 12},
	{0x6a, 12}, {0x6b, 12}, {0xd2, 12}, {0xd3, 12},
	{0xd4, 12}, {0xd5, 12}, {0xd6, 12}, {0xd7, 12},
	{0x6c, 12}, {0x6d, 12}, {0xda, 12}, {0xdb, 12},
	{0x54, 12}, {0x55, 12}, {0x56, 12}, {0x57, 12},
	{0x64, 12}, {0x65, 12}, {0x52, 12}, {0x53, 12},
	{0x24, 12}, {0x37, 12}, {0x38, 12}, {0x27, 12},
	{0x28, 12}, {0x58, 12}, {0x59, 12}, {0x2b, 12},
	{0x2c, 12}, {0x5a, 12}, {0x66, 12}, {0x67, 12}
	},

	/* Make-up codes. */
	{
	{0, 0} /* dummy */ , {0xf, 10}, {0xc8, 12}, {0xc9, 12},
	{0x5b, 12}, {0x33, 12}, {0x34, 12}, {0x35, 12},
	{0x6c, 13}, {0x6d, 13}, {0x4a, 13}, {0x4b, 13},
	{0x4c, 13}, {0x4d, 13}, {0x72, 13}, {0x73, 13},
	{0x74, 13}, {0x75, 13}, {0x76, 13}, {0x77, 13},
	{0x52, 13}, {0x53, 13}, {0x54, 13}, {0x55, 13},
	{0x5a, 13}, {0x5b, 13}, {0x64, 13}, {0x65, 13},
	{0x8, 11}, {0xc, 11}, {0xd, 11}, {0x12, 12},
	{0x13, 12}, {0x14, 12}, {0x15, 12}, {0x16, 12},
	{0x17, 12}, {0x1c, 12}, {0x1d, 12}, {0x1e, 12},
	{0x1f, 12}
	}
};

/* Uncompressed codes. */
const cfe_code cf_uncompressed[6] =
{
	{1, 1},
	{1, 2},
	{1, 3},
	{1, 4},
	{1, 5},
	{1, 6}
};

/* Uncompressed exit codes. */
const cfe_code cf_uncompressed_exit[10] =
{
	{2, 8}, {3, 8},
	{2, 9}, {3, 9},
	{2, 10}, {3, 10},
	{2, 11}, {3, 11},
	{2, 12}, {3, 12}
};

/* Some C compilers insist on having executable code in every file.... */
void scfetab_dummy(void) { }

