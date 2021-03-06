/* ddc_vcp.c
 *
 * Virtual Control Panel access
 *
 * <copyright>
 * Copyright (C) 2014-2017 Sanford Rockowitz <rockowitz@minsoft.com>
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

/** \file
 * Virtual Control Panel access
 */

#include <config.h>

/** \cond */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/** \endcond */

#include "util/report_util.h"

#include "base/ddc_errno.h"
#include "base/ddc_packets.h"
#include "base/displays.h"
#include "base/retry_history.h"
#include "base/status_code_mgt.h"

#include "i2c/i2c_bus_core.h"

#include "adl/adl_shim.h"

#ifdef USE_USB
#include "usb/usb_displays.h"
#include "usb/usb_vcp.h"
#endif

#include "vcp/vcp_feature_codes.h"

#include "ddc/ddc_multi_part_io.h"
#include "ddc/ddc_packet_io.h"
#include "ddc/ddc_vcp_version.h"

#include "ddc/ddc_vcp.h"


// Trace class for this file
static Trace_Group TRACE_GROUP = TRC_DDC;


//
//  Save Control Settings
//

/** Executes the DDC Save Control Settings command.
 *
 * \param  dh handle of open display device
 * \param  retry_history  if non-null, logs retryable errors
 * \return status code, as returned by #ddc_write_only_with_retry()
 */
Public_Status_Code
save_current_settings(
      Display_Handle * dh,
      Retry_History *  retry_history)
{
   bool debug = false;
   DBGTRC(debug, TRACE_GROUP,
          "Invoking DDC Save Current Settings command. dh=%s", dh_repr_t(dh));
   Public_Status_Code psc = 0;

   if (dh->dref->io_mode == DDCA_IO_USB) {
      // command line parser should block this case
      PROGRAM_LOGIC_ERROR("MCCS over USB does not have Save Current Settings command");
   }
   else {
      DDC_Packet * request_packet_ptr =
         create_ddc_save_settings_request_packet("save_current_settings:request packet");
      // DBGMSG("create_ddc_save_settings_request_packet returned packet_ptr=%p", request_packet_ptr);
      // dump_packet(request_packet_ptr);

      psc = ddc_write_only_with_retry(dh, request_packet_ptr, retry_history);

      if (request_packet_ptr)
         free_ddc_packet(request_packet_ptr);
   }

   DBGTRC(debug, TRACE_GROUP, "Returning %s", psc_desc(psc));
   if (psc == DDCRC_RETRIES && retry_history && (debug||IS_TRACING()))
      DBGMSG("          Try errors: %s", retry_history_string(retry_history));
   return psc;
}


//
// Set VCP feature value
//

/** Sets a non-table VCP feature value.
 *
 *  \param  dh            display handle for open display
 *  \param  feature_code  VCP feature code
 *  \param  new_value     new value
 *  \param  retry_history if non-null, logs retryable errors
 *  \return status code from #ddc_write_only_with_retry()
 */
Public_Status_Code
set_nontable_vcp_value(
      Display_Handle * dh,
      Byte             feature_code,
      int              new_value,
      Retry_History *  retry_history)
{
   bool debug = false;
   DBGTRC(debug, TRACE_GROUP,
          "Writing feature 0x%02x , new value = %d, dh=%s, retry_history=%p",
          feature_code, new_value, dh_repr_t(dh), retry_history);
   Public_Status_Code psc = 0;

   if (dh->dref->io_mode == DDCA_IO_USB) {
#ifdef USE_USB
      psc = usb_set_nontable_vcp_value(dh, feature_code, new_value);
#else
      PROGRAM_LOGIC_ERROR("ddcutil not built with USB support");
#endif
   }
   else {
      DDC_Packet * request_packet_ptr =
         create_ddc_setvcp_request_packet(feature_code, new_value, "set_vcp:request packet");
      // DBGMSG("create_ddc_getvcp_request_packet returned packet_ptr=%p", request_packet_ptr);
      // dump_packet(request_packet_ptr);

      psc = ddc_write_only_with_retry(dh, request_packet_ptr, retry_history);

      if (request_packet_ptr)
         free_ddc_packet(request_packet_ptr);
   }

   DBGTRC(debug, TRACE_GROUP, "Returning %s", psc_desc(psc));
   if (psc == DDCRC_RETRIES && retry_history && (debug || IS_TRACING()))
      DBGMSG("          Try errors: %s", retry_history_string(retry_history));
   return psc;
}


