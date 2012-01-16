/****************************************************************************
 * Copyright (C) 2010-2011 GGA Software Services LLC
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

/* Almost all string functions return the same pointer on success;
   you should not free() it, but rather strdup() it if you want to keep it. */

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

// Closes the file output stream but does not delete the object
CEXPORT int indigoClose (int output);

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
 * while (item = indigoNext(iter))
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

// Removes the item from its container (usually a molecule)
CEXPORT int indigoRemove (int item);

/* Molecules, query molecules, SMARTS */

CEXPORT int indigoCreateMolecule (void);
CEXPORT int indigoCreateQueryMolecule (void);

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

// accepts molecules and reactions (but not query ones)
CEXPORT int indigoSaveCml (int object, int output);
CEXPORT int indigoSaveCmlToFile (int object, const char *filename);
CEXPORT const char * indigoCml (int object);

// the output must be a file or a buffer, but not a string
// (because MDLCT data usually contains zeroes)
CEXPORT int indigoSaveMDLCT (int item, int output);

/* Reactions, query reactions */
/*
 * Reaction centers
 */
enum
{
   INDIGO_RC_NOT_CENTER     = -1,
   INDIGO_RC_UNMARKED       =  0,
   INDIGO_RC_CENTER         =  1,
   INDIGO_RC_UNCHANGED      =  2,
   INDIGO_RC_MADE_OR_BROKEN =  4,
   INDIGO_RC_ORDER_CHANGED  =  8
};
CEXPORT int indigoLoadReaction  (int source);
CEXPORT int indigoLoadReactionFromString (const char *string);
CEXPORT int indigoLoadReactionFromFile   (const char *filename);
CEXPORT int indigoLoadReactionFromBuffer (const char *buffer, int size);

CEXPORT int indigoLoadQueryReaction (int source);
CEXPORT int indigoLoadQueryReactionFromString (const char *string);
CEXPORT int indigoLoadQueryReactionFromFile   (const char *filename);
CEXPORT int indigoLoadQueryReactionFromBuffer (const char *buffer, int size);

CEXPORT int indigoLoadReactionSmarts (int source);
CEXPORT int indigoLoadReactionSmartsFromString (const char *string);
CEXPORT int indigoLoadReactionSmartsFromFile   (const char *filename);
CEXPORT int indigoLoadReactionSmartsFromBuffer (const char *buffer, int size);


CEXPORT int indigoCreateReaction (void);
CEXPORT int indigoCreateQueryReaction (void);

CEXPORT int indigoAddReactant (int reaction, int molecule);
CEXPORT int indigoAddProduct  (int reaction, int molecule);
CEXPORT int indigoAddCatalyst (int reaction, int molecule);

CEXPORT int indigoCountReactants (int reaction);
CEXPORT int indigoCountProducts  (int reaction);
CEXPORT int indigoCountCatalysts (int reaction);
// Counts reactants, products, and catalysts.
CEXPORT int indigoCountMolecules (int reaction);
CEXPORT int indigoGetMolecule (int reaction, int index);

CEXPORT int indigoIterateReactants (int reaction);
CEXPORT int indigoIterateProducts  (int reaction);
CEXPORT int indigoIterateCatalysts (int reaction);
// Returns an iterator for reactants, products, and catalysts.
CEXPORT int indigoIterateMolecules (int reaction);

CEXPORT int indigoSaveRxnfile (int reaction, int output);
CEXPORT int indigoSaveRxnfileToFile (int reaction, const char *filename);
CEXPORT const char * indigoRxnfile (int reaction);

// Method for query optimizations for faster substructure search
// (works for both query molecules and query reactions)
CEXPORT int indigoOptimize (int query, const char *options);

