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
#ifdef MMFILE_FORMAT_DEBUG_DUMP
static void _save_pgm (unsigned char *buf,int wrap, int xsize,int ysize,char *filename);

static void _save_pgm (unsigned char *buf,int wrap, int xsize,int ysize,char *filename)
{
	FILE *f;
	int i;

	f = fopen(filename,"w");
	if (f) {
		fprintf (f,"P5\n%d %d\n%d\n",xsize,ysize,255);
		for (i = 0; i < ysize; i++)
			fwrite (buf + i * wrap, 1, xsize, f);
		fclose (f);
	}
}
#endif

int mmfile_format_get_frame(const char* path, double timestamp, bool is_accurate, unsigned char **frame, int *size, int *width, int *height)
{
	int i = 0;
	int ret = MMFILE_FORMAT_SUCCESS;
	int videoStream = -1;
	int frameFinished = 0;
	double pos = timestamp;
	bool find = false ;
	bool first_seek = true;
	int64_t pts = 0;
	AVFormatContext *pFormatCtx = NULL;
	AVCodecContext *pVideoCodecCtx = NULL;
	AVCodec *pVideoCodec = NULL;
	AVFrame *pFrame = NULL, *pFrameRGB = NULL;
	AVPacket packet;
	int len = 0;
	int key_detected = 0;

	if (!size || !width || !height) {
		return MMFILE_FORMAT_FAIL;
	}

	av_register_all();

	/* Open video file */
	if(avformat_open_input(&pFormatCtx, path, NULL, NULL) != 0) {
		debug_error("error : avformat_open_input failed");
		return MMFILE_FORMAT_FAIL; /* Couldn't open file */
	}

	if (!pFormatCtx) {
		debug_warning ("failed to find av stream. maybe corrupted data.\n");
		ret = MMFILE_FORMAT_FAIL;
		goto exception;
	}

	/* Retrieve stream information */
#ifdef __MMFILE_FFMPEG_V100__
	if(avformat_find_stream_info(pFormatCtx, NULL) < 0) {
#else
	if(av_find_stream_info(pFormatCtx) < 0) {
#endif
		debug_error("error : av_find_stream_info failed");
		ret = MMFILE_FORMAT_FAIL;
		goto exception; /* Couldn't find stream information */
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
		ret = MMFILE_FORMAT_FAIL;
		goto exception; /* Didn't find a video stream */
	}

	/* Get a pointer to the codec context for the video stream */
	pVideoCodecCtx=pFormatCtx->streams[videoStream]->codec;
	if (pVideoCodecCtx == NULL) {
		debug_error ("invalid param\n");
		ret = MMFILE_FORMAT_FAIL;
		goto exception;
	}

	/* Find the decoder for the video stream */
	pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
	if(pVideoCodec == NULL) {
		debug_error("error : Unsupported codec");
		ret = MMFILE_FORMAT_FAIL;
		goto exception; /* Codec not found */
	}

	/* Open codec */
#ifdef __MMFILE_FFMPEG_V100__
	if(avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0) {
#else
	if(avcodec_open(pVideoCodecCtx, pVideoCodec) < 0) {
#endif
		debug_error("error : avcodec_open failed");
		ret = MMFILE_FORMAT_FAIL;
		goto exception;; /*Could not open codec */
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
#if 0
	if (duration <= 0) {
		double tmpDuration = 0.0;

		if (pStream->codec->bit_rate > 0 && pFormatCtx->file_size > 0) {
			if (pStream->codec->bit_rate >= 8)
				tmpDuration = 0.9 * pFormatCtx->file_size / (pStream->codec->bit_rate / 8);

			if (tmpDuration > 0)
				duration = tmpDuration;
		}
	}
#endif
	duration = duration * MILLION;
	if ((duration <= 0) ||(duration <= pos)) {
		debug_error("duration error duration[%f] pos[%f]", duration, pos);
		ret = MMFILE_FORMAT_FAIL;
		goto exception;
	}

	if (is_accurate)
		av_seek_frame(pFormatCtx, -1, pos, AVSEEK_FLAG_ANY);
	else
		av_seek_frame(pFormatCtx, -1, pos, AVSEEK_FLAG_BACKWARD);

	/* Reading Data */
	int64_t tmpPts = 0;
#ifdef __MMFILE_TEST_MODE__
	int idx = 0;
#endif

	av_init_packet(&packet);

	while(av_read_frame(pFormatCtx, &packet) >= 0) {
		frameFinished = 0;

		// Is this a packet from the video stream?
		if(packet.stream_index == videoStream) {
#ifdef __MMFILE_TEST_MODE__
			debug_msg("find Video Stream+++++++[%2d]", idx++);
#endif
			/* Decode video frame*/
			len = avcodec_decode_video2(pVideoCodecCtx, pFrame, &frameFinished, &packet);
			if (len < 0) {
					debug_warning ("Error while decoding frame");
			} else if ((packet.flags & AV_PKT_FLAG_KEY) || (key_detected == 1)) {

				key_detected = 0;

				if (first_seek || !is_accurate) {
					/* This is first seeking or not accurate mode.
					Sometimes flag is AV_PKT_FLAG_KEY but frameFinished is NULL.
					first_seek is used when accurate mode and when time stamp's frame is not key frame.
					Go back to previousto Key frame and decode frame until time stamp's frame*/

					if (frameFinished) {
						if(pFrame->key_frame) {
							#ifdef __MMFILE_TEST_MODE__
							debug_msg("find Video Stream+++++++Find key frame");
							#endif

							find = true;
						} else {
						#ifdef __MMFILE_TEST_MODE__
							debug_msg("find Video Stream+++++++skip (not key frame)");
						#endif
						}
					} else {
						#ifdef __MMFILE_TEST_MODE__
						debug_msg("find Video Stream+++++++Find key but no frame");
						#endif
						key_detected = 1;
					}
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

			if(find && frameFinished) {
				#ifdef MMFILE_FORMAT_DEBUG_DUMP
				char pgm_name[256] = {0,};
				sprintf (pgm_name, "./key_%d.ppm", (int)pos/1000);
				_save_pgm (pFrame->data[0], pFrame->linesize[0], pVideoCodecCtx->width, pVideoCodecCtx->height, pgm_name);
				#endif
				break;
			}
		}

		/* Free the packet that was allocated by av_read_frame*/
		av_free_packet(&packet);
		av_init_packet(&packet);
	}

	/*free pkt after loop breaking*/
	av_free_packet (&packet);

	/* Did we get a video frame?*/
	if(frameFinished && find) {

#ifdef __MMFILE_TEST_MODE__
		debug_msg("Find Frame");
#endif
		/* return frame infromations*/
		if((pVideoCodecCtx->width == 0) || (pVideoCodecCtx->height == 0)) {
			*width = pVideoCodecCtx->coded_width;
			*height = pVideoCodecCtx->coded_height;
		} else {
			*width = pVideoCodecCtx->width;
			*height = pVideoCodecCtx->height;
		}

		*size = avpicture_get_size(PIX_FMT_RGB24, *width, *height);
		*frame = mmfile_malloc (*size);
		if (NULL == *frame) {
			debug_error ("error: avpicture_get_size. [%d]\n", size);
			ret = MMFILE_FORMAT_FAIL;
			goto exception;
		}

#ifdef __MMFILE_TEST_MODE__
		debug_msg("size : %d", *size);
		debug_msg("width : %d", *width);
		debug_msg("height : %d", *height);
		debug_msg("frame : %x", *frame);
#endif
		ret = avpicture_fill ((AVPicture *)pFrameRGB, *frame, PIX_FMT_RGB24, *width, *height);
		if (ret < 0) {
			debug_error ("error: avpicture_fill fail. errcode = 0x%08X\n", ret);
			ret = MMFILE_FORMAT_FAIL;
			goto exception;
		}

#ifdef __MMFILE_FFMPEG_V085__
		struct SwsContext *img_convert_ctx = NULL;

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
			sws_freeContext(img_convert_ctx);
			img_convert_ctx = NULL;
			ret = MMFILE_FORMAT_FAIL;
			goto exception;
		}

		sws_freeContext(img_convert_ctx);
		img_convert_ctx = NULL;
#else
		ret = img_convert ((AVPicture *)pFrameRGB, PIX_FMT_RGB24, (AVPicture*)pFrame, pVideoCodecCtx->pix_fmt, *width, *height);
		if ( ret < 0 ) {
			debug_error ("failed to convet image\n");
			ret = MMFILE_FORMAT_FAIL;
			goto exception;
		}
#endif

	}
	else
	{
		debug_error("Not Found Proper Frame[%d][%d]", frameFinished, find);
		ret = MMFILE_FORMAT_FAIL;
		goto exception;
	}

	if (pFrame)			av_free (pFrame);
	if (pFrameRGB)		av_free (pFrameRGB);
	if (pVideoCodecCtx)	avcodec_close(pVideoCodecCtx);
	/* Close video file */
	if (pFormatCtx) avformat_close_input(&pFormatCtx);

	return MMFILE_FORMAT_SUCCESS;

exception:
	if (*frame)			{ mmfile_free (*frame); *frame = NULL; }
	if (pFrame)			av_free (pFrame);
	if (pFrameRGB)		av_free (pFrameRGB);
	if (pVideoCodecCtx) 	avcodec_close (pVideoCodecCtx);
	/* Close video file */
	if (pFormatCtx) 		avformat_close_input(&pFormatCtx);

	return ret;
  }
