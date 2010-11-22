/****************************************************************************
 * Copyright (C) 2010 GGA Software Services LLC
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

#include "indigo_internal.h"
#include "base_cpp/scanner.h"
#include "base_cpp/output.h"
#include "layout/molecule_layout.h"
#include "molecule/molecule.h"
#include "molecule/molfile_loader.h"
#include "molecule/sdf_loader.h"
#include "molecule/molfile_saver.h"
#include "molecule/molecule_auto_loader.h"
#include "reaction/rxnfile_loader.h"
#include "reaction/rxnfile_saver.h"
#include "reaction/reaction_auto_loader.h"
#include "reaction/reaction_product_enumerator.h"

struct ProductEnumeratorCallbackData 
{
   ReactionProductEnumerator *rpe;
   ObjArray<Reaction> *out_reactions;
};

static void product_proc( Molecule &product, Array<int> &monomers_indices, void *userdata )
{
   ProductEnumeratorCallbackData *rpe_data = (ProductEnumeratorCallbackData *)userdata;

   Reaction &reaction = rpe_data->out_reactions->push();

   QS_DEF(Molecule, new_product);
   new_product.clear();
   new_product.clone(product, NULL, NULL);   

   MoleculeLayout mol_layout(new_product);
   mol_layout.respect_existing_layout = false;
   mol_layout.make();
   new_product.stereocenters.markBonds();

   reaction.clear();

   for (int i = 0; i < monomers_indices.size(); i++)
      reaction.addReactantCopy(rpe_data->rpe->getMonomer(monomers_indices[i]), NULL, NULL);

   reaction.addProductCopy(new_product, NULL, NULL);

   for (int i = reaction.begin(); i != reaction.end(); i = reaction.next(i))
      reaction.getMolecule(i).stereocenters.markBonds();

   reaction.name.copy(product.name);
}

CEXPORT int indigoReactionProductEnumerate (int reaction, int monomers)
{
   INDIGO_BEGIN
   {
      QueryReaction &query_rxn = self.getObject(reaction).getQueryReaction();
      IndigoArray &monomers_object = self.getObject(monomers).asArray();

      ReactionProductEnumerator rpe(query_rxn);

      ObjArray<Reaction> out_reactions;

      if (monomers_object.objects.size() < query_rxn.reactantsCount())
         throw IndigoError("Too small monomers array");

      int user_reactant_idx = 0;
      for (int i = query_rxn.reactantBegin();
               i != query_rxn.reactantEnd();
               i = query_rxn.reactantNext(i))
      {
         IndigoArray &reactant_monomers_object = monomers_object.objects[i]->asArray();
         
         for (int j = 0; j < reactant_monomers_object.objects.size(); j++)
         {
            Molecule &monomer = reactant_monomers_object.objects[j]->getMolecule();
            rpe.addMonomer(i, monomer);
         }
         user_reactant_idx++;
      }

      rpe.is_multistep_reaction = self.rpe_params.is_multistep_reactions;
      rpe.is_one_tube = self.rpe_params.is_one_tube;
      rpe.is_self_react = self.rpe_params.is_self_react;
      rpe.max_deep_level = self.rpe_params.max_deep_level;
      rpe.max_product_count = self.rpe_params.max_product_count;

      rpe.product_proc = product_proc;

      ProductEnumeratorCallbackData rpe_data;
      rpe_data.out_reactions = &out_reactions;
      rpe_data.rpe = &rpe;

      rpe.userdata = &rpe_data;

      rpe.buildProducts();

      int out_array = indigoCreateArray();

      for (int i = 0; i < out_reactions.size(); i++)
      {
         QS_DEF(IndigoReaction, indigo_rxn);
         indigo_rxn.rxn.clone(out_reactions[i], NULL, NULL);
         
         indigoArrayAdd(out_array, self.addObject(indigo_rxn.clone()));
      }

      return out_array;
   }
   INDIGO_END(0, -1)
}

int indigoProductEnumeratorSetMultistepReactionFlag (int is_multistep_reactions)
{
   INDIGO_BEGIN
   {
      self.rpe_params.is_multistep_reactions = (is_multistep_reactions != 0);
   }
   INDIGO_END(1, 0)
}

int indigoProductEnumeratorSetOneTubeMode (const char *mode_string)
{
   INDIGO_BEGIN
   {
      if (strcmp(mode_string, "one-tube") == 0)
         self.rpe_params.is_one_tube = true;
      else if (strcmp(mode_string, "grid") == 0)
         self.rpe_params.is_one_tube = false;
      else
         throw IndigoError("%s is bad reaction product enumerator mode string", mode_string);
   }
   INDIGO_END(1, 0)
}

int indigoProductEnumeratorSetSelfReactionFlag (int is_self_react)
{
   INDIGO_BEGIN
   {
      self.rpe_params.is_self_react = (is_self_react != 0);
   }
   INDIGO_END(1, 0)
}

int indigoProductEnumeratorSetMaximumSearchDepth (int max_depth)
{
   INDIGO_BEGIN
   {
      self.rpe_params.max_deep_level = max_depth;
   }
   INDIGO_END(1, 0)
}

int indigoProductEnumeratorSetMaximumProductsCount (int max_pr_cnt)
{
   INDIGO_BEGIN
   {
      self.rpe_params.max_product_count = max_pr_cnt;
   }
   INDIGO_END(1, 0)
}


class _IndigoRPEOptionsHandlersSetter
{
public:
   _IndigoRPEOptionsHandlersSetter ();

};

_IndigoRPEOptionsHandlersSetter::_IndigoRPEOptionsHandlersSetter ()
{
   OptionManager &mgr = indigoGetOptionManager();
   OsLocker locker(mgr.lock);

   mgr.setOptionHandlerBool("rpe-multistep-reactions", indigoProductEnumeratorSetMultistepReactionFlag);
   mgr.setOptionHandlerString("rpe-mode", indigoProductEnumeratorSetOneTubeMode);
   mgr.setOptionHandlerBool("rpe-self-reaction", indigoProductEnumeratorSetSelfReactionFlag);
   mgr.setOptionHandlerInt("rpe-max-depth", indigoProductEnumeratorSetMaximumSearchDepth);
   mgr.setOptionHandlerInt("rpe-max-products-count", indigoProductEnumeratorSetMaximumProductsCount);
}

_IndigoRPEOptionsHandlersSetter _indigo_rpe_options_handlers_setter;
