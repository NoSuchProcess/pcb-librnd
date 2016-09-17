/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* prototypes for undo routines */

#ifndef	PCB_UNDO_H
#define	PCB_UNDO_H

#include "global.h"

#define DRAW_FLAGS	(PCB_FLAG_RAT | PCB_FLAG_SELECTED \
			| PCB_FLAG_HIDENAME | PCB_FLAG_HOLE | PCB_FLAG_OCTAGON | PCB_FLAG_FOUND | PCB_FLAG_CLEARLINE)

											/* different layers */

int Undo(pcb_bool);
int Redo(pcb_bool);
void IncrementUndoSerialNumber(void);
void SaveUndoSerialNumber(void);
void RestoreUndoSerialNumber(void);
void ClearUndoList(pcb_bool);
void MoveObjectToRemoveUndoList(int, void *, void *, void *);
void AddObjectToRemovePointUndoList(int, void *, void *, Cardinal);
void AddObjectToInsertPointUndoList(int, void *, void *, void *);
void AddObjectToRemoveContourUndoList(int, LayerType *, PolygonType *);
void AddObjectToInsertContourUndoList(int, LayerType *, PolygonType *);
void AddObjectToMoveUndoList(int, void *, void *, void *, Coord, Coord);
void AddObjectToChangeNameUndoList(int, void *, void *, void *, char *);
void AddObjectToChangePinnumUndoList(int, void *, void *, void *, char *);
void AddObjectToRotateUndoList(int, void *, void *, void *, Coord, Coord, BYTE);
void AddObjectToCreateUndoList(int, void *, void *, void *);
void AddObjectToMirrorUndoList(int, void *, void *, void *, Coord);
void AddObjectToMoveToLayerUndoList(int, void *, void *, void *);
void AddObjectToFlagUndoList(int, void *, void *, void *);
void AddObjectToSizeUndoList(int, void *, void *, void *);
void AddObjectTo2ndSizeUndoList(int, void *, void *, void *);
void AddObjectToClearSizeUndoList(int, void *, void *, void *);
void AddObjectToMaskSizeUndoList(int, void *, void *, void *);
void AddObjectToChangeAnglesUndoList(int, void *, void *, void *);
void AddObjectToClearPolyUndoList(int, void *, void *, void *, pcb_bool);
void AddLayerChangeToUndoList(int, int);
void AddNetlistLibToUndoList(LibraryTypePtr);
void LockUndo(void);
void UnlockUndo(void);
pcb_bool Undoing(void);

/* Publish actions - these may be useful for other actions */
int ActionUndo(int argc, const char **argv, Coord x, Coord y);
int ActionRedo(int argc, const char **argv, Coord x, Coord y);
int ActionAtomic(int argc, const char **argv, Coord x, Coord y);

#endif
