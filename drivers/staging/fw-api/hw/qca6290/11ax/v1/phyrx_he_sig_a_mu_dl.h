/*
 * Copyright (c) 2016-2017 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _PHYRX_HE_SIG_A_MU_DL_H_
#define _PHYRX_HE_SIG_A_MU_DL_H_
#if !defined(__ASSEMBLER__)
#endif

#include "he_sig_a_mu_dl_info.h"

// ################ START SUMMARY #################
//
//	Dword	Fields
//	0-1	struct he_sig_a_mu_dl_info phyrx_he_sig_a_mu_dl_info_details;
//
// ################ END SUMMARY #################

#define NUM_OF_DWORDS_PHYRX_HE_SIG_A_MU_DL 2

struct phyrx_he_sig_a_mu_dl {
    struct            he_sig_a_mu_dl_info                       phyrx_he_sig_a_mu_dl_info_details;
};

/*

struct he_sig_a_mu_dl_info phyrx_he_sig_a_mu_dl_info_details
			
			See detailed description of the STRUCT
*/

#define PHYRX_HE_SIG_A_MU_DL_0_HE_SIG_A_MU_DL_INFO_PHYRX_HE_SIG_A_MU_DL_INFO_DETAILS_OFFSET 0x00000000
#define PHYRX_HE_SIG_A_MU_DL_0_HE_SIG_A_MU_DL_INFO_PHYRX_HE_SIG_A_MU_DL_INFO_DETAILS_LSB 0
#define PHYRX_HE_SIG_A_MU_DL_0_HE_SIG_A_MU_DL_INFO_PHYRX_HE_SIG_A_MU_DL_INFO_DETAILS_MASK 0xffffffff
#define PHYRX_HE_SIG_A_MU_DL_1_HE_SIG_A_MU_DL_INFO_PHYRX_HE_SIG_A_MU_DL_INFO_DETAILS_OFFSET 0x00000004
#define PHYRX_HE_SIG_A_MU_DL_1_HE_SIG_A_MU_DL_INFO_PHYRX_HE_SIG_A_MU_DL_INFO_DETAILS_LSB 0
#define PHYRX_HE_SIG_A_MU_DL_1_HE_SIG_A_MU_DL_INFO_PHYRX_HE_SIG_A_MU_DL_INFO_DETAILS_MASK 0xffffffff


#endif // _PHYRX_HE_SIG_A_MU_DL_H_
