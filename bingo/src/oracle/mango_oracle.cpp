/****************************************************************************
 * Copyright (C) 2009-2010 GGA Software Services LLC
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

#include "oracle/mango_oracle.h"
#include "base_cpp/output.h"
#include "oracle/mango_shadow_table.h"
#include "oracle/bingo_oracle_context.h"
#include "base_cpp/auto_ptr.h"

const char *bad_molecule_warning = "WARNING: bad molecule: %s\n";
const char *bad_molecule_warning_rowid = "WARNING: bad molecule %s: %s\n";

MangoOracleContext::MangoOracleContext (BingoContext &context) :
MangoContext(context),
shadow_table(context.id),
fingerprints(context.id)
{
}

MangoOracleContext::~MangoOracleContext ()
{
}

BingoOracleContext & MangoOracleContext::context ()
{
   return (BingoOracleContext &)_context;
}

MangoOracleContext & MangoOracleContext::get (OracleEnv &env, int id, bool lock)
{
   TL_GET(PtrArray<MangoContext>, _instances);

   bool config_reloaded;

   BingoOracleContext &context = BingoOracleContext::get(env, id, lock, &config_reloaded);

   MangoContext *already = _get(id, context);
   MangoOracleContext *moc;

   AutoPtr<MangoOracleContext> res;

   if (already == 0)
   {
      res.reset(new MangoOracleContext(context));
      moc = res.get();
      config_reloaded = true;
   }
   else
      moc = (MangoOracleContext *)already;

   if (config_reloaded)
   {
      moc->fingerprints.init(context, context.fp_parameters.fingerprintSize(),
         context.fp_parameters.fingerprintSizeExt(),
         context.fp_parameters.fingerprintSizeExtOrd());

      moc->substructure.treat_x_as_pseudoatom = context.treat_x_as_pseudoatom;
      moc->similarity.treat_x_as_pseudoatom = context.treat_x_as_pseudoatom;
      moc->gross.treat_x_as_pseudoatom = context.treat_x_as_pseudoatom;
      moc->tautomer.treat_x_as_pseudoatom = context.treat_x_as_pseudoatom;
      moc->exact.treat_x_as_pseudoatom = context.treat_x_as_pseudoatom;

      moc->substructure.ignore_closing_bond_direction_mismatch =
              context.ignore_closing_bond_direction_mismatch;
      moc->similarity.ignore_closing_bond_direction_mismatch =
              context.ignore_closing_bond_direction_mismatch;
      moc->gross.ignore_closing_bond_direction_mismatch =
              context.ignore_closing_bond_direction_mismatch;
      moc->tautomer.ignore_closing_bond_direction_mismatch =
              context.ignore_closing_bond_direction_mismatch;
      moc->exact.ignore_closing_bond_direction_mismatch =
              context.ignore_closing_bond_direction_mismatch;

   }

   if (already == 0)
   {
      _instances.add(res.release());
      return *(MangoOracleContext *)_instances.top();
   }
   
   return *moc;
}

