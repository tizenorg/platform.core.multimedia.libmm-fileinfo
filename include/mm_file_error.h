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

#ifndef __MM_FILE_ERROR_H__
#define __MM_FILE_ERROR_H__

#ifdef __cplusplus
	 extern "C" {
#endif

#define FILEINFO_ERROR_NONE					0	/**< No Error */
#define FILEINFO_ERROR_INVALID_ARGUMENT		-1	/**< Invalid argument */
#define FILEINFO_ERROR_FILE_NOT_FOUND		-2	/**< Cannot find file */
#define FILEINFO_ERROR_ATTR_NOT_EXIST		-3	/**< Attribute doesn't exist. */
#define FILEINFO_ERROR_FILE_INTERNAL		-4	/**< Internal error */

#ifdef __cplusplus
	 }
#endif

#endif	/* __MM_FILE_ERROR_H__ */
