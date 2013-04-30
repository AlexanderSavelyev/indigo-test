/****************************************************************************
 * Copyright (C) 2010-2013 GGA Software Services LLC
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


#include "bingo.h"
#include "bingo_object.h"

#include "indigo_internal.h"
#include "indigo_molecule.h"
#include "indigo_reaction.h"
#include "indigo_cpp.h"
#include "bingo_internal.h"

#include "bingo_index.h"

#include <stdio.h>
#include <string>

#include "base_cpp/ptr_pool.h"
#include "base_cpp/auto_ptr.h"
#include "base_cpp/exception.h"

using namespace indigo;

// TODO: warning C4273: 'indigo::BingoException::BingoException' : inconsistent dll linkage 
IMPL_EXCEPTION(indigo, BingoException, "bingo");

static PtrPool<bingo::Index> _bingo_instances;
static PtrPool<bingo::Matcher> _searches;

static int _bingoCreateOrLoadDatabaseFile (const char *location, const char *type, const char *options, bool create)
{
   MoleculeFingerprintParameters fp_params;

   fp_params.ext = 0;
   fp_params.any_qwords = 15;
   fp_params.ord_qwords = 25;
   fp_params.tau_qwords = 0;
   fp_params.sim_qwords = 8;
   
   AutoPtr<bingo::Index> context;
   std::string loc_dir(location);

   if (loc_dir.find_last_of('/') != loc_dir.length() - 1)
      loc_dir += '/';

   if (strcmp(type, "molecule") == 0)
      context.reset(new bingo::MoleculeIndex());
   else if (strcmp(type, "reaction") == 0)
      context.reset(new bingo::ReactionIndex());
   else
      throw BingoException("wrong database type option");

   if (create)
      context->create(loc_dir.c_str(), fp_params, options);
   else
      context->load(loc_dir.c_str());

   int db_id = _bingo_instances.add(context.release());

   return db_id;
}


static int _insertObjectToDatabase ( Indigo &self, bingo::Index &bingo_index, IndigoObject &indigo_obj, int obj_id)
{
   if (bingo_index.getType() == bingo::Index::MOLECULE)
   {
      if (!IndigoMolecule::is(indigo_obj))
         throw BingoException("bingoInsertRecordObj: Only molecule objects can be added to molecule index");

      indigo_obj.getBaseMolecule().aromatize(self.arom_options);
         
      bingo::IndexMolecule ind_mol(indigo_obj.getMolecule());
      int id = bingo_index.add(ind_mol, obj_id);
      return id;
   }
   else if (bingo_index.getType() == bingo::Index::REACTION)
   {
      if (!IndigoReaction::is(indigo_obj))
         throw BingoException("bingoInsertRecordObj: Only reaction objects can be added to reaction index");

      indigo_obj.getBaseReaction().aromatize(self.arom_options);

      bingo::IndexReaction ind_rxn(indigo_obj.getReaction());
      int id = bingo_index.add(ind_rxn, obj_id);
      return id;
   }
   else
      throw BingoException("bingoInsertRecordObj: Incorrect database");

   return -1;
}


CEXPORT int bingoCreateDatabaseFile (const char *location, const char *type, const char *options)
{
   INDIGO_BEGIN
   {
      return _bingoCreateOrLoadDatabaseFile(location, type, options, true);
   }
   INDIGO_END(-1);
}

CEXPORT int bingoLoadDatabaseFile (const char *location, const char *type)
{
   INDIGO_BEGIN
   {
      return _bingoCreateOrLoadDatabaseFile(location, type, NULL, false);
   }
   INDIGO_END(-1);
}

CEXPORT int bingoCloseDatabase (int db)
{
   INDIGO_BEGIN
   {
      _bingo_instances.remove(db);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int bingoInsertRecordObj (int db, int obj)
{
   INDIGO_BEGIN
   {
      IndigoObject &indigo_obj = self.getObject(obj);
      bingo::Index &bingo_index = _bingo_instances.ref(db);

      long obj_id = -1;
      RedBlackStringObjMap< Array<char> > *properties = indigo_obj.getProperties();

      if (properties != 0)
      {
         const char *key_name = bingo_index.getIdPropertyName();

         if (key_name != 0 && properties->find(key_name))
         {
            Array<char> &key_str = properties->at(key_name);
            obj_id = strtol(key_str.ptr(), NULL, 10);
         }
      }

      return _insertObjectToDatabase (self, bingo_index, indigo_obj, obj_id);
   }
   INDIGO_END(-1);
}

CEXPORT int bingoInsertRecordObjWithId (int db, int obj, int id)
{
   INDIGO_BEGIN
   {
      IndigoObject &indigo_obj = self.getObject(obj);
      bingo::Index &bingo_index = _bingo_instances.ref(db);

      return _insertObjectToDatabase (self, bingo_index, indigo_obj, id);
   }
   INDIGO_END(-1);
}

CEXPORT int bingoDeleteRecord (int db, int index)
{
   INDIGO_BEGIN
   {
      bingo::Index &bingo_index = _bingo_instances.ref(db);

      bingo_index.remove(index);

      return index;
   }
   INDIGO_END(-1);
}

CEXPORT int bingoSearchSub (int db, int query_obj, const char *options)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(query_obj);
      
      if (IndigoQueryMolecule::is(obj))
      {
         obj.getBaseMolecule().aromatize(self.arom_options);

         AutoPtr<bingo::MoleculeSubstructureQueryData> query_data(new bingo::MoleculeSubstructureQueryData(obj.getQueryMolecule()));

         bingo::MoleculeIndex &bingo_index = dynamic_cast<bingo::MoleculeIndex &>(_bingo_instances.ref(db));
         bingo::MoleculeSubMatcher *matcher = dynamic_cast<bingo::MoleculeSubMatcher *>(bingo_index.createMatcher("sub", query_data.release()));
         return _searches.add(matcher);
      }
      else if (IndigoQueryReaction::is(obj))
      {
         obj.getBaseReaction().aromatize(self.arom_options);

         AutoPtr<bingo::ReactionSubstructureQueryData> query_data(new bingo::ReactionSubstructureQueryData(obj.getQueryReaction()));

         bingo::ReactionIndex &bingo_index = dynamic_cast<bingo::ReactionIndex &>(_bingo_instances.ref(db));
         bingo::ReactionSubMatcher *matcher = dynamic_cast<bingo::ReactionSubMatcher *>(bingo_index.createMatcher("sub", query_data.release()));
         return _searches.add(matcher);
      }
      else
         throw BingoException("bingoSearchSub: only query molecule and query reaction can be set as query object");
   }
   INDIGO_END(-1);
}

CEXPORT int bingoSearchSim (int db, int query_obj, float min, float max, const char *options)
{
   INDIGO_BEGIN
   {
      IndigoObject &obj = self.getObject(query_obj);
      
      if (IndigoMolecule::is(obj))
      {
         obj.getBaseMolecule().aromatize(self.arom_options);

         AutoPtr<bingo::MoleculeSimilarityQueryData> query_data(new bingo::MoleculeSimilarityQueryData(obj.getMolecule(), min, max));

         bingo::MoleculeIndex &bingo_index = dynamic_cast<bingo::MoleculeIndex &>(_bingo_instances.ref(db));
         bingo::SimMatcher *matcher = dynamic_cast<bingo::SimMatcher *>(bingo_index.createMatcher("sim", query_data.release()));
         return _searches.add(matcher);
      }
      else if (IndigoReaction::is(obj))
      {
         obj.getBaseReaction().aromatize(self.arom_options);

         AutoPtr<bingo::ReactionSimilarityQueryData> query_data(new bingo::ReactionSimilarityQueryData(obj.getReaction(), min, max));

         bingo::ReactionIndex &bingo_index = dynamic_cast<bingo::ReactionIndex &>(_bingo_instances.ref(db));
         bingo::SimMatcher *matcher = dynamic_cast<bingo::SimMatcher *>(bingo_index.createMatcher("sim", query_data.release()));

         return _searches.add(matcher);
      }
      else
         throw BingoException("bingoSearchSub: only query molecule and query reaction can be set as query object");
   }
   INDIGO_END(-1);
}

CEXPORT int bingoEndSearch (int search_obj)
{
   INDIGO_BEGIN
   {
      _searches.remove(search_obj);
      return 1;
   }
   INDIGO_END(-1);
}

CEXPORT int bingoNext (int search_obj)
{
   INDIGO_BEGIN
   {
      return _searches.ref(search_obj).next();
   }
   INDIGO_END(-1);
}

CEXPORT int bingoGetCurrentId (int search_obj)
{
   INDIGO_BEGIN
   {
      return _searches.ref(search_obj).currentId();
   }
   INDIGO_END(-1);
}

CEXPORT int bingoGetObject (int search_obj)
{
   INDIGO_BEGIN
   {
      throw BingoException("bingoGetObject is not implemented yet");
   }
   INDIGO_END(-1);
}
