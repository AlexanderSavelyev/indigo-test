#ifndef __bingo_base_index__
#define __bingo_base_index__

#include "molecule/molecule_fingerprint.h"
#include "bingo_object.h"
#include "bingo_fp_storage.h"
#include "bingo_cf_storage.h"
#include "bingo_mmf_storage.h"
#include "bingo_properties.h"
#include "bingo_exact_storage.h"
#include "bingo_fingerprint_table.h"

using namespace indigo;

namespace bingo
{
   class Matcher;
   class MatcherQueryData;

   class Index
   {
   public:
      virtual Matcher* createMatcher (const char *type, MatcherQueryData *query_data, const char *options) = 0;

      virtual void create (const char *location, const MoleculeFingerprintParameters &fp_params, const char *options) = 0;

      virtual void load (const char *location, const char *options) = 0;

      virtual int add (IndexObject &obj, int obj_id) = 0;

      virtual void optimize () = 0;

      virtual void remove (int id) = 0;
   
      typedef enum {MOLECULE, REACTION} IndexType;

      virtual const char * getIdPropertyName () = 0;

      virtual IndexType getType () const = 0;

      virtual ~Index () {};
   };

   class BaseIndex : public Index
   {
   private:   
      struct _Header
      {
         size_t properties_offset;
         size_t mapping_offset;
         size_t back_mapping_offset;
         size_t cf_offset;
         size_t sub_offset;
         size_t sim_offset;
         size_t exact_offset;
      };

   public:
      virtual void create (const char *location, const MoleculeFingerprintParameters &fp_params, const char *options);

      virtual void load (const char *location, const char *options);
      
      virtual int add (IndexObject &obj, int obj_id);

      virtual void optimize ();

      virtual void remove (int id);

      const MoleculeFingerprintParameters & getFingerprintParams () const;

      TranspFpStorage & getSubStorage ();

      FingerprintTable & getSimStorage ();

      ExactStorage & getExactStorage ();

      BingoArray<int> & getIdMapping ();

      BingoArray<int> & getBackIdMapping ();

      ByteBufferStorage & getCfStorage ();

      int getObjectsCount () const;

      virtual const char * getIdPropertyName ();

      virtual IndexType getType () const;

      static IndexType determineType (const char *location);

      virtual ~BaseIndex ();

   protected:
      BaseIndex (IndexType type);
      IndexType _type;

   private:
      struct _ObjectIndexData 
      {
         Array<byte> sub_fp;
         Array<byte> sim_fp;
         Array<char> cf_str;
         dword hash;
      };

      MMFStorage _mmf_storage;
      BingoPtr<_Header> _header;
      BingoPtr< BingoArray<int> > _id_mapping_ptr;
      BingoPtr< BingoArray<int> > _back_id_mapping_ptr;
      BingoPtr<TranspFpStorage> _sub_fp_storage;
      BingoPtr<FingerprintTable> _sim_fp_storage;
      BingoPtr<ExactStorage> _exact_storage;
      BingoPtr<ByteBufferStorage> _cf_storage;
      BingoPtr<Properties> _properties;
      
      _ObjectIndexData _object_index_data;
      MoleculeFingerprintParameters _fp_params;
      std::string _location;

      int _object_count;

      int _first_free_id;

      static void _parseOptions (const char *options, std::map<std::string, std::string> &option_map);

      static size_t _getMMfSize (std::map<std::string, std::string> &option_map);

      void _saveProperties (const MoleculeFingerprintParameters &fp_params, int sub_block_size, 
                            int sim_block_size, int cf_block_size, 
                            std::map<std::string, std::string> &option_map);

      bool _prepareIndexData (IndexObject &obj);

      void _insertIndexData();

      void _mappingCreate ();

      void _mappingLoad ();

      void _mappingAssign (int obj_id, int base_id);

      void _mappingAdd (int obj_id, int base_id);

      void _mappingRemove (int obj_id);
   };
};

#endif // __bingo_base_index__