// Automatic reaction atom-to-atom mapping
// mode is one of the following (separated by a space):
//    "discard" : discards the existing mapping entirely and considers only
//                the existing reaction centers (the default)
//    "keep"    : keeps the existing mapping and maps unmapped atoms
//    "alter"   : alters the existing mapping, and maps the rest of the
//                reaction but may change the existing mapping
//    "clear"   : removes the mapping from the reaction.
//
//    "ignore_charges" : do not consider atom charges while searching
//    "ignore_isotopes" : do not consider atom isotopes while searching
//    "ignore_valence" : do not consider atom valence while searching
//    "ignore_radicals" : do not consider atom radicals while searching
CEXPORT int indigoAutomap (int reaction, const char *mode);

// Returns mapping number. It might appear that there is more them 
// one atom with the same number in AAM
// Value 0 means no mapping number has been specified.
CEXPORT int indigoGetAtomMappingNumber (int reaction, int reaction_atom);
CEXPORT int indigoSetAtomMappingNumber (int reaction, int reaction_atom, int number);

// Getters and setters for reacting centers
CEXPORT int indigoGetReactingCenter (int reaction, int reaction_bond, int*rc);
CEXPORT int indigoSetReactingCenter (int reaction, int reaction_bond, int rc);

// Clears all reaction AAM information 
CEXPORT int indigoClearAAM (int reaction);

/* Accessing a molecule */

enum
{
   INDIGO_ABS = 1,
   INDIGO_OR = 2,
   INDIGO_AND = 3,
   INDIGO_EITHER = 4,
   INDIGO_UP = 5,
   INDIGO_DOWN = 6,
   INDIGO_CIS = 7,
   INDIGO_TRANS = 8,
   INDIGO_CHAIN = 9,
   INDIGO_RING = 10,
   INDIGO_ALLENE = 11
};

// Returns an iterator for all atoms of the given
// molecule, including r-sites and pseudoatoms.
CEXPORT int indigoIterateAtoms (int molecule);
CEXPORT int indigoIteratePseudoatoms (int molecule);
CEXPORT int indigoIterateRSites (int molecule);
CEXPORT int indigoIterateStereocenters (int molecule);
CEXPORT int indigoIterateAlleneCenters (int molecule);
CEXPORT int indigoIterateRGroups (int molecule);

CEXPORT int indigoIsPseudoatom (int atom);
CEXPORT int indigoIsRSite (int atom);

// returns INDIGO_{ABS,OR,AND,EITHER}
// or zero if the atom is not a stereoatom
CEXPORT int indigoStereocenterType (int atom);
CEXPORT int indigoChangeStereocenterType (int atom, int type);
CEXPORT int indigoSingleAllowedRGroup (int rsite);

CEXPORT int indigoAddStereocenter (int atom, int type, int v1, int v2, int v3, int v4);

// Applicable to an R-Group, but not to a molecule
CEXPORT int indigoIterateRGroupFragments (int rgroup);
// Applicable to an R-Group and to a molecule
// Returns maximal order of attachment points
CEXPORT int indigoCountAttachmentPoints (int item);
CEXPORT int indigoIterateAttachmentPoints (int item, int order);

CEXPORT const char * indigoSymbol (int atom);
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
CEXPORT int indigoAtomicNumber (int atom);
// Returns zero on unspecified or ambiguous isotope
CEXPORT int indigoIsotope (int atom);
// Not applicable to query molecules.
CEXPORT int indigoValence (int atom);

// Applicable to atoms, query atoms, and molecules. Can fail
// (return zero) on query atoms where the number of hydrogens
// is not definitely known. Otherwise, returns one and writes *hydro.
CEXPORT int indigoCountHydrogens (int item, int *hydro);

// Applicable to non-query molecules and atoms.
CEXPORT int indigoCountImplicitHydrogens (int item);

// On success, returns always the same pointer to a 3-element array;
// you should not free() it, but rather memcpy() it if you want to keep it.
CEXPORT float * indigoXYZ (int atom);

CEXPORT int indigoSetXYZ (int atom, float x, float y, float z);

