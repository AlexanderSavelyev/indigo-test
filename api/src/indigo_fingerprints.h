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

#ifndef __indigo_fingerprints__
#define __indigo_fingerprints__

#include "indigo_internal.h"

class IndigoFingerprint : public IndigoObject
{
public:
   IndigoFingerprint ();
   virtual ~IndigoFingerprint ();

   virtual DLLEXPORT void toString (Array<char> &str);
   virtual DLLEXPORT void toBuffer (Array<char> &buf);

   static DLLEXPORT IndigoFingerprint & cast (IndigoObject &obj);
   
   Array<byte> bytes;
};

#endif
