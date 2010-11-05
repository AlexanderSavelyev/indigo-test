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

#ifndef __indigo__
#define __indigo__

#ifdef _WIN32
   #define qword unsigned _int64
#ifdef INDIGO_PLUGIN
#define DLLEXPORT __declspec(dllimport)
#else
#define DLLEXPORT __declspec(dllexport)
#endif
#else
   #define qword unsigned long long
   #define DLLEXPORT
#endif

#ifdef _WIN32
#ifndef __cplusplus
   #define CEXPORT __declspec(dllexport)
#else
   #define CEXPORT extern "C" __declspec(dllexport)
#endif
#else
#ifndef __cplusplus
   #define CEXPORT
#else
   #define CEXPORT extern "C"
#endif
#endif

#ifndef __byte_typedef__
#define __byte_typedef__
typedef unsigned char byte;
#endif

/* All integer and float functions return -1 on error. */
/* All string functions return zero pointer on error. */

/* System */

CEXPORT const char * indigoVersion ();

// Allocate a new session. Each session has its own
// set of objects created and options set up.
CEXPORT qword indigoAllocSessionId   ();
// Switch to another session. The session, if was not allocated
// previously, is allocated automatically and initialized with
// empty set of objects and default options.
CEXPORT void  indigoSetSessionId     (qword id);
// Release session. The memory used by the released session
// is not freed, but the number will be reused on
// further allocations.
CEXPORT void  indigoReleaseSessionId (qword id);

// Get the last error message
CEXPORT const char * indigoGetLastError (void);

typedef void (*INDIGO_ERROR_HANDLER)(const char *message, void *context);
CEXPORT void indigoSetErrorHandler (INDIGO_ERROR_HANDLER handler, void *context);

// Free an object
CEXPORT int indigoFree (int handle);
// Clone an object
CEXPORT int indigoClone (int object);
// Count object currently allocated
CEXPORT int indigoCountReferences (void);

/* Options */

CEXPORT int indigoSetOption (const char *name, const char *value);
CEXPORT int indigoSetOptionInt (const char *name, int value);
CEXPORT int indigoSetOptionBool (const char *name, int value);
CEXPORT int indigoSetOptionFloat (const char *name, float value);
CEXPORT int indigoSetOptionColor (const char *name, float r, float g, float b);
CEXPORT int indigoSetOptionXY (const char *name, int x, int y);

/* Basic input-output */

// indigoRead*** return a new reader object.
// indigoLoad*** return a new reader object which already
// contains all the data and does not depend on the given
// string/buffer. All these functions are low-level and
// rarely needed to anyone.

CEXPORT int indigoReadFile   (const char *filename);
CEXPORT int indigoReadString (const char *str);
CEXPORT int indigoLoadString (const char *str);
CEXPORT int indigoReadBuffer (const char *buffer, int size);
CEXPORT int indigoLoadBuffer (const char *buffer, int size);

// indigoWrite*** return a new writer object.

CEXPORT int indigoWriteFile   (const char *filename);
CEXPORT int indigoWriteBuffer (void);

/* Iterators */

/* Iterators work in the following way:
 *
 * int item, iter = indigoIterate***(...)
 *
 * if (iter == -1)
 * {
 *    fprintf(stderr, "%s", indigoGetLastError());
 *    return;
 * }
 *
 * while ((item = indigoNext(iter))
 * {
 *    if (item == -1)
 *    {
 *       fprintf(stderr, "%s", indigoGetLastError());
 *       break;
 *    }
 *
 *    printf("on item #%d\n", indigoIndex(item));
 *
 *    // do something with item
 *
 *    indigoFree(item);
 * }
 * indigoFree(iter);
 */

// Obtains the next element, returns zero if there is no next element
CEXPORT int indigoNext (int iter);
// Does not obtain the next element, just tells if there is one
CEXPORT int indigoHasNext (int iter);
// Returns the index of the element
CEXPORT int indigoIndex (int item);

/* Molecules, query molecules, SMARTS */