CEXPORT int indigoCountSuperatoms (int molecule);
CEXPORT int indigoCountDataSGroups (int molecule);
CEXPORT int indigoCountRepeatingUnits (int molecule);
CEXPORT int indigoCountMultipleGroups (int molecule);
CEXPORT int indigoCountGenericSGroups (int molecule);
CEXPORT int indigoIterateDataSGroups (int molecule);
CEXPORT int indigoIterateSuperatoms (int molecule);
CEXPORT int indigoIterateGenericSGroups (int molecule);
CEXPORT int indigoIterateRepeatingUnits (int molecule);
CEXPORT int indigoIterateMultipleGroups (int molecule);
CEXPORT int indigoGetSuperatom (int molecule, int index);
CEXPORT int indigoGetDataSGroup (int molecule, int index);
CEXPORT const char * indigoDescription (int data_sgroup);

CEXPORT int indigoAddDataSGroup (int molecule, int natoms, int *atoms,
        int nbonds, int *bonds, const char *description, const char *data);

CEXPORT int indigoAddSuperatom (int molecule, int natoms, int *atoms, const char *name);

CEXPORT int indigoSetDataSGroupXY (int sgroup, float x, float y, const char *options);

CEXPORT int indigoResetCharge (int atom);
CEXPORT int indigoResetExplicitValence (int atom);
CEXPORT int indigoResetRadical (int atom);
CEXPORT int indigoResetIsotope (int atom);

CEXPORT int indigoSetAttachmentPoint (int atom, int order);
CEXPORT int indigoClearAttachmentPoints (int item);

CEXPORT int indigoRemoveConstraints  (int item, const char *type);
CEXPORT int indigoAddConstraint      (int item, const char *type, const char *value);
CEXPORT int indigoAddConstraintNot   (int item, const char *type, const char *value);
CEXPORT int indigoAddConstraintOr    (int atom, const char* type, const char* value);

CEXPORT int indigoResetStereo (int item);
CEXPORT int indigoInvertStereo (int item);

CEXPORT int indigoCountAtoms (int molecule);
CEXPORT int indigoCountBonds (int molecule);
CEXPORT int indigoCountPseudoatoms (int molecule);
CEXPORT int indigoCountRSites (int molecule);

CEXPORT int indigoIterateBonds (int molecule);
// Returns 1/2/3 if the bond is a single/double/triple bond
// Returns 4 if the bond is an aromatic bond
// Returns zero if the bond is ambiguous (query bond)
CEXPORT int indigoBondOrder  (int bond);

// Returns INDIGO_{UP/DOWN/EITHER/CIS/TRANS},
// or zero if the bond is not a stereobond
CEXPORT int indigoBondStereo (int bond);

// Returns INDIGO_{CHAIN/RING},
CEXPORT int indigoTopology (int bond);

// Returns an iterator whose elements can be treated as atoms.
// At the same time, they support indigoBond() call.
CEXPORT int indigoIterateNeighbors (int atom);

// Applicable exclusively to the "atom neighbors iterator".
// Returns a bond to the neighbor atom.
CEXPORT int indigoBond (int nei);

// Accessing atoms and bonds by index
CEXPORT int indigoGetAtom (int molecule, int idx);
CEXPORT int indigoGetBond (int molecule, int idx);

CEXPORT int indigoSource (int bond);
CEXPORT int indigoDestination (int bond);

CEXPORT int indigoClearCisTrans (int handle);
CEXPORT int indigoClearStereocenters (int handle);
CEXPORT int indigoCountStereocenters (int molecule);
CEXPORT int indigoClearAlleneCenters (int molecule);
CEXPORT int indigoCountAlleneCenters (int molecule);

CEXPORT int indigoResetSymmetricCisTrans (int handle);
CEXPORT int indigoMarkEitherCisTrans (int handle);
CEXPORT int indigoMarkStereobonds (int handle);

// Accepts a symbol from the periodic table (like "C" or "Br"),
// or a pseudoatom symbol, like "Pol". Returns the added atom.
CEXPORT int indigoAddAtom (int molecule, const char *symbol);
// Set a new atom instead of specified
CEXPORT int indigoResetAtom (int atom, const char *symbol);