/** Sets a table VCP feature value.
 *
 *  \param  dh            display handle for open display
 *  \param  feature_code  VCP feature code
 *  \param  bytes         pointer to table bytes
 *  \param  bytect        number of bytes
 *  \param  retry_history if non-null, collects retryable errors
 *  \return status code   normally as from ##multi_part_write_with_retry()
 *                        DDCL_UNIMPLEMENTED) if io mode is USB
 */
Public_Status_Code
set_table_vcp_value(
      Display_Handle *  dh,
      Byte              feature_code,
      Byte *            bytes,
      int               bytect,
      Retry_History *   retry_history)
{
   bool debug = false;
   DBGTRC(debug, TRACE_GROUP, "Writing feature 0x%02x , bytect = %d, retry_history=%p",
                              feature_code, bytect, retry_history);
   Public_Status_Code psc = 0;

   if (dh->dref->io_mode == DDCA_IO_USB) {
#ifdef USE_USB
      psc = DDCL_UNIMPLEMENTED;
#else
      PROGRAM_LOGIC_ERROR("ddcutil not built with USB support");
#endif
   }
   else {
      // TODO: clean up function signatures
      // pointless wrapping in a Buffer just to unwrap
      Buffer * new_value = buffer_new_with_value(bytes, bytect, __func__);

      psc = multi_part_write_with_retry(dh, feature_code, new_value, retry_history);

      buffer_free(new_value, __func__);
   }

   DBGTRC(debug, TRACE_GROUP, "Returning: %s", psc_desc(psc));
   if ( (debug || IS_TRACING()) && psc == DDCRC_RETRIES && retry_history)
      DBGMSG("      Try errors: %s", retry_history_string(retry_history));
   return psc;
}



static bool verify_setvcp = false;

/** Sets the setvcp verification setting.
 *
 *  If enabled, setvcp will read the feature value from the monitor after
 *  writing it, to ensure the monitor has actually changed the feature value.
 *
 *  \param onoff  **true** for enabled, **false** for disabled.
 */
void set_verify_setvcp(bool onoff) {
   bool debug = false;
   DBGMSF(debug, "Setting verify_setvcp = %s", bool_repr(onoff));
   verify_setvcp = onoff;
}


/** Gets the current setvcp verification setting.
 *
 *  \return **true** if setvcp verification enabled\n
 *          **false** if not
 */
bool get_verify_setvcp() {
   return verify_setvcp;
}


static bool
is_rereadable_feature(
      Display_Handle * dh,
      DDCA_Vcp_Feature_Code opcode)
{
   bool debug = false;
   DBGMSF(debug, "Starting opcode = 0x%02x", opcode);
   bool result = false;

   // readable features that should not be read after write
   DDCA_Vcp_Feature_Code unrereadable_features[] = {
         0x02,        // new control value
         0x03,        // soft controls
         0x60,        // input source ???
   };

   VCP_Feature_Table_Entry * vfte = vcp_find_feature_by_hexid(opcode);
   DBGMSF(debug, "vfte=%p", vfte);
   if (vfte) {
      assert(opcode < 0xe0);
      DDCA_MCCS_Version_Spec vspec = get_vcp_version_by_display_handle(dh);  // ensure dh->vcp_version set
      DBGMSF(debug, "vspec = %d.%d", vspec.major, vspec.minor);
      // hack, make a guess
      if ( vcp_version_eq(vspec, VCP_SPEC_UNKNOWN)   ||
           vcp_version_eq(vspec, VCP_SPEC_UNQUERIED ))
         vspec = VCP_SPEC_V22;

      // if ( !vcp_version_eq(vspec, VCP_SPEC_UNKNOWN) &&
      //      !vcp_version_eq(vspec, VCP_SPEC_UNQUERIED ))
      // {
         result = is_feature_readable_by_vcp_version(vfte, vspec);
         DBGMSF(debug, "vspec=%d.%d, readable feature = %s", vspec.major, vspec.minor, bool_repr(result));
      // }
   }
   if (result) {
      for (int ndx = 0; ndx < ARRAY_SIZE(unrereadable_features); ndx++) {
         if ( unrereadable_features[ndx] == opcode ) {
            result = false;
            DBGMSF(debug, "Unreadable opcode");
            break;
         }
      }
   }

   DBGMSF(debug, "Returning: %s", bool_repr(result));
   return result;
}


