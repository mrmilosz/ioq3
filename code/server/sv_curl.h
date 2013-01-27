/*
===========================================================================
Copyright (C) 2013 Milosz Kosmider (milosz@milosz.ca)

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// sv_curl.h

#ifndef __SVCURL_H__
#define __SVCURL_H__

#include "../qcommon/cm_curl.h"

qboolean SV_cURL_Init( void );
void SV_cURL_Shutdown( void );
void SV_cURL_Cleanup( void );
void SV_cURL_BeginDownload( const char * );
void SV_cURL_PerformDownload( void );

#endif // __SVCURL_H__