// Accepts Rsite name "R" (or just ""), "R1", "R2" or list with names "R1 R3"
CEXPORT int indigoAddRSite (int molecule, const char *name);
CEXPORT int indigoSetRSite (int atom, const char *name);

CEXPORT int indigoSetCharge (int atom, int charge);
CEXPORT int indigoSetIsotope (int atom, int isotope);

// Used for hacks with aromatic molecules; not recommended to use
// in other situations
CEXPORT int indigoSetImplicitHCount (int atom, int impl_h);

// Accepts two atoms (source and destination) and the order of the new bond
// (1/2/3/4 = single/double/triple/aromatic). Returns the added bond.
CEXPORT int indigoAddBond (int source, int destination, int order);

CEXPORT int indigoSetBondOrder (int bond, int order);

CEXPORT int indigoMerge (int where_to, int what);

/* Highlighting */

// Access atoms and bonds
CEXPORT int indigoHighlight (int item);

// Access atoms, bonds, molecules, and reactions
CEXPORT int indigoUnhighlight (int item);

// Access atoms and bonds
CEXPORT int indigoIsHighlighted (int item);

/* Connected components of molecules */

CEXPORT int indigoCountComponents (int molecule);
CEXPORT int indigoComponentIndex (int atom);
CEXPORT int indigoIterateComponents (int molecule);

// Returns a 'molecule component' object, which can not be used as a
// [query] molecule, but supports the indigo{Count,Iterate}{Atoms,Bonds} calls,
// and also the indigoClone() call, which returns a [query] molecule.
CEXPORT int indigoComponent (int molecule, int index);

/* Smallest Set of Smallest Rings */

CEXPORT int indigoCountSSSR (int molecule);
CEXPORT int indigoIterateSSSR (int molecule);

CEXPORT int indigoIterateSubtrees (int molecule, int min_atoms, int max_atoms);
CEXPORT int indigoIterateRings (int molecule, int min_atoms, int max_atoms);
CEXPORT int indigoIterateEdgeSubmolecules (int molecule, int min_bonds, int max_bonds);

/* Calculation on molecules */

CEXPORT int   indigoCountHeavyAtoms (int molecule);
CEXPORT int   indigoGrossFormula    (int molecule);
CEXPORT float indigoMolecularWeight (int molecule);
CEXPORT float indigoMostAbundantMass (int molecule);
CEXPORT float indigoMonoisotopicMass (int molecule);

CEXPORT const char * indigoCanonicalSmiles (int molecule);
CEXPORT const char * indigoLayeredCode (int molecule);

CEXPORT int indigoHasCoord (int molecule);
CEXPORT int indigoHasZCoord (int molecule);
CEXPORT int indigoIsChiral (int molecule);

CEXPORT int indigoCreateSubmolecule (int molecule, int nvertices, int *vertices);
CEXPORT int indigoCreateEdgeSubmolecule (int molecule, int nvertices, int *vertices, int nedges, int *edges);

CEXPORT int indigoGetSubmolecule (int molecule, int nvertices, int *vertices);

CEXPORT int indigoRemoveAtoms (int molecule, int nvertices, int *vertices);

// Determines and applies the best transformation to the given molecule
// so that the specified atoms move as close as possible to the desired
// positions. The size of desired_xyz is equal to 3 * natoms.
// The return value is the root-mean-square measure of the difference
// between the desired and obtained positions.
CEXPORT float indigoAlignAtoms (int molecule, int natoms, int *atom_ids, float *desired_xyz);

/* Things that work for both molecules and reactions */

CEXPORT int indigoAromatize (int item);
CEXPORT int indigoDearomatize (int item);

CEXPORT int indigoFoldHydrogens (int item);
CEXPORT int indigoUnfoldHydrogens (int item);

CEXPORT int indigoLayout (int object);

