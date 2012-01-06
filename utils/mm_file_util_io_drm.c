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
 
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <drm-service.h>
#include "mm_debug.h"
#include "mm_file_utils.h"

typedef struct
{
    DRM_FILE_HANDLE hfile;
    long long offset;
    long long fsize;
} MMFileDRMHandle;


static int mmfile_drm_open(MMFileIOHandle *h, const char *pseudofilename, int flags)
{
    MMFileDRMHandle *drmHandle = NULL;
    DRM_BOOL res = DRM_TRUE;
    int ret = 0;

    if (!h || !pseudofilename || !h->iofunc || !h->iofunc->handleName)
    {
        debug_error ("invalid param\n");
        return MMFILE_IO_FAILED;        
    }

    pseudofilename += strlen(h->iofunc->handleName) + 3; /* :// */

    res = drm_svc_is_drm_file (pseudofilename);
    if (DRM_FALSE == res)
    {
        debug_error ("error: %s is not DRM file\n", pseudofilename);
        return MMFILE_IO_FAILED;
    }

    /* Checks the DRM file type (supports only for OMA) if it is DRM */
	if(drm_svc_get_drm_type(pseudofilename) != DRM_FILE_TYPE_OMA)
	{
		debug_error ("error: %s is not OMA DRM file\n", pseudofilename);
		return MMFILE_IO_FAILED;
	}

    drmHandle = mmfile_malloc (sizeof(MMFileDRMHandle));
    if (NULL == drmHandle)
    {
        debug_error ("error: mmfile_malloc\n");
        return MMFILE_IO_FAILED;
    }

    drmHandle->hfile = NULL;
    drmHandle->offset = 0;

    res = drm_svc_open_file(pseudofilename, DRM_PERMISSION_PLAY, &drmHandle->hfile);
    if (DRM_RESULT_SUCCESS != res)
    {
        debug_error ("error: drm_svc_open_file\n");
        ret = MMFILE_IO_FAILED;
        goto exception;
    }

    if (DRM_RESULT_SUCCESS != drm_svc_seek_file(drmHandle->hfile, 0, DRM_SEEK_END))
    {
        debug_error ("error: drm_svc_seek_file\n");
        ret = MMFILE_IO_FAILED;
        goto exception;
    }

    drmHandle->fsize = drm_svc_tell_file(drmHandle->hfile);

    if (DRM_RESULT_SUCCESS != drm_svc_seek_file(drmHandle->hfile, 0, DRM_SEEK_SET))
    {
        debug_error ("error: drm_svc_seek_file\n");
        ret = MMFILE_IO_FAILED;
        goto exception;
    }

    h->privateData = (void *) drmHandle;

    return MMFILE_IO_SUCCESS;

exception:
    if (drmHandle)
    {
        if (drmHandle->hfile)
        {
            drm_svc_close_file (drmHandle->hfile);
        }
        
        mmfile_free(drmHandle);
        h->privateData = NULL;
    }
    return ret;
}

static int mmfile_drm_read(MMFileIOHandle *h, unsigned char *buf, int size)
{
    int readSize = 0;
    MMFileDRMHandle *drmHandle =NULL;

    if (!h || !h->privateData || !buf)
    {
        debug_error ("invalid param\n");
        return MMFILE_IO_FAILED;
    }

    drmHandle = h->privateData;

    if ( DRM_RESULT_SUCCESS != drm_svc_read_file (drmHandle->hfile, buf, size, (unsigned int *)&readSize))
    {
        debug_error ("error: drm_svc_read_file\n");        
        return MMFILE_IO_FAILED;        
    }
    
    drmHandle->offset += readSize;
    
    return readSize;
}


static long long mmfile_drm_seek(MMFileIOHandle *h, long long pos, int whence)
{
	MMFileDRMHandle *drmHandle = NULL;
	DRM_RESULT res = 0;
	DRM_FILE_SEEK_MODE drm_whence;

	if (!h || !h->privateData) {
		debug_error ("invalid param\n");
		return MMFILE_IO_FAILED;
	}

	drmHandle = h->privateData;

	switch (whence) {
		case MMFILE_SEEK_SET:
			drm_whence = DRM_SEEK_SET;
			break;
		case MMFILE_SEEK_CUR:
			drm_whence = DRM_SEEK_CUR;
			break;
		case MMFILE_SEEK_END:
			drm_whence = DRM_SEEK_END;
			break;
		default:
			debug_error ("invalid whence[%d]\n", whence);
			return MMFILE_IO_FAILED;
	}

	res = drm_svc_seek_file (drmHandle->hfile, pos, drm_whence);
	if (DRM_RESULT_SUCCESS != res) {
		debug_error ("error: drm_svc_seek_file [%d][mode=%d]\n", res, drm_whence);
		return MMFILE_IO_FAILED;
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
		return MMFILE_IO_FAILED;
	}

	return drmHandle->offset;
}

static long long mmfile_drm_tell (MMFileIOHandle *h)
{
    MMFileDRMHandle *drmHandle = NULL;
    
    if (!h || !h->privateData)
    {
        debug_error ("invalid param\n");
        return MMFILE_IO_FAILED;
    }

    drmHandle = h->privateData;
    
    drmHandle->offset = drm_svc_tell_file (drmHandle->hfile);

    return drmHandle->offset;
}

static int mmfile_drm_close(MMFileIOHandle* h)
{
    MMFileDRMHandle *drmHandle = NULL;
    
    if (!h || !h->privateData)
    {
        debug_error ("invalid param\n");
        return MMFILE_IO_FAILED;
    }

    drmHandle = h->privateData;
    
    if (drmHandle)
    {
        if (drmHandle->hfile)
        {
            drm_svc_close_file (drmHandle->hfile);
            drmHandle->hfile = NULL;
        }

    mmfile_free (drmHandle);
    h->privateData = NULL;
    }

    return MMFILE_IO_SUCCESS;
}


MMFileIOFunc mmfile_drm_io_handler = {
	"drm",
	mmfile_drm_open,
	mmfile_drm_read,
	NULL,
	mmfile_drm_seek,
	mmfile_drm_tell,
	mmfile_drm_close
};

