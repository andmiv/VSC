extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
}

#include <string>
#include <iostream>

#define GRAB

#include <QApplication>

#ifdef NON

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.open();

    return app.exec();
}

#endif

#ifdef GRAB

#include <QDebug>
#include <QTime>

//static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, const char *filename)
//{
//    FILE *f;
//    int i;
//    f = fopen(filename, "w");
//    // writing the minimal required header for a pgm file x11_format
//    // portable graymap x11_format -> https://en.wikipedia.org/wiki/Netpbm_x11_format#PGM_example
//    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

//    // writing line by line
//    for(i = 0; i < ysize; i++)
//        fwrite(buf + i * wrap, 1, xsize, f);

//    qDebug() << "Frame written:" << xsize << "x" << ysize;
//    fclose(f);
//}

#define ofile_name out_file_name.toStdString().data()

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QString out_file_name = "/home/adi/temp/rec_test.h264";

    avdevice_register_all();
    avformat_network_init();

    // Входной контекст от иксов {

    AVFormatContext *x11_formatContext = avformat_alloc_context();
    AVInputFormat *x11_format = av_find_input_format("x11grab");

    AVStream *x11_stream = nullptr;
    AVCodecParameters *x11_codecpar = nullptr;
    AVDictionary *dict = nullptr;
    AVFrame *x11_frame = nullptr;
    AVCodecContext *x11_codecContext = nullptr;
    AVCodec *x11_codec = nullptr;
    AVPacket *x11_packet = nullptr;

    av_dict_set(&dict, "framerate", "60", 0);
    av_dict_set(&dict, "video_size", "3840x1080", 0);
    av_dict_set(&dict, "probesize", "100M", 0);

    if(avformat_open_input(&x11_formatContext, ":0.0", x11_format, &dict) != 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }

    if(avformat_find_stream_info(x11_formatContext, nullptr) < 0)
    {
        printf("Couldn't find stream information.\n");
        return -1;
    }

    // }

    // Выходной контекст в формате yuv/h264 {

//    AVFormatContext *yuv_formatContext = nullptr;
//    AVStream *yuv_stream = nullptr;
    AVCodecContext *yuv_codecContext = nullptr;
    AVCodec *yuv_codec = nullptr;
    AVPacket *yuv_packet = nullptr;
    AVFrame *yuv_frame = nullptr;

