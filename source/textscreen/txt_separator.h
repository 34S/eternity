// Emacs style mode select   -*- C -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2006 Simon Howard
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//

#ifndef TXT_SEPARATOR_H
#define TXT_SEPARATOR_H

typedef struct txt_separator_s txt_separator_t;

#include "txt_widget.h"

struct txt_separator_s
{
    txt_widget_t widget;
    char *label;
};

extern txt_widget_class_t txt_separator_class;

txt_separator_t *TXT_NewSeparator(char *label);

#endif /* #ifndef TXT_SEPARATOR_H */


