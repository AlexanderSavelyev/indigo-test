/****************************************************************************
 * Copyright (C) 2009-2011 GGA Software Services LLC
 * 
 * This file is part of Indigo toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#include "oracle/bingo_oracle.h"
#include "oracle/ora_wrap.h"
#include "oracle/ora_logger.h"
#include "oracle/mango_oracle.h"

#include "reaction/reaction.h"
#include "reaction/reaction_auto_loader.h"
#include "reaction/rsmiles_saver.h"
#include "reaction/icr_saver.h"
#include "reaction/rxnfile_saver.h"
#include "reaction/reaction_auto_loader.h"
#include "ringo_oracle.h"
#include "molecule/molfile_loader.h"
#include "reaction/rxnfile_loader.h"
#include "layout/reaction_layout.h"
#include "oracle/bingo_oracle_context.h"
#include "molecule/elements.h"

static OCIString * _ringoRSMILES (OracleEnv &env, const Array<char> &target_buf,
                                  BingoOracleContext &context)
{
   QS_DEF(Reaction, target);

   ReactionAutoLoader loader(target_buf);
   loader.treat_x_as_pseudoatom = context.treat_x_as_pseudoatom;
   loader.ignore_closing_bond_direction_mismatch =
           context.ignore_closing_bond_direction_mismatch;
   loader.loadReaction(target);

   QS_DEF(Array<char>, rsmiles);

   ArrayOutput out(rsmiles);

   RSmilesSaver saver(out);

   saver.saveReaction(target);

   OCIString *result = 0;
   env.callOCI(OCIStringAssignText(env.envhp(), env.errhp(), (text *)rsmiles.ptr(),
      rsmiles.size(), &result));

   return result;
}

ORAEXT OCIString *oraRingoRSMILES (OCIExtProcContext *ctx,
                             OCILobLocator *target_locator, short target_indicator,
                             short *return_indicator)
{
   OCIString *result = NULL;

   logger.initIfClosed(log_filename);

   ORABLOCK_BEGIN
   {
      OracleEnv env(ctx, logger);
      BingoOracleContext &context = BingoOracleContext::get(env, 0, false, 0);

      *return_indicator = OCI_IND_NULL;

      if (target_indicator != OCI_IND_NOTNULL)
         throw BingoError("Null reaction given");

      OracleLOB target_lob(env, target_locator);

      QS_DEF(Array<char>, buf);

      target_lob.readAll(buf, false);

      result = _ringoRSMILES(env, buf, context);
      *return_indicator = OCI_IND_NOTNULL;
   }
   ORABLOCK_END

   return result;
}

ORAEXT OCIString * oraRingoCheckReaction (OCIExtProcContext *ctx,
                     OCILobLocator *target_locator, short target_indicator,
                     short *return_indicator)
{
   OCIString *result = NULL;

   logger.initIfClosed(log_filename);

   ORABLOCK_BEGIN
   {
      OracleEnv env(ctx, logger);
      BingoOracleContext &context = BingoOracleContext::get(env, 0, false, 0);

      *return_indicator = OCI_IND_NULL;

      if (target_indicator != OCI_IND_NOTNULL)
      {
         static const char *msg = "null reaction given";

         env.callOCI(OCIStringAssignText(env.envhp(), env.errhp(),
            (text *)msg, strlen(msg), &result));
         *return_indicator = OCI_IND_NOTNULL;
      }
      else
      {
         OracleLOB target_lob(env, target_locator);

         QS_DEF(Array<char>, buf);
         QS_DEF(Reaction, reaction);

         target_lob.readAll(buf, false);

         TRY_READ_TARGET_RXN
         {
            ReactionAutoLoader loader(buf);
            loader.treat_x_as_pseudoatom = context.treat_x_as_pseudoatom;
            loader.ignore_closing_bond_direction_mismatch =
                 context.ignore_closing_bond_direction_mismatch;
            loader.loadReaction(reaction);
            Reaction::checkForConsistency(reaction);
         }
         CATCH_READ_TARGET_RXN(
            OCIStringAssignText(env.envhp(), env.errhp(), (text *)e.message(), strlen(e.message()), &result);
            *return_indicator = OCI_IND_NOTNULL;);

         if (*return_indicator == OCI_IND_NULL)
            // This is needed for Oracle 9. Returning NULL drops the extproc.
            OCIStringAssignText(env.envhp(), env.errhp(), (text *)"nil", 3, &result);
      }
   }
   ORABLOCK_END

   return result;
}

void _ICR (OracleLOB &target_lob, int save_xyz, Array<char> &icr, BingoOracleContext &context)
{
   QS_DEF(Array<char>, target);
   QS_DEF(Reaction, reaction);

   target_lob.readAll(target, false);

   ReactionAutoLoader loader(target);

   loader.treat_x_as_pseudoatom = context.treat_x_as_pseudoatom;
   loader.ignore_closing_bond_direction_mismatch =
                 context.ignore_closing_bond_direction_mismatch;
   loader.loadReaction(reaction);

   if ((save_xyz != 0) && !Reaction::haveCoord(reaction))
      throw BingoError("reaction has no XYZ");

   ArrayOutput output(icr);
   IcrSaver saver(output);

   saver.save_xyz = (save_xyz != 0);
   saver.saveReaction(reaction);
}

ORAEXT OCILobLocator *oraRingoICR (OCIExtProcContext *ctx,
                                   OCILobLocator *target_locator, short target_indicator,
                                   int save_xyz, short *return_indicator)
{
   OCILobLocator *result = 0;

   ORABLOCK_BEGIN
   {
      *return_indicator = OCI_IND_NULL;

      OracleEnv env(ctx, logger);
      BingoOracleContext &context = BingoOracleContext::get(env, 0, false, 0);

      if (target_indicator == OCI_IND_NULL)
         throw BingoError("null molecule given");

      OracleLOB target_lob(env, target_locator);
      QS_DEF(Array<char>, icr);

      _ICR(target_lob, save_xyz, icr, context);

      OracleLOB lob(env);

      lob.createTemporaryBLOB();
      lob.write(0, icr);
      *return_indicator = OCI_IND_NOTNULL;
      lob.doNotDelete();
      result = lob.get();
   }
   ORABLOCK_END

   return result;
}

ORAEXT void oraRingoICR2 (OCIExtProcContext *ctx,
                          OCILobLocator *target_locator, short target_indicator,
                          OCILobLocator *result_locator, short result_indicator,
                          int save_xyz)
{
   ORABLOCK_BEGIN
   {
      OracleEnv env(ctx, logger);
      BingoOracleContext &context = BingoOracleContext::get(env, 0, false, 0);

      if (target_indicator == OCI_IND_NULL)
         throw BingoError("null molecule given");
      if (result_indicator == OCI_IND_NULL)
         throw BingoError("null LOB given");

      OracleLOB target_lob(env, target_locator);
      QS_DEF(Array<char>, icr);

      _ICR(target_lob, save_xyz, icr, context);

      OracleLOB result_lob(env, result_locator);

      result_lob.write(0, icr);
      result_lob.trim(icr.size());
   }
   ORABLOCK_END
}


ORAEXT OCILobLocator *oraRingoRxnfile (OCIExtProcContext *ctx,
                                       OCILobLocator *target_locator, short target_indicator,
                                       short *return_indicator)
{
   OCILobLocator *result = 0;

   ORABLOCK_BEGIN
   {
      *return_indicator = OCI_IND_NULL;

      OracleEnv env(ctx, logger);
      BingoOracleContext &context = BingoOracleContext::get(env, 0, false, 0);

      if (target_indicator == OCI_IND_NULL)
         throw BingoError("null molecule given");

      OracleLOB target_lob(env, target_locator);

      QS_DEF(Array<char>, target);
      QS_DEF(Array<char>, icm);
      QS_DEF(Reaction, reaction);

      target_lob.readAll(target, false);

      ReactionAutoLoader loader(target);

      loader.treat_x_as_pseudoatom = context.treat_x_as_pseudoatom;
      loader.ignore_closing_bond_direction_mismatch =
                 context.ignore_closing_bond_direction_mismatch;
      loader.loadReaction(reaction);

      if (!Reaction::haveCoord(reaction))
      {
         ReactionLayout layout(reaction);

         layout.make();
         reaction.markStereocenterBonds();
      }

      ArrayOutput output(icm);
      RxnfileSaver saver(output);

      saver.saveReaction(reaction);

      OracleLOB lob(env);

      lob.createTemporaryCLOB();
      lob.write(0, icm);
      *return_indicator = OCI_IND_NOTNULL;
      lob.doNotDelete();
      result = lob.get();
   }
   ORABLOCK_END

   return result;
}
