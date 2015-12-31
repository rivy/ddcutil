/* ddc_multi_part_io.h
 *
 * Created on: Jun 11, 2014
 *     Author: rock
 *
 * <copyright>
 * Copyright (C) 2014-2015 Sanford Rockowitz <rockowitz@minsoft.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * </endcopyright>
 */

#ifndef DDC_MULTI_PART_IO_H_
#define DDC_MULTI_PART_IO_H_

#include <stdbool.h>

#include "util/coredefs.h"
#include "util/data_structures.h"

#include "base/status_code_mgt.h"
#include "base/displays.h"


// Statistics
void ddc_reset_multi_part_read_stats();
void ddc_report_multi_part_read_stats();

// Retry management
void ddc_set_max_multi_part_read_tries(int ct);
int ddc_get_max_multi_part_read_tries();


Global_Status_Code
multi_part_read_with_retry(
   Display_Handle * dh,
   Byte             request_type,
   Byte             request_subtype,   // VCP feature code for table read, ignore for capabilities
   Buffer**         ppbuffer);

#endif /* DDC_MULTI_PART_IO_H_ */