CEXPORT int indigoLoadMolecule  (int source);
CEXPORT int indigoLoadMoleculeFromString (const char *string);
CEXPORT int indigoLoadMoleculeFromFile   (const char *filename);
CEXPORT int indigoLoadMoleculeFromBuffer (const char *buffer, int size);

CEXPORT int indigoLoadQueryMolecule (int source);
CEXPORT int indigoLoadQueryMoleculeFromString (const char *string);
CEXPORT int indigoLoadQueryMoleculeFromFile   (const char *filename);
CEXPORT int indigoLoadQueryMoleculeFromBuffer (const char *buffer, int size);

CEXPORT int indigoLoadSmarts (int source);
CEXPORT int indigoLoadSmartsFromString (const char *string);
CEXPORT int indigoLoadSmartsFromFile   (const char *filename);
CEXPORT int indigoLoadSmartsFromBuffer (const char *buffer, int size);

CEXPORT int indigoSaveMolfile (int molecule, int output);
CEXPORT int indigoSaveMolfileToFile (int molecule, const char *filename);
CEXPORT const char * indigoMolfile (int molecule);

CEXPORT int indigoSaveCml (int object, int output);
CEXPORT int indigoSaveCmlToFile (int object, const char *filename);
CEXPORT const char * indigoCml (int object);

/* Reactions, query reactions */

CEXPORT int indigoLoadReaction  (int source);
CEXPORT int indigoLoadReactionFromString (const char *string);
CEXPORT int indigoLoadReactionFromFile   (const char *filename);
CEXPORT int indigoLoadReactionFromBuffer (const char *buffer, int size);

CEXPORT int indigoLoadQueryReaction (int source);
CEXPORT int indigoLoadQueryReactionFromString (const char *string);
CEXPORT int indigoLoadQueryReactionFromFile   (const char *filename);
CEXPORT int indigoLoadQueryReactionFromBuffer (const char *buffer, int size);

CEXPORT int indigoCreateReaction (void);
CEXPORT int indigoCreateQueryReaction (void);

CEXPORT int indigoAddReactant (int reaction, int molecule);
CEXPORT int indigoAddProduct  (int reaction, int molecule);

CEXPORT int indigoCountReactants (int reaction);
CEXPORT int indigoCountProducts  (int reaction);
// Counts both reactants and products.
CEXPORT int indigoCountMolecules (int reaction);
CEXPORT int indigoIterateReactants (int reaction);
CEXPORT int indigoIterateProducts  (int reaction);
// Returns an iterator for both reactants and products.
CEXPORT int indigoIterateMolecules (int reaction);

CEXPORT int indigoSaveRxnfile (int reaction, int output);
CEXPORT int indigoSaveRxnfileToFile (int reaction, const char *filename);
CEXPORT const char * indigoRxnfile (int reaction);

/* Accessing a molecule */

// Returns an iterator for all atoms of the given
// molecule, including r-sites and pseudoatoms.
CEXPORT int indigoIterateAtoms (int molecule);
CEXPORT int indigoIteratePseudoatoms (int molecule);
CEXPORT int indigoIterateRSites (int molecule);
CEXPORT int indigoIterateRGroups (int molecule);
CEXPORT int indigoIsPseudoatom (int atom);
CEXPORT int indigoIsRSite (int atom);
CEXPORT int indigoSingleAllowedRGroup (int rsite);

// Applicable to an R-Group, not to a molecule
CEXPORT int indigoIterateRGroupFragments (int rgroup);
CEXPORT int indigoCountAttachmentPoints (int rgroup);

CEXPORT const char * indigoPseudoatomLabel (int atom);
CEXPORT int indigoDegree (int atom);

// Returns zero if the charge is ambiguous
// If the charge is nonambiguous, returns 1 and writes *charge
CEXPORT int indigoGetCharge (int atom, int *charge);
// Same as indigoGetCharge
CEXPORT int indigoGetExplicitValence (int atom, int *valence);
// Same as indigoGetCharge
CEXPORT int indigoGetRadicalElectrons (int atom, int *electrons);
// Returns a number of element from the periodic table.
// Returns zero on ambiguous atom.
// Can not be applied to pseudo-atoms and R-sites.
CEXPORT int indigoAtomNumber (int atom);
// Returns zero on unspecified or ambiguous isotope
CEXPORT int indigoAtomIsotope (int atom);

