/****************************************************************************
 * Copyright (C) 2015 GGA Software Services LLC
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

#ifndef __multiple_cdx_loader__
#define __multiple_cdx_loader__

#include "base_cpp/tlscont.h"
#include "base_cpp/reusable_obj_array.h"


namespace indigo
{

class Scanner;

class MultipleCdxLoader
{
public:

   DECL_ERROR;
   
   MultipleCdxLoader (Scanner &scanner);

   bool isEOF ();
   void readNext ();
   void readAt (int index);
   int tell ();
   int currentNumber ();
   int count ();

   bool isReaction();

   CP_DECL;
   TL_CP_DECL(Array<char>, data);

protected:
   TL_CP_DECL(Array<int>, _offsets);
   Scanner &_scanner;
   int _current_number;
   int _max_offset;
   bool _reaction;

   void _checkHeader ();
   bool _findObject (int &beg, int &length);
   bool _hasNextObject ();
   void _skipObject ();
};

}
#endif