CEXPORT const char * indigoSmiles (int item);

// Returns a "mapping" if there is an exact match, zero otherwise
// The flags string consists of space-separated flags.
// The more flags, the more restrictive matching is done.
// "ELE": Distribution of electrons: bond types, atom charges, radicals, valences
// "MAS": Atom isotopes
// "STE": Stereochemistry: chiral centers, stereogroups, and cis-trans bonds
// "FRA": Connected fragments: disallows match of separate ions in salts
// "ALL": All of the above
// By default (with null or empty flags string) all flags are on.
CEXPORT int indigoExactMatch (int item1, int item2, const char *flags);

// "beg" and "end" refer to the two ends of the tautomeric chain. Allowed
// elements are separated by commas. '1' at the beginning means an aromatic
// atom, while '0' means an aliphatic atom.
CEXPORT int indigoSetTautomerRule (int id, const char *beg, const char *end);

CEXPORT int indigoRemoveTautomerRule (int id);

CEXPORT int indigoClearTautomerRules ();

CEXPORT const char * indigoName (int handle);
CEXPORT int indigoSetName (int handle, const char *name);

// You should not free() the obtained buffer, but rather memcpy() it if you want to keep it
CEXPORT int indigoSerialize (int handle, byte **buf, int *size);

CEXPORT int indigoUnserialize (const byte *buf, int size);

// Applicable to molecules/reactions obtained from SDF or RDF files,
// and to their clones, and to their R-Group deconvolutions.
CEXPORT int indigoHasProperty (int handle, const char *prop);
CEXPORT const char * indigoGetProperty (int handle, const char *prop);

// Applicable to newly created or cloned molecules/reactions,
// and also to molecules/reactions obtained from SDF or RDF files.
// If the property with the given name does not exist, it is created automatically.
CEXPORT int indigoSetProperty (int item, const char *prop, const char *value);

// Does not raise an error if the given property does not exist
CEXPORT int indigoRemoveProperty (int item, const char *prop);

// Returns an iterator that one can pass to indigoName() to
// know the name of the property. The value of the property can be
// obtained via indigoGetProperty() call to the object
CEXPORT int indigoIterateProperties (int handle);

// Clears all properties of the molecule
CEXPORT int indigoClearProperties (int handle);

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
// The following fingerprint types are available:
//   "sim"     -- "Similarity fingerprint", useful for calculating
//                 similarity measures (the default)
//   "sub"     -- "Substructure fingerprint", useful for substructure screening
//   "sub-res" -- "Resonance substructure fingerprint", useful for resonance
//                 substructure screening
//   "sub-tau" -- "Tautomer substructure fingerprint", useful for tautomer
//                 substructure screening
//   "full"    -- "Full fingerprint", which has all the mentioned
//                 fingerprint types included
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

/* Working with SDF/RDF/SMILES/CML files  */

CEXPORT int indigoIterateSDF    (int reader);
CEXPORT int indigoIterateRDF    (int reader);
CEXPORT int indigoIterateSmiles (int reader);
CEXPORT int indigoIterateCML    (int reader);

CEXPORT int indigoIterateSDFile     (const char *filename);
CEXPORT int indigoIterateRDFile     (const char *filename);
CEXPORT int indigoIterateSmilesFile (const char *filename);
CEXPORT int indigoIterateCMLFile    (const char *filename);

// Applicable to items returned by SDF/RDF iterators.
// Returns the content of SDF/RDF item.
CEXPORT const char * indigoRawData (int item);

// Applicable to items returned by SDF/RDF iterators.
// Returns the offset in the SDF/RDF file.
CEXPORT int indigoTell (int handle);

// Saves the molecule to an SDF output stream
CEXPORT int indigoSdfAppend (int output, int item);
// Saves the molecule to a multiline SMILES output stream
CEXPORT int indigoSmilesAppend (int output, int item);