CEXPORT int indigoCountAtoms (int molecule);
CEXPORT int indigoCountBonds (int molecule);
CEXPORT int indigoCountPseudoatoms (int molecule);
CEXPORT int indigoCountRSites (int molecule);

CEXPORT int indigoCisTransClear (int handle);
CEXPORT int indigoStereocentersClear (int handle);
CEXPORT int indigoCountStereocenters (int molecule);

/* Calculation on molecules */

CEXPORT int   indigoGrossFormula    (int molecule);
CEXPORT float indigoMolecularWeight (int molecule);
CEXPORT float indigoMostAbundantMass (int molecule);
CEXPORT float indigoMonoisotopicMass (int molecule);

CEXPORT const char * indigoCanonicalSmiles (int molecule);
CEXPORT const char * indigoLayeredCode (int molecule);

// Returns the number of connected components
CEXPORT int indigoCountComponents (int molecule);

CEXPORT int indigoCreateSubmolecule (int molecule, int nvertices, int *vertices);
CEXPORT int indigoCreateEdgeSubmolecule (int molecule, int nvertices, int *vertices, int nedges, int *edges);

/* Things that work both for molecules and reactions */

CEXPORT int indigoAromatize (int object);
CEXPORT int indigoDearomatize (int object);

CEXPORT int indigoLayout (int object);

CEXPORT const char * indigoSmiles (int item);

// Returns 1 if there is an exact match
CEXPORT int indigoExactMatch (int item1, int item2);

CEXPORT const char * indigoName (int handle);
CEXPORT int indigoSetName (int handle, const char *name);

// Applicable to molecules/reactions obtained from SDF or RDF files,
// and to their clones, and to their R-Group deconvolutions.
CEXPORT int indigoHasProperty (int handle, const char *field);
CEXPORT const char * indigoGetProperty (int handle, const char *field);

// Applicable to newly created or cloned molecules/reactions,
// and also to molecules/reactions obtained from SDF or RDF files.
// If the property with the given name does not exist, it is created automatically.
CEXPORT int indigoSetProperty (int handle, const char *field, const char *value);

// Returns an iterator that one can pass to indigoName() to
// know the name of the property. The value of the property can be
// obtained via indigoGetProperty() call to the object
CEXPORT int indigoIterateProperties (int handle);

// Accepts a molecule or reaction (but not query molecule or query reaction).
// Returns a string describing the first encountered mistake with valence.
// Returns an empty string if the input molecule/reaction is fine.
CEXPORT const char * indigoCheckBadValence (int handle);

// Accepts a molecule or reaction (but not query molecule or query reaction).
// Returns a string describing the first encountered mistake with ambiguous H counter.
// Returns an empty string if the input molecule/reaction is fine.
CEXPORT const char * indigoCheckAmbiguousH (int handle);

/* Fingerprints */

// Returns a 'fingerprint' object, which can then be passed to:
//   indigoToString() -- to get hexadecimal representation
//   indigoToBuffer() -- to get raw byte data
//   indigoSimilarity() -- to calculate similarity with another fingerprint
CEXPORT int indigoFingerprint (int item, const char *type);

// Counts the nonzero (i.e. one) bits in a fingerprint
CEXPORT int indigoCountBits (int fingerprint);

// Counts the number of the coinincident in two fingerprints
CEXPORT int indigoCommonBits (int fingerprint1, int fingerprint2);

// Accepts two molecules, two reactions, or two fingerprints.
// Returns the similarity measure between them.
// Metrics: "tanimoto", "tversky", "tversky <alpha> <beta>", or "euclid-sub".
// Zero pointer or empty string defaults to "tanimoto".
// "tversky" without numbers defaults to alpha = beta = 0.5
CEXPORT float indigoSimilarity (int item1, int item2, const char *metrics);

