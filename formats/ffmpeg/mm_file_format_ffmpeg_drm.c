/*
 * libmm-fileinfo
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Haejeong Kim <backto.kim@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdlib.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <drm-service.h>
#include "mm_file_utils.h"
#include "mm_debug.h"

typedef struct
{
    DRM_FILE_HANDLE hfile;
    long long offset;
    long long fsize;
} MMFileDRMHandle;

static int mmfile_drm_close(URLContext* h)
{
    MMFileDRMHandle *drmHandle = NULL;

    if (!h || !h->priv_data)
    {
        debug_error ("invalid para\n");
        return MMFILE_UTIL_FAIL;
    }

    drmHandle = h->priv_data;

    if (drmHandle)
    {
        if (drmHandle->hfile)
        {
        drm_svc_close_file (drmHandle->hfile);
            drmHandle->hfile = NULL;
        }

        mmfile_free (drmHandle);
        h->priv_data = NULL;
    }

    return 0;
}

static int mmfile_drm_open(URLContext *h, const char *pseudofilename, int flags)
{
    MMFileDRMHandle *drmHandle = NULL;
    DRM_BOOL res = DRM_TRUE;
    int ret = 0;

    pseudofilename += strlen(h->prot->name) + 3; /* :// */

    res = drm_svc_is_drm_file (pseudofilename);
    if (DRM_FALSE == res)
    {
        debug_error ("error: %s is not DRM file\n", pseudofilename);
        return -2;
    }

    /* Checks the DRM file type (supports only for OMA) if it is DRM */
	if(drm_svc_get_drm_type(pseudofilename) != DRM_FILE_TYPE_OMA)
	{
		debug_error ("error: %s is not OMA DRM file\n", pseudofilename);
		return -2;
	}

    drmHandle = mmfile_malloc (sizeof(MMFileDRMHandle));
    if (NULL == drmHandle)
    {
        debug_error ("error: drm_svc_get_drm_type\n");
        return -2;
    }

    drmHandle->hfile = NULL;
    drmHandle->offset = 0;

    res = drm_svc_open_file(pseudofilename, DRM_PERMISSION_PLAY, &drmHandle->hfile);
    if (DRM_RESULT_SUCCESS != res)
    {
        debug_error ("error: drm_svc_open_file\n");
        ret = -2;
        goto exception;
    }

    if (DRM_RESULT_SUCCESS != drm_svc_seek_file(drmHandle->hfile, 0, DRM_SEEK_END))
    {
        debug_error ("error: drm_svc_seek_file\n");
        ret = -2;
        goto exception;
    }

    drmHandle->fsize = drm_svc_tell_file(drmHandle->hfile);

    if (DRM_RESULT_SUCCESS != drm_svc_seek_file(drmHandle->hfile, 0, DRM_SEEK_SET))
    {
        debug_error ("error: drm_svc_seek_file\n");
        ret = -2;
        goto exception;

    }

    h->priv_data = (void *) drmHandle;
    h->is_streamed = 0; /*FALSE*/
    h->max_packet_size = 0;

    return 0;
    
exception:
    if (drmHandle)
    {
        if (drmHandle->hfile)
        {
            drm_svc_close_file (drmHandle->hfile);
        }
        
        mmfile_free(drmHandle);
        h->priv_data = NULL;
    }
    return ret;
    
}

static int mmfile_drm_read(URLContext *h, unsigned char *buf, int size)
{
    unsigned int readSize = 0;
    MMFileDRMHandle *drmHandle = h->priv_data;

    if (drmHandle)
    {
        drm_svc_read_file (drmHandle->hfile, buf, size, &readSize);
        drmHandle->offset += readSize;
        return readSize;
    }

    return 0;
}

static int mmfile_drm_write(URLContext *h, unsigned char *buf, int size)
{
    debug_warning ("Permission Deny: DRM writing\n");
    return 0;
}

static long long mmfile_drm_seek(URLContext *h, long long pos, int whence)
{
	MMFileDRMHandle *drmHandle = h->priv_data;
	DRM_RESULT res = 0;
	DRM_FILE_SEEK_MODE drm_whence;

	if (drmHandle) {
		#ifdef __MMFILE_TEST_MODE__
		debug_msg ("handle:%p, pos:%lld, whence:%d\n", h, pos, whence);
		#endif

		switch (whence) {
			case SEEK_SET:
				drm_whence = DRM_SEEK_SET;
				break;
			case SEEK_CUR:
				drm_whence = DRM_SEEK_CUR;
				break;
			case SEEK_END:
				drm_whence = DRM_SEEK_END;
				break;
			case AVSEEK_SIZE:	/*FFMPEG specific*/
				return drmHandle->fsize;
			default:
				debug_error ("invalid whence[%d]\n", whence);
				return -2;
		}

		res = drm_svc_seek_file (drmHandle->hfile, pos, drm_whence);
		if (DRM_RESULT_SUCCESS != res) {
			debug_error ("error: drm_svc_seek_file [%d][mode=%d]\n", res, drm_whence);
			return -2;
		}

		switch (drm_whence) {
			case DRM_SEEK_SET: {
				drmHandle->offset = pos;
				break;
			}
			case DRM_SEEK_CUR: {
				drmHandle->offset += pos;
				break;
			}
			case DRM_SEEK_END: {
				drmHandle->offset = drmHandle->fsize + pos;
				break;
			}
		}

		if (drmHandle->offset > drmHandle->fsize) {
			return -1;
		}

		return drmHandle->offset;
	}

	return -1;
}

URLProtocol MMFileDRMProtocol = {
	"drm",
	mmfile_drm_open,
	mmfile_drm_read,
	mmfile_drm_write,
	mmfile_drm_seek,
	mmfile_drm_close
};

