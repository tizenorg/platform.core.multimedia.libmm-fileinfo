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

#ifndef __MM_FILE_FORMAT_IMELODY_H__
#define __MM_FILE_FORMAT_IMELODY_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mmfileimelodytags {
	char *title;
	char *composer;
	int   beat;
	char *copyright;
	char *comment;
} tMMFileImelodyTagInfo;


#ifdef __cplusplus
}
#endif

#endif /*__MM_FILE_FORMAT_IMELODY_H__*/