// Similarly for RDF files, except that the header should be written first
CEXPORT int indigoRdfHeader (int output);
CEXPORT int indigoRdfAppend (int output, int item);

// Similarly for CML files, except that they have both header and footer
CEXPORT int indigoCmlHeader (int output);
CEXPORT int indigoCmlAppend (int output, int item);
CEXPORT int indigoCmlFooter (int output);

// Create saver objects that can be used to save molecules or reactions
// Supported formats: 'sdf', 'smi' or 'smiles', 'cml', 'rdf'
// Format argument is case-insensitive
// Saver should be closed with indigoClose function
CEXPORT int indigoCreateSaver (int output, const char *format);
CEXPORT int indigoCreateFileSaver (const char *filename, const char *format);

// Append object to a specified saver stream
CEXPORT int indigoAppend (int saver, int object);

/* Arrays */

CEXPORT int indigoCreateArray ();
// Note: a clone of the object is added, not the object itself
CEXPORT int indigoArrayAdd (int arr, int object);
CEXPORT int indigoAt (int item, int index);
CEXPORT int indigoCount (int item);
CEXPORT int indigoClear (int arr);
CEXPORT int indigoIterateArray (int arr);

/* Substructure matching */

// Returns a new 'matcher' object
// 'mode' is reserved for future use; currently its value is ignored
CEXPORT int indigoSubstructureMatcher (int target, const char *mode);

// Ignore target atom in the substructure matcher
CEXPORT int indigoIgnoreAtom (int matcher, int atom_object);

// Ignore target atom in the substructure matcher
CEXPORT int indigoUnignoreAtom (int matcher, int atom_object);

// Clear list of ignored target atoms in the substructure matcher
CEXPORT int indigoUnignoreAllAtoms (int matcher);

// Returns a new 'match' object on success, zero on fail
//    matcher is an matcher object returned by indigoSubstructureMatcher
CEXPORT int indigoMatch (int matcher, int query);                                                      

// Counts the number of embeddings of the query structure into the target
CEXPORT int indigoCountMatches (int matcher, int query);

// Counts the number of embeddings of the query structure into the target
// If number of embeddings is more then limit then limit is returned
CEXPORT int indigoCountMatchesWithLimit (int matcher, int query, int embeddings_limit);

// Returns substructure matches iterator
CEXPORT int indigoIterateMatches (int matcher, int query);

// Accepts a 'match' object obtained from indigoMatchSubstructure.
// Returns a new molecule which has the query highlighted.
CEXPORT int indigoHighlightedTarget (int match);

// Accepts an atom from the query, not an atom index.
//   You can use indigoGetAtom() to obtain the atom by its index.
// Returns the corresponding target atom, not an atom index. If query 
// atom doesn't match particular atom in the target (R-group or explicit 
// hydrogen) then return value is zero.
//   You can use indigoIndex() to obtain the index of the returned atom.
CEXPORT int indigoMapAtom (int handle, int atom);

// Accepts a bond from the query, not a bond index.
//   You can use indigoGetBond() to obtain the bond by its index.
// Returns the corresponding target bond, not a bond index. If query
// bond doesn't match particular bond in the target (R-group or explicit
// hydrogen) then return value is zero.
//   You can use indigoIndex() to obtain the index of the returned bond.
CEXPORT int indigoMapBond (int handle, int bond);

// Accepts a molecule from the query reaction, not a molecule index.
//   You can use indigoGetMolecule() to obtain the bond by its index.
// Returns the corresponding target molecule, not a reaction index. If query
// molecule doesn't match particular molecule in the target then return 
// value is zero.
//   You can use indigoIndex() to obtain the index of the returned molecule.
CEXPORT int indigoMapMolecule (int handle, int molecule);

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

CEXPORT int indigoTransform (int reaction, int monomers);

/* Debug functionality */

// Returns internal type of an object
CEXPORT const char * indigoDbgInternalType (int object);

// Internal breakpoint
CEXPORT void indigoDbgBreakpoint (void);

#endif
