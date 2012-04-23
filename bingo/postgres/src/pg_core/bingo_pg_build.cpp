extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "utils/relcache.h"
#include "utils/rel.h"
#include "storage/bufmgr.h"
}
#ifdef qsort
#undef qsort
#endif

#include "bingo_pg_build.h"

#include "bingo_core_c.h"
#include "base_cpp/auto_ptr.h"

#include "pg_bingo_context.h"
#include "bingo_pg_buffer.h"
#include "bingo_pg_config.h"
#include "bingo_pg_common.h"
#include "bingo_pg_ext_bitset.h"
#include "bingo_pg_search_engine.h"
#include "mango_pg_build_engine.h"
#include "ringo_pg_build_engine.h"



BingoPgBuild::BingoPgBuild(PG_OBJECT index_ptr, const char* schema_name, const char* index_schema, bool new_index):
_index(index_ptr),
_bufferIndex(index_ptr),
_buildingState(new_index) {
   /*
    * Prepare buffer section for building or updating
    */
   BingoPgCommon::appendPath(index_schema);
   
   if (_buildingState) {
      _bufferIndex.setStrategy(BingoPgIndex::BUILDING_STRATEGY);
      _prepareBuilding(schema_name, index_schema);
   } else {
      _bufferIndex.setStrategy(BingoPgIndex::UPDATING_STRATEGY);
      _prepareUpdating();
   }

}

BingoPgBuild::~BingoPgBuild() {
   /*
    * Finish building stage
    */
   if(_buildingState) {
      fp_engine.ref().finishShadowProcessing();
   }
   /*
    * Write meta info in desctructor
    */
   _bufferIndex.writeDictionary(fp_engine.ref());
   _bufferIndex.writeMetaInfo();

}
/*
 * Inserts a new structure into the index
 * Returns true if insertion was successfull
 */
void BingoPgBuild::_prepareBuilding(const char* schema_name, const char* index_schema) {
   Relation index = (Relation) _index;
   BingoPgWrapper func_wr;
   const char* func_name = func_wr.getFuncName(index->rd_support[0]);
   
   BingoPgWrapper rel_wr;
   const char* rel_name = rel_wr.getRelName(index->rd_id);

   elog(DEBUG1, "bingo: index build: start create index '%s'", rel_name);

   BingoPgConfig bingo_config;
   /*
    * Safety check
    */
   if (RelationGetNumberOfBlocks(index) != 0)
      throw Error("cannot initialize non-empty bingo index \"%s\"",
           RelationGetRelationName(index));

   /*
    * Set up configuration from postgres table
    */
   bingo_config.readDefaultConfig(schema_name);
   /*
    * Update configuration from pg_class.reloptions
    */
   bingo_config.updateByIndexConfig(index);


   /*
    * Define index type
    */
   if (strcasecmp(func_name, "matchsub") == 0) {
      fp_engine.reset(new MangoPgBuildEngine(bingo_config, rel_name));
   } else if (strcasecmp(func_name, "matchrsub") == 0) {
      fp_engine.reset(new RingoPgBuildEngine(bingo_config, rel_name));
   } else {
      throw Error("internal error: unknown index build function %s", func_name);
   }

   /*
    * If new build then create a metapage and initial section
    */
   _bufferIndex.writeBegin(fp_engine.ref(), bingo_config);

   fp_engine.ref().prepareShadowInfo(schema_name, index_schema);
}

void BingoPgBuild::_prepareUpdating() {
   Relation index = (Relation) _index;
   BingoPgWrapper rel_wr;
   const char* rel_name = rel_wr.getRelName(index->rd_id);

   elog(DEBUG1, "bingo: index build: start update index '%s'", rel_name);

   BingoPgConfig bingo_config;

   _bufferIndex.readMetaInfo();
   _bufferIndex.readConfigParameters(bingo_config);


   /*
    * Define index type
    */
   if (_bufferIndex.getIndexType() == BINGO_INDEX_TYPE_MOLECULE)
      fp_engine.reset(new MangoPgBuildEngine(bingo_config, rel_name));
   else if (_bufferIndex.getIndexType() == BINGO_INDEX_TYPE_REACTION)
      fp_engine.reset(new RingoPgBuildEngine(bingo_config, rel_name));
   else
      throw Error("internal error: unknown index type %d", _bufferIndex.getIndexType());

   /*
    * Prepare for an update
    */
   _bufferIndex.updateBegin();
   /*
    * Load cmf dictionary
    */
   fp_engine->loadDictionary(_bufferIndex);
}


bool BingoPgBuild::insertStructure(PG_OBJECT item_ptr, BingoPgText& struct_text) {
   /*
    * Insert a new structure
    */

   indigo::AutoPtr<BingoPgFpData> data;
   int block_number = ItemPointerGetBlockNumber((ItemPointer) item_ptr);
   int offset_number = ItemPointerGetOffsetNumber((ItemPointer)item_ptr);

   elog(DEBUG1, "bingo: insert structure: processing the table entry with ctid='(%d,%d)'::tid",  block_number, offset_number);

   if (!fp_engine->processStructure(struct_text, data)) {
      elog(WARNING, "can not insert a structure with ctid='(%d,%d)'::tid (see at the previous warning)", block_number, offset_number);
      return false;
   }

   BingoPgFpData& data_ref = data.ref();

   data_ref.setTidItem(item_ptr);

   _bufferIndex.insertStructure(data_ref);

   fp_engine->insertShadowInfo(data_ref);
   
   elog(DEBUG1, "bingo: insert structure: finish processing the table entry with ctid='(%d,%d)'::tid",  block_number, offset_number);

   return true;
}

void BingoPgBuild::_errorHandler(const char* message, void*) {
   throw Error("Error while building index: %s", message);
}


