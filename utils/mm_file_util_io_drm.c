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
#include <drm_client.h>
#include <drm_trusted_client.h>
#include "mm_file_debug.h"
#include "mm_file_utils.h"

typedef struct {
	DRM_DECRYPT_HANDLE hfile;
	long long offset;
	long long fsize;
} MMFileDRMHandle;


static int mmfile_drm_open(MMFileIOHandle *h, const char *pseudofilename, int flags)
{
	MMFileDRMHandle *drmHandle = NULL;
	drm_bool_type_e res = DRM_TRUE;
	drm_file_type_e file_type = DRM_TYPE_UNDEFINED;
	drm_trusted_open_decrypt_info_s open_input_data;
	drm_trusted_open_decrypt_resp_data_s open_output_data;
	drm_trusted_seek_decrypt_info_s seek_input_data;
	drm_trusted_tell_decrypt_resp_data_s tell_output_data;
	drm_trusted_set_consumption_state_info_s state_input_data;
	int ret = 0;

	if (!h || !pseudofilename || !h->iofunc || !h->iofunc->handleName) {
		debug_error("invalid param\n");
		return MMFILE_IO_FAILED;
	}

	pseudofilename += strlen(h->iofunc->handleName) + 3; /* :// */

	/* Check is DRM */
	ret = drm_is_drm_file(pseudofilename, &res);
	if (DRM_FALSE == res) {
		debug_error("error: %s is not DRM file. ret[%x]\n", pseudofilename, ret);
		return MMFILE_IO_FAILED;
	}
	if (ret != DRM_RETURN_SUCCESS) {
		debug_error("error: %s is not DRM file. ret[%x]\n", pseudofilename, ret);
		return MMFILE_IO_FAILED;
	}

	/* Checks the DRM file type (supports only for OMA) if it is DRM */
	ret = drm_get_file_type(pseudofilename, &file_type);
	if (ret != DRM_RETURN_SUCCESS) {
		debug_error("error: %s is not DRM file. ret[%x]\n", pseudofilename, ret);
		return MMFILE_IO_FAILED;
	}
	if ((file_type != DRM_TYPE_OMA_V1) && (file_type != DRM_TYPE_OMA_V2)) {
		debug_error("error: %s is not DRM file. file_type[%d]\n", pseudofilename, file_type);
		return MMFILE_IO_FAILED;
	}

	/* Set DRM handle*/
	drmHandle = mmfile_malloc(sizeof(MMFileDRMHandle));
	if (NULL == drmHandle) {
		debug_error("error: mmfile_malloc\n");
		return MMFILE_IO_FAILED;
	}

	drmHandle->hfile = NULL;
	drmHandle->offset = 0;

	/* Open DRM File*/
	memset(&open_input_data, 0x0, sizeof(drm_trusted_open_decrypt_info_s));
	memset(&open_output_data, 0x0, sizeof(drm_trusted_open_decrypt_resp_data_s));

	memcpy(open_input_data.filePath, pseudofilename, strlen(pseudofilename));
	if (file_type == DRM_TYPE_OMA_V1)	open_input_data.file_type = DRM_TRUSTED_TYPE_OMA_V1;
	else	open_input_data.file_type = DRM_TRUSTED_TYPE_OMA_V2;
	open_input_data.permission = DRM_TRUSTED_PERMISSION_TYPE_DISPLAY;
	/*open_input_data.operation_callback.callback; */

	ret = drm_trusted_open_decrypt_session(&open_input_data, &open_output_data, &drmHandle->hfile);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		debug_error("error: drm_trusted_open_decrypt_session() [%x]\n", ret);
		ret = MMFILE_IO_FAILED;
		goto exception;
	}

	/* Seek End*/
	memset(&seek_input_data, 0x0, sizeof(drm_trusted_seek_decrypt_info_s));
	seek_input_data.offset = 0;
	seek_input_data.seek_mode = DRM_SEEK_END; /* Set cursor to end */

	ret = drm_trusted_seek_decrypt_session(drmHandle->hfile, &seek_input_data);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		debug_error("error: drm_trusted_seek_decrypt_session() [%x]\n", ret);
		ret = MMFILE_IO_FAILED;
		goto exception;
	}

	/* Tell to get the file size */
	memset(&tell_output_data, 0x0, sizeof(drm_trusted_tell_decrypt_resp_data_s));
	ret = drm_trusted_tell_decrypt_session(drmHandle->hfile, &tell_output_data);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		debug_error("error: drm_trusted_tell_decrypt_session() [%x]\n", ret);
		ret = MMFILE_IO_FAILED;
		goto exception;
	}

	drmHandle->fsize = tell_output_data.offset;

	/* Seek Set*/
	memset(&seek_input_data, 0x0, sizeof(drm_trusted_seek_decrypt_info_s));
	seek_input_data.offset = 0;
	seek_input_data.seek_mode = DRM_SEEK_SET;
	ret = drm_trusted_seek_decrypt_session(drmHandle->hfile, &seek_input_data);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		debug_error("error: drm_trusted_seek_decrypt_session() [%x]\n", ret);
		ret = MMFILE_IO_FAILED;
		goto exception;
	}

	h->privateData = (void *) drmHandle;

	/* Set Consumption state*/
	memset(&state_input_data, 0x0, sizeof(drm_trusted_set_consumption_state_info_s));
	state_input_data.state = DRM_CONSUMPTION_PREVIEW;
	ret = drm_trusted_set_decrypt_state(drmHandle->hfile, &state_input_data);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		debug_error("error: drm_trusted_set_decrypt_state [%x]\n", ret);
		ret = MMFILE_IO_FAILED;
		goto exception;
	}

