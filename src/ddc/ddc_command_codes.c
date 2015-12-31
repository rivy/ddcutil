/* ddc_command_codes.c
 *
 * Created on: Nov 17, 2015
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

// Direct writes to sysout/syserr: NO

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "util/string_util.h"

#include "ddc/ddc_command_codes.h"


//
// MCCS Command and Response Codes
//

Cmd_Code_Table_Entry cmd_code_table[] = {
      {CMD_VCP_REQUEST          , "VCP Request" },
      {CMD_VCP_RESPONSE         , "VCP Response" },
      {CMD_VCP_SET              , "VCP Set"      },
      {CMD_TIMING_REPLY         , "Timing Reply "},
      {CMD_TIMING_REQUEST       , "Timing Request" },
      {CMD_VCP_RESET            , "VCP Reset" },
      {CMD_SAVE_SETTINGS        , "Save Settings" },
      {CMD_SELF_TEST_REPLY      , "Self Test Reply" },
      {CMD_SELF_TEST_REQUEST    , "Self Test Request" },
      {CMD_ID_REPLY             , "Identification Reply"},
      {CMD_TABLE_READ_REQUST    , "Table Read Request" },
      {CMD_CAPABILITIES_REPLY   , "Capabilities Reply" },
      {CMD_TABLE_READ_REPLY     , "Table Read Reply" },
      {CMD_TABLE_WRITE          , "Table Write" },
      {CMD_ID_REQUEST           , "Identification Request" },
      {CMD_CAPABILITIES_REQUEST , "Capabilities Request" },
      {CMD_ENABLE_APP_REPORT    , "Enable Application Report" }

};
int ddc_cmd_code_count = sizeof(cmd_code_table)/sizeof(Cmd_Code_Table_Entry);


Cmd_Code_Table_Entry * get_ddc_cmd_struct_by_index(int ndx) {
   // DBGMSG("ndx=%d, cmd_code_count=%d  ", ndx, cmd_code_count );
   assert( 0 <= ndx && ndx < ddc_cmd_code_count);
   return &cmd_code_table[ndx];
}

// Commented out as part of removing printf statements from code that
// is part of library.  This function is not currently used.  If needed,
// this function can be passed a message collector of some sort, or
// just implemented in the caller.
//void list_cmd_codes() {
//   printf("DDC command codes:\n");
//   int ndx = 0;
//   for (;ndx < ddc_cmd_code_count; ndx++) {
//      Cmd_Code_Table_Entry entry = cmd_code_table[ndx];
//      printf("  %02x - %-30s\n", entry.cmd_code, entry.name);
//   }
//}


Cmd_Code_Table_Entry * get_ddc_cmd_struct_by_id(Byte cmd_id) {
   // DBGMSG("Starting. id=0x%02x ", id );
   int ndx = 0;
   Cmd_Code_Table_Entry * result = NULL;
   for (;ndx < ddc_cmd_code_count; ndx++) {
      if (cmd_id == cmd_code_table[ndx].cmd_code) {
         result = &cmd_code_table[ndx];
         break;
      }
   }
   // DBGMSG("Done.  ndx=%d. returning %p", ndx, result);
   return result;
}


char * ddc_cmd_code_name(Byte command_id) {
   char * result = NULL;
   Cmd_Code_Table_Entry * cmd_entry = get_ddc_cmd_struct_by_id(command_id);
   if (cmd_entry)
      result = cmd_entry->name;
   else
      result ="unrecognized command";
   return result;
}