static bool
single_vcp_value_equal(
      DDCA_Single_Vcp_Value * vrec1,
      DDCA_Single_Vcp_Value * vrec2)
{
   bool debug = false;

   bool result = false;
   if (vrec1->opcode     == vrec2->opcode &&
       vrec1->value_type == vrec2->value_type)
   {
      switch(vrec1->value_type) {
      case(DDCA_NON_TABLE_VCP_VALUE):
            // only check SL byte which would be set for any VCP, monitor
            result = (vrec1->val.nc.sl == vrec2->val.nc.sl);
            break;
      case(DDCA_TABLE_VCP_VALUE):
            result = (vrec1->val.t.bytect == vrec2->val.t.bytect) &&
                     (memcmp(vrec1->val.t.bytes, vrec2->val.t.bytes, vrec1->val.t.bytect) == 0 );
      }
   }
   DBGMSF(debug, "Returning: %s", bool_repr(result));
   return result;
}



// TODO: Consider wrapping set_vcp_value() in set_vcp_value_with_retry(), which would
// retry in case verification fails

/** Sets a VCP feature value.
 *
 *  \param  dh            display handle for open display
 *  \param  vrec          pointer to value record
 *  \param  retry_history if non-null, collects retryable errors
 *  \return status code
 *
 *  If write verification is turned on, reads the feature value after writing it
 *  to ensure the display has actually changed the value.
 */
Public_Status_Code
set_vcp_value(
      Display_Handle *        dh,
      DDCA_Single_Vcp_Value * vrec,
      Retry_History *         retry_history)
{
   bool debug = false;
   DBGMSF(debug, "Starting. retry_history=%p", retry_history);
   FILE * fout = FOUT;
   if ( get_output_level() < DDCA_OL_VERBOSE )
      fout = NULL;

   Public_Status_Code psc = 0;
   if (vrec->value_type == DDCA_NON_TABLE_VCP_VALUE) {
      psc = set_nontable_vcp_value(dh, vrec->opcode, vrec->val.c.cur_val, retry_history);
   }
   else {
      assert(vrec->value_type == DDCA_TABLE_VCP_VALUE);
      psc = set_table_vcp_value(dh, vrec->opcode, vrec->val.t.bytes, vrec->val.t.bytect, retry_history);
   }

   if (psc == 0 && verify_setvcp) {
      if (is_rereadable_feature(dh, vrec->opcode) ) {
         f0printf(fout, "Verifying that value of feature 0x%02x successfully set...\n", vrec->opcode);
         DDCA_Single_Vcp_Value * newval = NULL;
         retry_history_clear(retry_history);
         psc = get_vcp_value(
             dh,
             vrec->opcode,
             vrec->value_type,
             &newval,
             retry_history);
         if (psc != 0) {
            f0printf(fout, "(%s) Read after write failed. get_vcp_value() returned: %s\n",
                           __func__, psc_desc(psc));
            if (psc == DDCRC_RETRIES && retry_history)
               f0printf(fout, "(%s)    Try errors: %s\n", __func__, retry_history_string(retry_history));
            // psc = DDCRC_VERIFY;
         }
         else {
            if (! single_vcp_value_equal(vrec,newval)) {
               psc = DDCRC_VERIFY;
               f0printf(fout, "Current value does not match value set.\n");
            }
            else {
               f0printf(fout, "Verification succeeded\n");
            }
         }
      }
      else {
         f0printf(fout, "Feature 0x%02x does not support verification\n", vrec->opcode);
         // rpt_vstring(0, "Feature 0x%02x does not support verification", vrec->opcode);
      }
   }

   DBGMSF(debug, "Returning: %s", psc_desc(psc));
   return psc;
}


//
// Get VCP values
//

/** Gets the value for a non-table feature.
 *
 *  \param  dh                 handle for open display
 *  \param  feature_code       VCP feature code
 *  \param  ppInterpretedCode  where to return parsed response
 *  \param  retry_history      if non-null, records status code for retries
 *  \return status code
 *
 * It is the responsibility of the caller to free the parsed response.
 *
 * The value pointed to by ppInterpretedCode is non-null iff the returned status code is 0.
 */
