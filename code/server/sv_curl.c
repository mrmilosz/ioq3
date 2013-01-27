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
// sv_curl.c

#ifdef USE_CURL

#include "sv_curl.h"

#include "server.h"

/*
=================
SV_cURL_Init
=================
*/
qboolean SV_cURL_Init()
{
	sv.cURLEnabled = CM_cURL_Init();
	return sv.cURLEnabled;
}

/*
=================
SV_cURL_Shutdown
=================
*/
void SV_cURL_Shutdown( void )
{
	SV_cURL_Cleanup();
	CM_cURL_Shutdown();
}

/*
=================
SV_cURL_Cleanup
=================
*/
void SV_cURL_Cleanup( void )
{
	if(sv.downloadCURLM) {
		if(sv.downloadCURL) {
			qcurl_multi_remove_handle(sv.downloadCURLM,
				sv.downloadCURL);
			qcurl_easy_cleanup(sv.downloadCURL);
		}
		qcurl_multi_cleanup(sv.downloadCURLM);
		sv.downloadCURLM = NULL;
		sv.downloadCURL = NULL;
	}
	else if(sv.downloadCURL) {
		qcurl_easy_cleanup(sv.downloadCURL);
		sv.downloadCURL = NULL;
	}
}
/* TODO
static int SV_cURL_CallbackProgress( void *dummy, double dltotal, double dlnow,
	double ultotal, double ulnow )
{
	sv.downloadSize = (int)dltotal;
	Cvar_SetValue( "sv_downloadSize", sv.downloadSize );
	sv.downloadCount = (int)dlnow;
	Cvar_SetValue( "sv_downloadCount", sv.downloadCount );
	return 0;
}

static size_t SV_cURL_CallbackWrite(void *buffer, size_t size, size_t nmemb,
	void *stream)
{
	FS_Write( buffer, size*nmemb, ((fileHandle_t*)stream)[0] );
	return size*nmemb;
}

void SV_cURL_BeginDownload( const char *mapName )
{
	sv.cURLUsed = qtrue;
	Com_Printf("URL: %s\n", remoteURL);
	Com_DPrintf("***** SV_cURL_BeginDownload *****\n"
		"Localname: %s\n"
		"RemoteURL: %s\n"
		"****************************\n", localName, remoteURL);
	SV_cURL_Cleanup();
	Q_strncpyz(sv.downloadURL, remoteURL, sizeof(sv.downloadURL));
	Q_strncpyz(sv.downloadName, localName, sizeof(sv.downloadName));
	Com_sprintf(sv.downloadTempName, sizeof(sv.downloadTempName),
		"%s.tmp", localName);

	// Set so UI gets access to it
	Cvar_Set("sv_downloadName", localName);
	Cvar_Set("sv_downloadSize", "0");
	Cvar_Set("sv_downloadCount", "0");
	Cvar_SetValue("sv_downloadTime", sv.realtime);

	sv.downloadBlock = 0; // Starting new file
	sv.downloadCount = 0;

	sv.downloadCURL = qcurl_easy_init();
	if(!sv.downloadCURL) {
		Com_Error(ERR_DROP, "SV_cURL_BeginDownload: qcurl_easy_init() "
			"failed");
		return;
	}
	sv.download = FS_SV_FOpenFileWrite(sv.downloadTempName);
	if(!sv.download) {
		Com_Error(ERR_DROP, "SV_cURL_BeginDownload: failed to open "
			"%s for writing", sv.downloadTempName);
		return;
	}

	if(com_developer->integer)
		qcurl_easy_setopt(sv.downloadCURL, CURLOPT_VERBOSE, 1);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_URL, sv.downloadURL);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_TRANSFERTEXT, 0);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_REFERER, va("ioQ3://%s",
		NET_AdrToString(sv.serverAddress)));
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_USERAGENT, va("%s %s",
		Q3_VERSION, qcurl_version()));
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_WRITEFUNCTION,
		SV_cURL_CallbackWrite);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_WRITEDATA, &sv.download);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_NOPROGRESS, 0);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_PROGRESSFUNCTION,
		SV_cURL_CallbackProgress);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_PROGRESSDATA, NULL);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_FAILONERROR, 1);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_FOLLOWLOCATION, 1);
	qcurl_easy_setopt(sv.downloadCURL, CURLOPT_MAXREDIRS, 5);
	sv.downloadCURLM = qcurl_multi_init();	
	if(!sv.downloadCURLM) {
		qcurl_easy_cleanup(sv.downloadCURL);
		sv.downloadCURL = NULL;
		Com_Error(ERR_DROP, "SV_cURL_BeginDownload: qcurl_multi_init() "
			"failed");
		return;
	}
	qcurl_multi_add_handle(sv.downloadCURLM, sv.downloadCURL);

	if(!(sv.sv_allowDownload & DLF_NO_DISCONNECT) &&
		!sv.cURLDisconnected) {

		SV_AddReliableCommand("disconnect", qtrue);
		SV_WritePacket();
		SV_WritePacket();
		SV_WritePacket();
		sv.cURLDisconnected = qtrue;
	}
}

void SV_cURL_PerformDownload(void)
{
	CURLMcode res;
	CURLMsg *msg;
	int c;
	int i = 0;

	res = qcurl_multi_perform(sv.downloadCURLM, &c);
	while(res == CURLM_CALL_MULTI_PERFORM && i < 100) {
		res = qcurl_multi_perform(sv.downloadCURLM, &c);
		i++;
	}
	if(res == CURLM_CALL_MULTI_PERFORM)
		return;
	msg = qcurl_multi_info_read(sv.downloadCURLM, &c);
	if(msg == NULL) {
		return;
	}
	FS_FCloseFile(sv.download);
	if(msg->msg == CURLMSG_DONE && msg->data.result == CURLE_OK) {
		FS_SV_Rename(sv.downloadTempName, sv.downloadName);
		sv.downloadRestart = qtrue;
	}
	else {
		long code;

		qcurl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE,
			&code);	
		Com_Error(ERR_DROP, "Download Error: %s Code: %ld URL: %s",
			qcurl_easy_strerror(msg->data.result),
			code, sv.downloadURL);
	}

	SV_NextDownload();
}
*/
#endif // USE_CURL
