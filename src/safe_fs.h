 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
  */

/* Wrap standard file system calls, giving the user a chance to control
   where pcb-rnd may go on the file system */

#ifndef PCB_SAFE_FS_H
#define PCB_SAFE_FS_H

FILE *pcb_fopen(const char *path, const char *mode);
FILE *pcb_popen(const char *cmd, const char *mode);
int pcb_system(const char *cmd);
int pcb_remove(const char *path);
int pcb_rename(const char *old_path, const char *new_path);

#endif