Public_Status_Code get_nontable_vcp_value(
       Display_Handle *               dh,
       DDCA_Vcp_Feature_Code          feature_code,
       Parsed_Nontable_Vcp_Response** ppInterpretedCode,
       Retry_History *                retry_history)
{
   bool debug = false;
   DBGTRC(debug, TRACE_GROUP, "Reading feature 0x%02x", feature_code);

   Public_Status_Code psc = 0;
   Parsed_Nontable_Vcp_Response * parsed_response = NULL;

   DDC_Packet * request_packet_ptr  = NULL;
   DDC_Packet * response_packet_ptr = NULL;
   request_packet_ptr = create_ddc_getvcp_request_packet(
                           feature_code, "get_vcp_by_DisplayRef:request packet");
   // dump_packet(request_packet_ptr);

   Byte expected_response_type = DDC_PACKET_TYPE_QUERY_VCP_RESPONSE;
   Byte expected_subtype = feature_code;
   int max_read_bytes  = 20;    // actually 3 + 8 + 1, or is it 2 + 8 + 1?

   // retry:
   psc = ddc_write_read_with_retry(
           dh,
           request_packet_ptr,
           max_read_bytes,
           expected_response_type,
           expected_subtype,
           false,                       // all_zero_response_ok
       //  retry_null_response,
           &response_packet_ptr,
           retry_history                        // Retry_History
        );
   // TRCMSGTG(tg, "perform_ddc_write_read_with_retry() returned %s", psc_desc(psc));
   if (debug || IS_TRACING() ) {
      if (psc != 0)
         DBGMSG("perform_ddc_write_read_with_retry() returned %s, reponse_packet_ptr=%p", psc_desc(psc), response_packet_ptr);
   }

   if (psc == 0) {
      // dump_packet(response_packet_ptr);
      psc = get_interpreted_vcp_code(response_packet_ptr, true /* make_copy */, &parsed_response);   // ???
      if (psc == 0) {
#ifdef NO_LONGER_NEEDED
         if (parsed_response->vcp_code != feature_code) {
            DBGMSG("!!! WTF! requested feature_code = 0x%02x, but code in response is 0x%02x",
                   feature_code, parsed_response->vcp_code);
            call_tuned_sleep_i2c(SE_POST_READ);
            goto retry;
         }
#endif

         if (!parsed_response->valid_response)  {
            psc = DDCRC_INVALID_DATA;
         }
         else if (!parsed_response->supported_opcode) {
            psc = DDCRC_REPORTED_UNSUPPORTED;
         }
         if (psc != 0) {
            free(parsed_response);
            parsed_response = NULL;
         }
      }
   }

   if (request_packet_ptr)
      free_ddc_packet(request_packet_ptr);
   if (response_packet_ptr)
      free_ddc_packet(response_packet_ptr);

   DBGTRC(debug, TRACE_GROUP,
          "Returning %s, *ppinterpreted_code=%p", psc_desc(psc), parsed_response);
   *ppInterpretedCode = parsed_response;
   assert( (psc == 0 && parsed_response) || (psc < 0 && !parsed_response));
   return psc;
}


/** Gets the value of a table feature in a newly allocated Buffer struct.
 *  It is the responsibility of the caller to free the Buffer.
 *
 *  \param  dh              display handle
 *  \param  feature_code    VCP feature code
 *  \param  pp_table_bytes  location at which to save address of newly allocated Buffer
 *  \param  retry_history   if non-null, collects retry status codes
 *  \return status code
 */
Public_Status_Code get_table_vcp_value(
       Display_Handle *       dh,
       Byte                   feature_code,
       Buffer**               pp_table_bytes,
       Retry_History *        retry_history)
{
   bool debug = false;
   DBGTRC(debug, TRACE_GROUP, "Starting. Reading feature 0x%02x", feature_code);

   Public_Status_Code psc = 0;
   DDCA_Output_Level output_level = get_output_level();
   Buffer * paccumulator =  NULL;

   psc = multi_part_read_with_retry(
            dh,
            DDC_PACKET_TYPE_TABLE_READ_REQUEST,
            feature_code,
            true,                      // all_zero_response_ok
            &paccumulator,
            retry_history);
   if (debug || psc != 0) {
      DBGTRC(debug, TRACE_GROUP,
             "perform_ddc_write_read_with_retry() returned %s", psc_desc(psc));
   }

   if (psc == 0) {
      *pp_table_bytes = paccumulator;
      if (output_level >= DDCA_OL_VERBOSE) {
         printf("Bytes returned on table read:");
         buffer_dump(paccumulator);
      }
   }

   DBGTRC(debug, TRACE_GROUP,
          "Done. Returning rc=%s, *pp_table_bytes=%p", psc_desc(psc), *pp_table_bytes);
   // if (psc == DDCRC_RETRIES && retry_history && (debug || IS_TRACING()) )
   //       DBGMSG("    Try errors: %s", retry_history_string(retry_history));
   DBGTRC_RETRY_ERRORS(debug, psc, retry_history);
   return psc;
}


