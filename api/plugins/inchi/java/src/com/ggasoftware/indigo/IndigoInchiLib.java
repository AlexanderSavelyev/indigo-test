/****************************************************************************
 * Copyright (C) 2011 GGA Software Services LLC
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

package com.ggasoftware.indigo;

import com.sun.jna.Library;

public interface IndigoInchiLib extends Library
{
	int indigoInchiResetOptions ();
	int indigoInchiLoadMolecule (String inchi_string);
	String indigoInchiGetInchi (int molecule);
	String indigoInchiGetInchiKey (String inchi_string);
	String indigoInchiGetWarning ();
	String indigoInchiGetLog ();
	String indigoInchiGetAuxInfo ();
}
