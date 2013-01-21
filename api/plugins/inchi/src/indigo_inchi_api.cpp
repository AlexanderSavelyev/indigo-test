/****************************************************************************
 * Copyright (C) 2010-2012 GGA Software Services LLC
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

#include "indigo-inchi.h"

#include "indigo_inchi_core.h"
                      
#include "indigo_internal.h"
#include "indigo_molecule.h"
#include "option_manager.h"


using namespace indigo;

CEXPORT const char* indigoInchiVersion ()
{
   return IndigoInchi::version();
}

//
// Session Inchi instance
//

_SessionLocalContainer<IndigoInchi> indigo_inchi_self;

IndigoInchi &indigoInchiGetInstance ()
{
   return indigo_inchi_self.getLocalCopy();
}

// 
// C interface functions
//

CEXPORT int indigoInchiResetOptions (void)
{
   IndigoInchi &indigo_inchi = indigoInchiGetInstance();
   indigo_inchi.clear();
   return 0;
}

CEXPORT int indigoInchiLoadMolecule (const char *inchi_string)
{
   INDIGO_BEGIN
   {
      IndigoInchi &indigo_inchi = indigoInchiGetInstance();

      AutoPtr<IndigoMolecule> mol_obj(new IndigoMolecule());

      indigo_inchi.loadMoleculeFromInchi(inchi_string, mol_obj->mol);
      return self.addObject(mol_obj.release());
   }
   INDIGO_END(-1)
}

CEXPORT const char* indigoInchiGetInchi (int molecule)
{
   INDIGO_BEGIN
   {
      IndigoInchi &indigo_inchi = indigoInchiGetInstance();
      IndigoObject &obj = self.getObject(molecule);

      indigo_inchi.saveMoleculeIntoInchi(obj.getMolecule(), self.tmp_string);
      return self.tmp_string.ptr();
   }
   INDIGO_END(0)
}

CEXPORT const char* indigoInchiGetInchiKey (const char *inchi_string)
{
   INDIGO_BEGIN
   {
      self.tmp_string.resize(30);
      self.tmp_string.zerofill();
      int ret = GetINCHIKeyFromINCHI(inchi_string, 0, 0, self.tmp_string.ptr(), 0, 0);
      if (ret != INCHIKEY_OK)
      {
         if (ret == INCHIKEY_UNKNOWN_ERROR)
            throw IndigoError("Indigo-InChI: INCHIKEY_UNKNOWN_ERROR");
         else if (ret == INCHIKEY_EMPTY_INPUT)
            throw IndigoError("Indigo-InChI: INCHIKEY_EMPTY_INPUT");
         else if (ret == INCHIKEY_INVALID_INCHI_PREFIX)
            throw IndigoError("Indigo-InChI: INCHIKEY_INVALID_INCHI_PREFIX");
         else if (ret == INCHIKEY_NOT_ENOUGH_MEMORY)
            throw IndigoError("Indigo-InChI: INCHIKEY_NOT_ENOUGH_MEMORY");
         else if (ret == INCHIKEY_INVALID_INCHI)
            throw IndigoError("Indigo-InChI: INCHIKEY_INVALID_INCHI");
         else if (ret == INCHIKEY_INVALID_STD_INCHI)
            throw IndigoError("Indigo-InChI: INCHIKEY_INVALID_STD_INCHI");
         else
            throw IndigoError("Indigo-InChI: Undefined error");
      }
      return self.tmp_string.ptr();
   }
   INDIGO_END(0)
}

CEXPORT const char* indigoInchiGetWarning ()
{
   IndigoInchi &indigo_inchi = indigoInchiGetInstance();
   if (indigo_inchi.warning.size() != 0)
      return indigo_inchi.warning.ptr();
   return "";
}

CEXPORT const char* indigoInchiGetLog ()
{
   IndigoInchi &indigo_inchi = indigoInchiGetInstance();
   if (indigo_inchi.log.size() != 0)
      return indigo_inchi.log.ptr();
   return "";
}

CEXPORT const char* indigoInchiGetAuxInfo ()
{
   IndigoInchi &indigo_inchi = indigoInchiGetInstance();
   if (indigo_inchi.auxInfo.size() != 0)
      return indigo_inchi.auxInfo.ptr();
   return "";
}

//
// Options
//

void indigoInchiSetInchiOptions (const char *options)
{
   IndigoInchi &inchi = indigoInchiGetInstance();
   inchi.options.readString(options, true);
   // Replace '/' and '-' according to InChI manual:
   //   "(use - instead of / for O.S. other than MS Windows)"
#ifdef _WIN32
   for (int i = 0; i < inchi.options.size(); i++)
      if (inchi.options[i] == '-')
         inchi.options[i] = '/';
#else
   for (int i = 0; i < inchi.options.size(); i++)
      if (inchi.options[i] == '/')
         inchi.options[i] = '-';
#endif
}

class _IndigoInchiOptionsHandlersSetter
{
public:
   _IndigoInchiOptionsHandlersSetter ();
};

_IndigoInchiOptionsHandlersSetter::_IndigoInchiOptionsHandlersSetter ()
{
   OptionManager &mgr = indigoGetOptionManager();
   OsLocker locker(mgr.lock);
   
   mgr.setOptionHandlerString("inchi-options", indigoInchiSetInchiOptions);
}

_IndigoInchiOptionsHandlersSetter _indigo_inchi_options_handlers_setter;