/** Gets the value of a VCP feature.
 *
 * \param  dh              handle for open display
 * \param  feature_code    feature code id
 * \param  call_type       indicates whether table or non-table
 * \param  pvalrec         location where to return newly allocated #DDCA_Single_Vcp_Value
 * \param  retry_history   if non-null, collects retryable errors
 * \return status code
 *
 * The value pointed to by pvalrec is non-null iff the returned status code is 0.
 *
 * The caller is responsible for freeing the value pointed returned at pvalrec.
 */
Public_Status_Code
get_vcp_value(
       Display_Handle *          dh,
       Byte                      feature_code,
       DDCA_Vcp_Value_Type       call_type,
       DDCA_Single_Vcp_Value **  pvalrec,
       Retry_History *           retry_history)
{
   bool debug = false;
   DBGTRC(debug, TRACE_GROUP, "Starting. Reading feature 0x%02x, dh=%s, dh->fh=%d",
            feature_code, dh_repr_t(dh), dh->fh);

   Public_Status_Code psc = 0;
   Buffer * buffer = NULL;
   Parsed_Nontable_Vcp_Response * parsed_nontable_response = NULL;  // vs interpreted ..
   DDCA_Single_Vcp_Value * valrec = NULL;

   // why are we coming here for USB?
   if (dh->dref->io_mode == DDCA_IO_USB) {
#ifdef USE_USB
      DBGMSF(debug, "USB case");
      if (retry_history)
         retry_history_clear(retry_history);

      switch (call_type) {

          case (DDCA_NON_TABLE_VCP_VALUE):
                psc = usb_get_nontable_vcp_value(
                      dh,
                      feature_code,
                      &parsed_nontable_response);    //
                if (psc == 0) {
                   valrec = create_nontable_vcp_value(
                               feature_code,
                               parsed_nontable_response->mh,
                               parsed_nontable_response->ml,
                               parsed_nontable_response->sh,
                               parsed_nontable_response->sl);
                   free(parsed_nontable_response);
                }
                break;

          case (DDCA_TABLE_VCP_VALUE):
                psc = DDCL_UNIMPLEMENTED;
                break;
          }
#else
      PROGRAM_LOGIC_ERROR("ddcutil not built with USB support");
#endif
   }
   else {
      switch (call_type) {

      case (DDCA_NON_TABLE_VCP_VALUE):
            psc = get_nontable_vcp_value(
                     dh,
                     feature_code,
                     &parsed_nontable_response,
                     retry_history);
            if (psc == 0) {
               valrec = create_nontable_vcp_value(
                           feature_code,
                           parsed_nontable_response->mh,
                           parsed_nontable_response->ml,
                           parsed_nontable_response->sh,
                           parsed_nontable_response->sl);
               free(parsed_nontable_response);
            }
            break;

      case (DDCA_TABLE_VCP_VALUE):
            psc = get_table_vcp_value(
                    dh,
                    feature_code,
                    &buffer,
                    retry_history);
            if (psc == 0) {
               valrec = create_table_vcp_value_by_buffer(feature_code, buffer);
               buffer_free(buffer, __func__);
            }
            break;
      }

   } // non USB

   *pvalrec = valrec;

   DBGTRC(debug, TRACE_GROUP, "Done. Returning: %s", psc_desc(psc) );
   if (psc == 0 && debug)
      report_single_vcp_value(valrec,1);

   if (psc == DDCRC_RETRIES && retry_history && ( debug || IS_TRACING())) {
      // retry_history_dump(retry_history);
      DBGTRC(debug, TRACE_GROUP, "      Try errors: %s", retry_history_string(retry_history));
   }



   assert( (psc == 0 && *pvalrec) || (psc != 0 && !*pvalrec) );
   return psc;
}
