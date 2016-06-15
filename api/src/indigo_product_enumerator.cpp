/****************************************************************************
 * Copyright (C) 2009-2015 EPAM Systems
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
#include "indigo_reaction.h"
#include "indigo_array.h"
#include "base_cpp/scanner.h"
#include "base_cpp/output.h"
#include "base_cpp/cancellation_handler.h"
#include "layout/molecule_layout.h"
#include "layout/reaction_layout.h"
#include "molecule/molecule.h"
#include "molecule/molfile_loader.h"
#include "molecule/sdf_loader.h"
#include "molecule/molfile_saver.h"
#include "molecule/molecule_auto_loader.h"
#include "reaction/rxnfile_loader.h"
#include "reaction/rxnfile_saver.h"
#include "reaction/reaction_auto_loader.h"
#include "reaction/reaction_product_enumerator.h"
#include "reaction/reaction_transformation.h"
#include "base_cpp/properties_map.h"
#include "indigo_mapping.h"
#include "indigo_molecule.h"

CEXPORT const char * cano (Molecule &mol)
{
   INDIGO_BEGIN
   {
      AutoPtr<IndigoMolecule> molptr(new IndigoMolecule());
      molptr->mol.clone_KeepIndices(mol);
      return indigoCanonicalSmiles(self.addObject(molptr.release()));
   }
   INDIGO_END("<err>");
}

struct ProductEnumeratorCallbackData 
{
   ReactionProductEnumerator *rpe;
   ObjArray<Reaction> *out_reactions;
   ObjArray<Array<int>> *out_indices;
};

static void product_proc( Molecule &product, Array<int> &monomers_indices, Array<int> &mapping, void *userdata )
{
   ProductEnumeratorCallbackData *rpe_data = (ProductEnumeratorCallbackData *)userdata;

   QS_DEF(Molecule, new_product);
   new_product.clear();
   new_product.clone(product, NULL, NULL);   

   Reaction &reaction = rpe_data->out_reactions->push();
   reaction.clear();

   for (int i = 0; i < monomers_indices.size(); i++)
      reaction.addReactantCopy(rpe_data->rpe->getMonomer(monomers_indices[i]), NULL, NULL);

   reaction.addProductCopy(new_product, NULL, NULL);
   reaction.name.copy(product.name);
   
   Array<int> &indices = rpe_data->out_indices->push();
   indices.copy(monomers_indices);
}

CEXPORT int indigoReactionProductEnumerate (int reaction, int monomers)
{
   INDIGO_BEGIN
   {
      bool has_coord = false;

      QueryReaction &query_rxn = self.getObject(reaction).getQueryReaction();
      IndigoArray &monomers_object = IndigoArray::cast(self.getObject(monomers));

      ReactionProductEnumerator rpe(query_rxn);
      rpe.arom_options = self.arom_options;

      if (monomers_object.objects.size() < query_rxn.reactantsCount())
         throw IndigoError("Too small monomers array");

      ObjArray<PropertiesMap> monomers_properties;
      for (int i = query_rxn.reactantBegin();
               i != query_rxn.reactantEnd();
               i = query_rxn.reactantNext(i))
      {
         IndigoArray &reactant_monomers_object = IndigoArray::cast(*monomers_object.objects[i]);

         auto size = reactant_monomers_object.objects.size();
         for (int j = 0; j < size; j++)
         {
            IndigoObject &object = *reactant_monomers_object.objects[j];
            monomers_properties.push().copy(object.getProperties());
            
            Molecule &monomer = object.getMolecule();
            rpe.addMonomer(i, monomer);
            if (monomer.have_xyz)
               has_coord = true;
         }
      }

      rpe.is_multistep_reaction = self.rpe_params.is_multistep_reactions;
      rpe.is_one_tube = self.rpe_params.is_one_tube;
      rpe.is_self_react = self.rpe_params.is_self_react;
      rpe.max_deep_level = self.rpe_params.max_deep_level;
      rpe.max_product_count = self.rpe_params.max_product_count;

      rpe.product_proc = product_proc;

      ObjArray<Reaction> out_reactions;
      ObjArray<Array<int>> out_indices_all;
      
      ProductEnumeratorCallbackData rpe_data;
      rpe_data.out_reactions = &out_reactions;
      rpe_data.out_indices = &out_indices_all;
      rpe_data.rpe = &rpe;
      rpe.userdata = &rpe_data;

      rpe.buildProducts();

      int out_array = indigoCreateArray();

      for (int k = 0; k < out_reactions.size(); k++)
      {
         Reaction& out_reaction = out_reactions[k];
         if (has_coord && self.rpe_params.is_layout)
         {
            ReactionLayout layout(out_reaction, self.smart_layout);
            layout.make();
            out_reaction.markStereocenterBonds();
         }

         QS_DEF(IndigoReaction, indigo_rxn);
         indigo_rxn._monomersProperties.clear();
         indigo_rxn.rxn.clone(out_reaction, NULL, NULL, NULL);

         Array<int>& out_indices = out_indices_all[k];
         for (auto m = 0; m < out_indices.size(); m++) {
            indigo_rxn._monomersProperties.push().copy(monomers_properties[out_indices[m]]);
         }

         indigoArrayAdd(out_array, self.addObject(indigo_rxn.clone()));
      }

      return out_array;
   }
   INDIGO_END(-1)
}

CEXPORT int indigoTransform (int reaction, int monomers)
{
   INDIGO_BEGIN
   {
      IndigoObject &monomers_object = self.getObject(monomers);
      QueryReaction &query_rxn = self.getObject(reaction).getQueryReaction();
         
      ReactionTransformation rt;
      rt.arom_options = self.arom_options;
      rt.layout_flag = self.rpe_params.transform_is_layout;
      rt.smart_layout = self.smart_layout;

      // Try to work with molecule first
      bool is_mol = false;
      try 
      {
         monomers_object.getMolecule();
         is_mol = true;
      }
      catch (IndigoError)
      {
      }

      TimeoutCancellationHandler cancellation(self.cancellation_timeout);
      rt.cancellation = &cancellation;

      bool transformed_flag = false;
   
      IndigoObject *out_mapping = 0;
      
      if (is_mol)
      {
         Array<int> mapping;
         Molecule &mol = monomers_object.getMolecule();
         Molecule input_mol;
         input_mol.clone(mol, 0, 0);
         
         transformed_flag = rt.transform(mol, query_rxn, &mapping);

         AutoPtr<IndigoMapping> mptr(new IndigoMapping(input_mol, mol));
      
         mptr.get()->mapping.copy(mapping);

         out_mapping = mptr.release();
      }
      else if (monomers_object.type == IndigoObject::ARRAY)
      {
         IndigoArray &monomers_array = IndigoArray::cast(self.getObject(monomers));
         AutoPtr<IndigoArray> out_array(new IndigoArray());
         
         for (int i = 0; i < monomers_array.objects.size(); i++)
         {
            
            Array<int> mapping;
            Molecule &mol = monomers_object.getMolecule();
            Molecule input_mol;
            input_mol.clone(mol, 0, 0);
         
            if (rt.transform(monomers_array.objects[i]->getMolecule(), query_rxn, &mapping))
               transformed_flag = true;

            AutoPtr<IndigoMapping> mptr(new IndigoMapping(input_mol, mol));
            mptr.get()->mapping.copy(mapping);

            out_array.get()->objects.add(mptr.release());
         }

         out_mapping = out_array.release();
      }
      else
         throw IndigoError("%s is not a molecule or array of molecules", self.getObject(monomers).debugInfo());

      if (transformed_flag)
         return self.addObject(out_mapping);
      else
      {
         return 0;
      }
   }
   INDIGO_END(-1)
}

void indigoProductEnumeratorSetMultistepReactionFlag (int is_multistep_reactions)
{
   Indigo &self = indigoGetInstance();
   self.rpe_params.is_multistep_reactions = (is_multistep_reactions != 0);
}

void indigoProductEnumeratorSetOneTubeMode (const char *mode_string)
{
   Indigo &self = indigoGetInstance();
   if (strcmp(mode_string, "one-tube") == 0)
      self.rpe_params.is_one_tube = true;
   else if (strcmp(mode_string, "grid") == 0)
      self.rpe_params.is_one_tube = false;
   else
      throw IndigoError("%s is bad reaction product enumerator mode string", mode_string);
}

void indigoProductEnumeratorSetSelfReactionFlag (int is_self_react)
{
   Indigo &self = indigoGetInstance();
   self.rpe_params.is_self_react = (is_self_react != 0);
}

void indigoProductEnumeratorSetMaximumSearchDepth (int max_depth)
{
   Indigo &self = indigoGetInstance();
   self.rpe_params.max_deep_level = max_depth;
}

void indigoProductEnumeratorSetMaximumProductsCount (int max_pr_cnt)
{
   Indigo &self = indigoGetInstance();
   self.rpe_params.max_product_count = max_pr_cnt;
}

void indigoProductEnumeratorSetLayoutFlag (int layout_flag)
{
   Indigo &self = indigoGetInstance();
   self.rpe_params.is_layout = (layout_flag != 0);
}

void indigoProductEnumeratorSetTransformLayoutFlag (int transform_layout_flag)
{
   Indigo &self = indigoGetInstance();
   self.rpe_params.transform_is_layout = (transform_layout_flag != 0);
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
   mgr.setOptionHandlerBool("rpe-layout", indigoProductEnumeratorSetLayoutFlag);
   mgr.setOptionHandlerBool("transform-layout", indigoProductEnumeratorSetTransformLayoutFlag);
}

_IndigoRPEOptionsHandlersSetter _indigo_rpe_options_handlers_setter;
