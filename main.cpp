extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
}

#include <string>
#include <iostream>

#define NON

#ifdef NON

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
        QApplication app(argc, argv);

        MainWindow w;
        w.open();

    return app.exec();
}

#endif

#ifdef DECODER
static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, const char *filename)
{
    FILE *f;
    int i;

    f = fopen(filename, "wb");

    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for(i = 0; i < ysize; ++i)
        fwrite(buf + i * wrap, 1, xsize, f);

    fclose(f);
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, const char *filename)
{
    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if(ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while(ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if(ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        printf("saving frame %3d\n", dec_ctx->frame_number);
        fflush(stdout);

        snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
        pgm_save(frame->data[0], frame->linesize[0],
                 frame->width, frame->height, buf);
    }
}

#define INBUF_SIZE 4096

int main(int argc, char *argv[])
{
    const char *filename, *outfilename;
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c = nullptr;
    FILE *f;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t data_size;
    int ret;
    AVPacket *pkt;

    if(argc <= 2) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n"
                "And check your input file is encoded by mpeg1video please.\n", argv[0]);
        exit(0);
    }

    filename = argv[1];
    outfilename = argv[2];

    pkt = av_packet_alloc();
    if(!pkt)
        exit(1);

    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if(!parser) {
        fprintf(stderr, "parser not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if(!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    //    if(codec->id == AV_CODEC_ID_H264)
    //        av_opt_set(c->priv_data, "preset", "slow", 0);

    if(avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "rb");
    if(!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if(!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    while(!feof(f)) {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, f);
        if(!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while(data_size > 0) {
            ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if(ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data      += ret;
            data_size -= ret;

            if(pkt->size)
                decode(c, frame, pkt, outfilename);
        }
    }

    decode(c, frame, NULL, outfilename);

    fclose(f);

    av_parser_close(parser);
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}

#endif
#ifdef ENCODER
static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile)
{
    int ret;

    if(frame) {
        std::string msg;
        msg.append("Send frame %3").append(PRId64).append("\n");
        printf(msg.data(), frame->pts);

    }

    ret = avcodec_send_frame(enc_ctx, frame);
    if(ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while(ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if(ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        std::string msg;
        msg.append("Write packet %3").append(PRId64).append("(size=%5d)\n");
        printf(msg.data(), pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
        std::cout << "write new frame *********************************";
    }
}

int main(int argc, char *argv[])
{
    const char *filename, *codec_name;
    const AVCodec *codec;
    AVCodecContext *c = nullptr;
    int i = 0, ret = 0, x = 0, y = 0;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

//    if(argc <= 1) {
//        fprintf(stderr, "Usage: %s <output file> <codec name>\n", argv[0]);
//        exit(0);
//    }

    filename = "/home/mranderson/gitProjects/test";
    //    codec_name = argv[2];

    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if(!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    pkt = av_packet_alloc();
    if(!pkt)
        exit(1);

    c->bit_rate = 4000000;
    c->width = 1920;
    c->height = 1080;
    c->time_base = (AVRational) {
        2, 60
    };
    c->framerate = (AVRational) {
        60, 2
    };

    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

//    if(codec->id == AV_CODEC_ID_H264)
//        av_opt_set(c->priv_data, "preset", "slow", 0);

    ret = avcodec_open2(c, codec, nullptr);
    if(ret < 0) {
        char *err = new char[AV_ERROR_MAX_STRING_SIZE];
        fprintf(stderr, "Could not open codec: %d\n", av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE));
        delete[] err;
        exit(1);
    }

    f = fopen(filename, "wb");
    if(!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if(!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    ret = av_frame_get_buffer(frame, 0);
    if(ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    for(i = 0; i < 10; ++i) {
        fflush(stdout);

        ret = av_frame_make_writable(frame);
        if(ret < 0)
            exit(1);

        for(y = 0; y < c->height; ++y) {
            for(x = 0; x < c->width; ++x)
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
        }

        for(y = 0; y < c->height / 2; ++y) {
            for(x = 0; x < c->width / 2; ++x) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        frame->pts = i;
        encode(c, frame, pkt, f);
    }

    encode(c, nullptr, pkt, f);

    if(codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode, 1, sizeof(endcode), f);

    fclose(f);

    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    //    QApplication app(argc, argv);

    //    MainWindow w;
    //    w.open();

    return 0;//app.exec();
}
#endif