#ifdef __MMFILE_TEST_MODE__
	debug_msg("drm open success===============\n");
#endif

	return MMFILE_IO_SUCCESS;

exception:
	if (drmHandle) {
		if (drmHandle->hfile) {
			drm_trusted_close_decrypt_session(&drmHandle->hfile);
		}

		mmfile_free(drmHandle);
		h->privateData = NULL;
	}

	return ret;
}

static int mmfile_drm_read(MMFileIOHandle *h, unsigned char *buf, int size)
{
	MMFileDRMHandle *drmHandle = NULL;
	drm_trusted_payload_info_s read_input_data;
	drm_trusted_read_decrypt_resp_data_s read_output_data;
	int ret = 0;

	if (!h || !h->privateData || !buf) {
		debug_error("invalid param\n");
		return MMFILE_IO_FAILED;
	}

	drmHandle = h->privateData;

	memset(&read_input_data, 0x0, sizeof(drm_trusted_payload_info_s));
	memset(&read_output_data, 0x0, sizeof(drm_trusted_read_decrypt_resp_data_s));

	read_input_data.payload_data = buf;
	read_input_data.payload_data_len = (unsigned int)size;
	read_input_data.payload_data_output = buf;

	ret = drm_trusted_read_decrypt_session(drmHandle->hfile, &read_input_data, &read_output_data);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		debug_error("error: drm_trusted_read_decrypt_session() [%x]\n", ret);
		return MMFILE_IO_FAILED;
	}

	drmHandle->offset += read_output_data.read_size;

	return read_output_data.read_size;
}


static long long mmfile_drm_seek(MMFileIOHandle *h, long long pos, int whence)
{
	MMFileDRMHandle *drmHandle = NULL;
	drm_trusted_seek_mode_e drm_whence;
	drm_trusted_seek_decrypt_info_s seek_input_data;
	int ret = 0;

	if (!h || !h->privateData) {
		debug_error("invalid param\n");
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
			debug_error("invalid whence[%d]\n", whence);
			return MMFILE_IO_FAILED;
	}

	memset(&seek_input_data, 0x0, sizeof(drm_trusted_seek_decrypt_info_s));
	seek_input_data.offset = pos;
	seek_input_data.seek_mode = drm_whence;

	ret = drm_trusted_seek_decrypt_session(drmHandle->hfile, &seek_input_data);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		debug_error("error: drm_trusted_seek_decrypt_session() [%x] [mode=%d]\n", ret, drm_whence);
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

static long long mmfile_drm_tell(MMFileIOHandle *h)
{
	MMFileDRMHandle *drmHandle = NULL;
	drm_trusted_tell_decrypt_resp_data_s tell_output_data;
	int ret = 0;

	if (!h || !h->privateData) {
		debug_error("invalid param\n");
		return MMFILE_IO_FAILED;
	}

	drmHandle = h->privateData;

	memset(&tell_output_data, 0x0, sizeof(drm_trusted_tell_decrypt_resp_data_s));
	ret = drm_trusted_tell_decrypt_session(drmHandle->hfile, &tell_output_data);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		debug_error("error: drm_trusted_tell_decrypt_session() [%x]\n", ret);
		return MMFILE_IO_FAILED;
	}

	drmHandle->offset = tell_output_data.offset;

	return drmHandle->offset;
}

static int mmfile_drm_close(MMFileIOHandle *h)
{
	MMFileDRMHandle *drmHandle = NULL;
	drm_trusted_set_consumption_state_info_s state_input_data;
	int ret = 0;

	if (!h || !h->privateData) {
		debug_error("invalid param\n");
		return MMFILE_IO_FAILED;
	}

	drmHandle = h->privateData;

	/* Set Consumption state*/
	memset(&state_input_data, 0x0, sizeof(drm_trusted_set_consumption_state_info_s));
	state_input_data.state = DRM_CONSUMPTION_STOPPED;
	ret = drm_trusted_set_decrypt_state(drmHandle->hfile, &state_input_data);
	if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
		debug_error("error: drm_trusted_set_decrypt_state() [%x]\n", ret);
	} else {
#ifdef __MMFILE_TEST_MODE__
		debug_msg("Success : drm_trusted_set_decrypt_state\n");
#endif
	}

	if (drmHandle) {
		if (drmHandle->hfile) {
			ret = drm_trusted_close_decrypt_session(&drmHandle->hfile);
			if (ret != DRM_TRUSTED_RETURN_SUCCESS) {
				debug_error("error: drm_trusted_close_decrypt_session() [%x]\n", ret);
			} else {
#ifdef __MMFILE_TEST_MODE__
				debug_msg("Success : drm_trusted_close_decrypt_session\n");
#endif
			}

			drmHandle->hfile = NULL;
		}

		mmfile_free(drmHandle);
		h->privateData = NULL;
	}

#ifdef __MMFILE_TEST_MODE__
	debug_msg("drm close success===============\n");
#endif

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
