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

#include "lzw/lzw_encoder.h"
#include "base_cpp/bitoutworker.h"
#include "base_cpp/output.h"

using namespace indigo;

LzwEncoder::LzwEncoder( LzwDict &NewDict, Output &NewOut ) : 
_dict(NewDict), _bitout(_dict.getBitCodeSize(), NewOut) 
{
   _string = -1;
   _char = 0;
   _isFinished = false;
}

void LzwEncoder::start( void )
{
   _isFinished = false;
}

void LzwEncoder::send( int NextSymbol )
{
   int Index, HashIndex;

   if (_string < 0)
   {
      _string = NextSymbol;
      return;
   }
   
   _char = NextSymbol;

   HashIndex = _dict.hashFunction(_string, _char);

   /* 'string++char' is in dictionary */
   if ((Index = _dict.dictSearch(_string, _char, HashIndex)) != -1)
   {
      _string = Index; 
      return;
   }

   _dict.addElem(_string, _char, HashIndex);

   _bitout.writeBits(_string);
   
   _string = _char;
}


void LzwEncoder::finish( void )
{
   if (!_isFinished)
   {
      _bitout.writeBits(_string);
      _bitout.close();
      _string = -1;
      _isFinished = true;
   }
}

LzwEncoder::~LzwEncoder( void )
{
   finish();
}