//    int ret = avformat_alloc_output_context2(&yuv_formatContext, nullptr, nullptr, ofile_name);
//    Q_ASSERT(ret >= 0);

    int videoindex = -1;
    for(unsigned int i = 0; i < x11_formatContext->nb_streams; i++) {
        if(x11_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            x11_stream = x11_formatContext->streams[i];
            x11_codecpar = x11_stream->codecpar;

//            yuv_stream = avformat_new_stream(yuv_formatContext, nullptr);
//            Q_ASSERT(yuv_stream != nullptr);

//            int ret = avcodec_parameters_copy(yuv_stream->codecpar, x11_codecpar);
//            Q_ASSERT(ret >= 0);

            videoindex = i;
            break;
        }
    }

    if(videoindex == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    // Инициализация форматирования {

    x11_codec = avcodec_find_decoder(x11_codecpar->codec_id);
    Q_ASSERT(x11_codec != nullptr);

    x11_codecContext = avcodec_alloc_context3(x11_codec);
    Q_ASSERT(x11_codecContext != nullptr);

    avcodec_parameters_to_context(x11_codecContext, x11_codecpar);
    avcodec_open2(x11_codecContext, x11_codec, nullptr);

//    if(yuv_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
//        yuv_formatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    x11_frame = av_frame_alloc();
    Q_ASSERT(x11_frame != nullptr);

    x11_packet = av_packet_alloc();
    Q_ASSERT(x11_packet != nullptr);

    yuv_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    Q_ASSERT(yuv_codec != nullptr);

    yuv_codecContext = avcodec_alloc_context3(yuv_codec);
    Q_ASSERT(yuv_codecContext != nullptr);

    yuv_codecContext->height = x11_codecContext->height;
    yuv_codecContext->width = x11_codecContext->width;

    yuv_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    yuv_codecContext->bit_rate = 2 * 1000 * 1000;
    yuv_codecContext->rc_buffer_size = 4 * 1000 * 1000;
    yuv_codecContext->rc_max_rate = 2 * 1000 * 1000;
    yuv_codecContext->rc_min_rate = 2.5 * 1000 * 1000;

    yuv_codecContext->time_base = av_inv_q(av_guess_frame_rate(x11_formatContext, x11_stream, nullptr));
//    yuv_stream->time_base = yuv_codecContext->time_base;

    int ret = avcodec_open2(yuv_codecContext, yuv_codec, nullptr);
    Q_ASSERT(ret == 0);
//    ret = avcodec_parameters_from_context(yuv_stream->codecpar, yuv_codecContext);
//    Q_ASSERT(ret == 0);

    yuv_packet = av_packet_alloc();
    Q_ASSERT(yuv_packet != nullptr);

    yuv_frame = av_frame_alloc();
    Q_ASSERT(yuv_frame != nullptr);

    yuv_frame->format = yuv_codecContext->pix_fmt;
    yuv_frame->width = yuv_codecContext->width;
    yuv_frame->height = yuv_codecContext->height;
    ret = av_frame_get_buffer(yuv_frame, 0);
    Q_ASSERT(ret == 0);


    // }

    // Открываем выход {

//    if(!(yuv_formatContext->oformat->flags & AVFMT_NOFILE)) {
//        int ret = avio_open(&yuv_formatContext->pb, ofile_name, AVIO_FLAG_WRITE);
//        Q_ASSERT(ret >= 0);
//    }

//    ret = avformat_write_header(yuv_formatContext, nullptr);
//    Q_ASSERT(ret >= 0);

    // }

    // Инициализация конвертера {

    SwsContext *convertContext = sws_getContext(x11_codecContext->width,
                                                x11_codecContext->height,
                                                x11_codecContext->pix_fmt,
                                                x11_codecContext->width,
                                                x11_codecContext->height,
                                                AV_PIX_FMT_YUV420P,
                                                SWS_BICUBIC,
                                                nullptr,
                                                nullptr,
                                                nullptr);

    // }


//    yuv_stream->avg_frame_rate.den = x11_stream->avg_frame_rate.den;
//    yuv_stream->avg_frame_rate.num = x11_stream->avg_frame_rate.num;


    int index = 0;
    bool exit = false;

    while(!exit && index < 6000) {
        exit = av_read_frame(x11_formatContext, x11_packet);
        if(exit)
            continue;

        ret = avcodec_send_packet(x11_codecContext, x11_packet);
        while(ret >= 0) {
            ret = avcodec_receive_frame(x11_codecContext, x11_frame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            Q_ASSERT(ret >= 0);

            sws_scale(convertContext,
                      (const unsigned char *const *)x11_frame->data,
                      x11_frame->linesize,
                      0,
                      x11_codecContext->height,
                      yuv_frame->data,
                      yuv_frame->linesize);

            yuv_frame->pts = index;
//            yuv_frame->pict_type = x11_frame->pict_type;

//            qDebug() << "Frame:" << av_get_picture_type_char(x11_frame->pict_type)
//                     << "\nFrame_num:" << x11_codecContext->frame_number
//                     << "\n[ Frame_width:" << x11_frame->width << "; Frame_height:" << x11_frame->height << " ]"
//                     << ("\nPts: ") << (x11_frame->pts)
//                     << ("\nPkt_sts: ") << (x11_frame->pkt_dts)
//                     << ("\nKey_frame:") << (x11_frame->key_frame)
//                     << ("\n[ Coded_pcts_num:") << (x11_frame->coded_picture_number)
//                     << ("Display_pcts_num:") << (x11_frame->display_picture_number) << " ]\n";


            ret = avcodec_send_frame(yuv_codecContext, yuv_frame);
            while(ret >= 0) {
                ret = avcodec_receive_packet(yuv_codecContext, yuv_packet);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                Q_ASSERT(ret >= 0);

                yuv_packet->stream_index = x11_packet->stream_index;
                //                yuv_packet->duration = x11_stream->time_base.den
                //                                       / x11_stream->time_base.num
                //                                       / x11_stream->avg_frame_rate.num
                //                                       * yuv_stream->avg_frame_rate.den;

                qDebug() << "out pts:" << yuv_frame->pts
                         << "\nout time_base:" << yuv_codecContext->time_base.num << "/" << yuv_codecContext->time_base.den
                         << "\nout framerate:" << yuv_codecContext->framerate.num << "/" << yuv_codecContext->framerate.den
                         << "\nout dts:" << yuv_frame->pkt_dts << "\npkt dts:" << x11_packet->dts
                         << "\nout pos in stream:" << yuv_packet->pos
                         << "\nin avg_frame_rate:" << x11_stream->avg_frame_rate.num << "/" << x11_stream->avg_frame_rate.den
//                         << "\nout avg_frame_rate:" << yuv_stream->avg_frame_rate.num << "/" << yuv_stream->avg_frame_rate.den
                         << "\nout duration:" << yuv_packet->duration
                         << "\nin pict type:" << av_get_picture_type_char(x11_frame->pict_type)
                         << "\nout pict type:" << av_get_picture_type_char(yuv_frame->pict_type)
                         << "\nOUT_PACK_SIZE:" << yuv_packet->size << '\n';

//                av_packet_rescale_ts(yuv_packet, yuv_stream->time_base, x11_stream->time_base);
//                ret = av_interleaved_write_frame(yuv_formatContext, yuv_packet);
            }

            av_packet_unref(yuv_packet);
            av_frame_unref(x11_frame);
        }
        av_packet_unref(x11_packet);

        ++index;
    }

//    av_write_trailer(yuv_formatContext);
    return 0;
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
        fprintf(stderr, "Could not allocate video codec x11_formatContext\n");
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
    avcodec_free_x11_formatContext(&c);
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
        fprintf(stderr, "Could not allocate video codec x11_formatContext\n");
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

    frame->x11_format = c->pix_fmt;
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

    avcodec_free_x11_formatContext(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    //    QApplication app(argc, argv);

    //    MainWindow w;
    //    w.open();

    return 0;//app.exec();
}
#endif
