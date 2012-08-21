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
#include <stdbool.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include "mm_debug.h"
#include "mm_file_formats.h"
#include "mm_file_utils.h"
#include "mm_file_format_frame.h"

#define MILLION 1000000

int mmfile_format_get_frame(const char* path, double timestamp, bool keyframe, unsigned char **data, int *size, int *width, int *height)
{
	int i;
	int ret;
	int videoStream = -1;
	int frameFinished;
	double pos = timestamp;
	bool find = false ;
	bool first_seek = true;
	int64_t pts = 0;
	AVFormatContext *pFormatCtx = NULL;
	AVCodecContext *pVideoCodecCtx;
	AVCodec *pCodec = NULL;
	AVFrame *pFrame = NULL, *pFrameRGB = NULL;
	AVPacket packet;

	if (!size || !width || !height) {
		return MMFILE_FORMAT_FAIL;
	}

	av_register_all();

	/* Open video file */
	if(avformat_open_input(&pFormatCtx, path, NULL, NULL) != 0) {
		debug_error("error : avformat_open_input failed");
		return MMFILE_FORMAT_FAIL; /* Couldn't open file */
	}

	/* Retrieve stream information */
	if(av_find_stream_info(pFormatCtx) < 0) {
		debug_error("error : av_find_stream_info failed");
		return MMFILE_FORMAT_FAIL; /* Couldn't find stream information */
	}

	/* Find the first video stream */
	for(i = 0; i < pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}

	if(videoStream == -1) {
		debug_error("error : videoStream == -1");
		return MMFILE_FORMAT_FAIL; /* Didn't find a video stream */
	}

	/* Get a pointer to the codec context for the video stream */
	pVideoCodecCtx=pFormatCtx->streams[videoStream]->codec;

	/* Find the decoder for the video stream */
	pCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
	if(pCodec == NULL) {
		debug_error("error : Unsupported codec");
		return MMFILE_FORMAT_FAIL; /* Codec not found */
	}

	/* Open codec */
	if(avcodec_open(pVideoCodecCtx, pCodec) < 0) {
		debug_error("error : avcodec_open failed");
		return MMFILE_FORMAT_FAIL; /*Could not open codec */
	}

	/* Storing Data  */
	/* Allocate video frame */
	pFrame = avcodec_alloc_frame();
	if(pFrame == NULL) {
		debug_error ("error: pFrame is NULL\n");
		ret = MMFILE_FORMAT_FAIL;
		goto exception;
	}

	/* Allocate an AVFrame structure */
	pFrameRGB = avcodec_alloc_frame();
	if(pFrameRGB == NULL) {
		debug_error ("error: pFrameRGB is NULL\n");
		ret = MMFILE_FORMAT_FAIL;
		goto exception;
	}

	/* Seeking */
	AVStream *pStream = pFormatCtx->streams[videoStream];
	double duration = (double) pFormatCtx->duration / AV_TIME_BASE;
	if (duration <= 0) {
		double tmpDuration = 0.0;

		if (pStream->codec->bit_rate > 0 && pFormatCtx->file_size > 0) {
			if (pStream->codec->bit_rate >= 8)
				tmpDuration = 0.9 * pFormatCtx->file_size / (pStream->codec->bit_rate / 8);

			if (tmpDuration > 0)
				duration = tmpDuration;
		}
	}
	duration = duration * MILLION;
	if (duration <= 0 || duration <= pos) {
		debug_error("duration error");
		ret = MMFILE_FORMAT_FAIL;
		goto exception;
	}

	if (keyframe)
		av_seek_frame(pFormatCtx, -1, pos, 0);
	else
		av_seek_frame(pFormatCtx, -1, pos, AVSEEK_FLAG_ANY);

	/* Reading Data */
	int64_t tmpPts;

	while(av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if(packet.stream_index == videoStream) {
			/* Decode video frame*/
			avcodec_decode_video2(pVideoCodecCtx, pFrame, &frameFinished, &packet);
			if (packet.flags & AV_PKT_FLAG_KEY) {
				if (first_seek) {
					//This is first seeking.
					find = true;
				}
			} else {
				if (first_seek) {
					pts = (packet.pts == AV_NOPTS_VALUE) ? (packet.dts * av_q2d(pStream->time_base)) : packet.pts;
					first_seek = false;

					av_seek_frame(pFormatCtx, -1, pos, AVSEEK_FLAG_BACKWARD);
				} else {
					tmpPts = (packet.pts == AV_NOPTS_VALUE) ? (packet.dts * av_q2d(pStream->time_base)) : packet.pts;
					if (pts == tmpPts)
						find = true;
				}
			}

			if (find)
				break;
		}

		/* Free the packet that was allocated by av_read_frame*/
		av_free_packet(&packet);
	}

	/* Did we get a video frame?*/
	if(frameFinished && find) {
		debug_msg("FIND");

		/* return frame infromations*/
		if((pVideoCodecCtx->width == 0) || (pVideoCodecCtx->height == 0)) {
			*width = pVideoCodecCtx->coded_width;
			*height = pVideoCodecCtx->coded_height;
		} else {
			*width = pVideoCodecCtx->width;
			*height = pVideoCodecCtx->height;
		}

		*size = avpicture_get_size(PIX_FMT_RGB24, *width, *height);
		*data = mmfile_malloc (*size);
		if (NULL == *data) {
			debug_error ("error: avpicture_get_size. [%d]\n", size);
			ret = MMFILE_FORMAT_FAIL;
			goto exception;
		}

		debug_msg("size : %d", *size);
		debug_msg("width : %d", *width);
		debug_msg("height : %d", *height);
		debug_msg("data : %x", *data);

		ret = avpicture_fill ((AVPicture *)pFrameRGB, *data, PIX_FMT_RGB24, *width, *height);
		if (ret < 0) {
			debug_error ("error: avpicture_fill fail. errcode = 0x%08X\n", ret);
			ret = MMFILE_FORMAT_FAIL;
			goto exception;
		}

#ifdef __MMFILE_FFMPEG_V085__
		static struct SwsContext *img_convert_ctx;

		img_convert_ctx = sws_getContext (*width, *height, pVideoCodecCtx->pix_fmt,
		                          *width, *height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

		if (NULL == img_convert_ctx) {
			debug_error ("failed to get img convet ctx\n");
			ret = MMFILE_FORMAT_FAIL;
			goto exception;
		}

		ret = sws_scale (img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize,
		     0, *height, pFrameRGB->data, pFrameRGB->linesize);
		if ( ret < 0 ) {
			debug_error ("failed to convet image\n");
			ret = MMFILE_FORMAT_FAIL;
			goto exception;
		}

		sws_freeContext(img_convert_ctx);
#else
		ret = img_convert ((AVPicture *)pFrameRGB, PIX_FMT_RGB24, (AVPicture*)pFrame, pVideoCodecCtx->pix_fmt, *width, *height);
		if ( ret < 0 ) {
			debug_error ("failed to convet image\n");
			ret = MMFILE_FORMAT_FAIL;
			goto exception;
		}
#endif

		if (pFrame)			av_free (pFrame);
		if (pFrameRGB)		av_free (pFrameRGB);
		if (pVideoCodecCtx)	avcodec_close(pVideoCodecCtx);
	}

	return MMFILE_FORMAT_SUCCESS;

exception:
	if (pVideoCodecCtx)		avcodec_close (pVideoCodecCtx);
	if (*data)	{ mmfile_free (*data); *data = NULL; }
	if (pFrame)				av_free (pFrame);
	if (pFrameRGB)			av_free (pFrameRGB);

	return ret;
  }