/* Working with SDF/RDF/SMILES files  */

CEXPORT int indigoIterateSDF    (int reader);
CEXPORT int indigoIterateRDF    (int reader);
CEXPORT int indigoIterateSmiles (int reader);

CEXPORT int indigoIterateSDFile     (const char *filename);
CEXPORT int indigoIterateRDFile     (const char *filename);
CEXPORT int indigoIterateSmilesFile (const char *filename);

// Applicable to items returned by SDF/RDF iterators.
// Returns the content of SDF/RDF item.
CEXPORT const char * indigoRawData (int item);

// Applicable to items returned by SDF/RDF iterators.
// Returns the offset in the SDF/RDF file.
CEXPORT int indigoTell (int handle);

// Saves molecule to an SDF output stream
CEXPORT int indigoSdfAppend (int output, int item);
// Saves molecule to a multiline SMILES output stream
CEXPORT int indigoSmilesAppend (int output, int item);

/* Arrays */

CEXPORT int indigoCreateArray ();
// Note: a clone of the object is added, not the object itself
CEXPORT int indigoArrayAdd (int arr, int object);
CEXPORT int indigoArrayAt (int arr, int index);
CEXPORT int indigoArrayCount (int arr);
CEXPORT int indigoArrayClear (int arr);
CEXPORT int indigoIterateArray (int arr);

/* Substructure matching */

// Returns a new 'match' object on success, zero on fail
CEXPORT int indigoMatchSubstructure (int query, int target);

// Accepts a 'match' object obtained from indigoMatchSubstructure.
// Returns a new molecule which has the query highlighted.
CEXPORT int indigoMatchHighlight (int match);

// Count number of embeddings of query into target molecule.
// Returns a number if embeddings.
CEXPORT int indigoCountSubstructureMatches (int query, int target);

/* Scaffold detection */

// Returns zero if no common substructure is found.
// Otherwise, it returns a new object, which can be
//   (i) treated as a structure: the maximum (by the number of rings) common
//       substructure of the given structures.
//  (ii) passed to indigoAllScaffolds()
CEXPORT int indigoExtractCommonScaffold (int structures, const char *options);

// Returns an array of all possible scaffolds.
// The input parameter is the value returned by indigoExtractCommonScaffold().
CEXPORT int indigoAllScaffolds (int extracted);

/* R-Group deconvolution */

// Returns a ``decomposition'' object that can be passed to
// indigoDecomposedMoleculeScaffold() and
// indigoIterateDecomposedMolecules()
CEXPORT int indigoDecomposeMolecules (int scaffold, int structures);

// Returns a scaffold molecule with r-sites marking the place
// for substituents to add to form the structures given above.
CEXPORT int indigoDecomposedMoleculeScaffold (int decomp);

// Returns an iterator which corresponds to the given collection of structures.
// indigoDecomposedMoleculeHighlighted() and
// indigoDecomposedMoleculeWithRGroups() are applicable to the
// values returned by the iterator.
CEXPORT int indigoIterateDecomposedMolecules (int decomp);

// Returns a molecule with highlighted scaffold
CEXPORT int indigoDecomposedMoleculeHighlighted (int decomp);

// Returns a query molecule with r-sites and "R1=...", "R2=..."
// substituents defined. The 'scaffold' part of the molecule
// is identical to the indigoDecomposedMoleculeScaffold()
CEXPORT int indigoDecomposedMoleculeWithRGroups (int decomp);

/* Other */

CEXPORT const char * indigoToString (int handle);
CEXPORT int indigoToBuffer (int handle, char **buf, int *size);
  
/* Reaction products enumeration */

// Accepts a query reaction with markd R-sites, and array of arrays
// of substituents corresponding to the R-Sites. Returns an array of
// reactions with R-Sites replaced by the actual substituents.
CEXPORT int indigoReactionProductEnumerate (int reaction, int monomers);

#endif
