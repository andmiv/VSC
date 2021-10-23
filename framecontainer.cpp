#include "framecontainer.h"

#include <QDebug>
#include <QVideoFrame>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/error.h"
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

int FrameContainer::timestap = 0;

FrameContainer::FrameContainer()
    : m_width(-1)
    , m_height(-1)
    , m_codec(nullptr)
    , m_codecContext(nullptr)
    , m_frame(nullptr)
    , m_packet(nullptr)
    , m_parser(nullptr)
{

}

FrameContainer::~FrameContainer()
{
    av_parser_close(m_parser);
    avcodec_free_context(&m_codecContext);
    av_frame_free(&m_frame);
    av_packet_free(&m_packet);
}

FrameContainer *FrameContainer::fromImage(QImage img)
{
    qDebug() << "from img" << img.isNull() << img.width() << img.height();
    if(img.isNull())
        return nullptr;

    FrameContainer *container = new FrameContainer;
    container->m_height = img.height();
    if(container->m_height % 2)
        container->m_height++;
    container->m_width = img.width();
    if(container->m_width & 2)
        container->m_width++;

    container->m_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    Q_ASSERT(container->m_codec);
    container->m_codecContext = avcodec_alloc_context3(container->m_codec);
    Q_ASSERT(container->m_codecContext);
    container->m_codecContext->bit_rate = 4000000;
    container->m_codecContext->width = container->m_width;
    container->m_codecContext->height = container->m_height;
    container->m_codecContext->time_base = (AVRational) { 1, 60 };
    container->m_codecContext->framerate = (AVRational) { 60, 1 };
    container->m_codecContext->gop_size = 0;
    container->m_codecContext->max_b_frames = 1;
    container->m_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    int ret = avcodec_open2(container->m_codecContext, container->m_codec, nullptr);
    Q_ASSERT(ret >= 0);

    container->m_packet = av_packet_alloc();
    Q_ASSERT(container->m_packet);

    container->m_frame = av_frame_alloc();
    Q_ASSERT(container->m_frame);

    container->m_frame->format = container->m_codecContext->pix_fmt;
    container->m_frame->width = container->m_width;
    container->m_frame->height = container->m_height;

    ret = av_frame_get_buffer(container->m_frame, 0);
    Q_ASSERT(ret >= 0);

    fflush(stdout);
    ret = av_frame_make_writable(container->m_frame);
    Q_ASSERT(ret >= 0);

    container->m_frame->pts = timestap;
    ++timestap;

    while(container->m_packet->pts < 0) {
        for(int y = 0; y < container->m_codecContext->height; ++y) {
            for(int x = 0; x < container->m_codecContext->width; ++x)
                container->m_frame->data[0][y * container->m_frame->linesize[0] + x] = x + y + timestap * 3;
        }

        for(int y = 0; y < container->m_codecContext->height / 2; ++y) {
            for(int x = 0; x < container->m_codecContext->width / 2; ++x) {
                container->m_frame->data[1][y * container->m_frame->linesize[1] + x] = 128 + y + timestap * 2;
                container->m_frame->data[2][y * container->m_frame->linesize[2] + x] = 64 + x + timestap * 5;
            }
        }

        //    quint16 r = 0;
        //    quint16 g = 0;
        //    quint16 b = 0;

        //    quint8 Y = 0;
        //    quint8 Cb = 0;
        //    quint8 Cr = 0;

        //    for(int y = 0; container->m_height; ++y) {
        //        const QRgb *imgLine = reinterpret_cast<QRgb *>(img.scanLine(y));
        //        for(int x = 0; container->m_width; ++x) {
        //            const QRgb &imgPix = imgLine[x];

        //            r = qRed(imgPix);
        //            g = qGreen(imgPix);
        //            b = qBlue(imgPix);

        //            //Y
        //            Y = 0.299 * r + 0.587 * g + 0.114 * b;
        //            Cb = 128 - 0.168736 * r - 0.331264 * g + 0.5 * b;
        //            Cr = 128 + 0.5 * r - 0.418688 * g - 0.081312 * b;

        //            container->m_frame->data[0][y * container->m_frame->linesize[0] + x] = Y;
        //            container->m_frame->data[1][y * container->m_frame->linesize[0] + x] = Cb;
        //            container->m_frame->data[2][y * container->m_frame->linesize[2] + x] = Cr;
        //        }
        //    }

        if(container->m_frame)
            qDebug() << "Send frame" << container->m_frame->pts;


        ret = avcodec_send_frame(container->m_codecContext, container->m_frame);
        Q_ASSERT(ret >= 0);

        avcodec_receive_packet(container->m_codecContext, container->m_packet);

        std::string msg;
        msg.append("Write packet ")
                .append(QString::number(container->m_packet->dts)
                        .toStdString())
                .append("%3")
                .append(PRId64)
                .append("(size=%5d)\n");
        printf(msg.data(), container->m_packet->pts, container->m_packet->size);


        container->m_data.clear();
        container->m_data = QByteArray::fromRawData((char *)container->m_packet->data, container->m_packet->size);
    }
    return container;
}

FrameContainer *FrameContainer::fromByteArray(QByteArray arr, int imgWidth, int imgHeight)
{
    if(arr.isEmpty())
        return nullptr;

    FrameContainer *container = new FrameContainer;

    container->m_width = imgWidth;
    container->m_height = imgHeight;
    container->m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    Q_ASSERT(container->m_codec);
    if(!container->m_codec) {
        delete container;
        return nullptr;
    }

    container->m_parser = av_parser_init(container->m_codec->id);
    Q_ASSERT(container->m_parser);
    if(!container->m_parser) {
        delete container;
        return nullptr;
    }

    container->m_codecContext = avcodec_alloc_context3(container->m_codec);
    Q_ASSERT(container->m_codecContext);
    if(!container->m_codecContext) {
        delete container;
        return nullptr;
    }

    int ret = avcodec_open2(container->m_codecContext, container->m_codec, nullptr);
    Q_ASSERT(ret >= 0);
    if(ret < 0) {
        delete container;
        return nullptr;
    }

    container->m_frame = av_frame_alloc();
    Q_ASSERT(container->m_frame);
    if(!container->m_frame) {
        delete container;
        return nullptr;
    }

    int arrSize = arr.size();
    uint8_t *ptr = (uint8_t *)(arr.data());
    while(arrSize > 0) {
        int ret = av_parser_parse2(container->m_parser, container->m_codecContext,
                                   &container->m_packet->data, &container->m_packet->size,
                                   ptr, arrSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

        arrSize -= ret;
        ptr += ret;
    }

    qDebug() << container->m_packet->data << container->m_packet->size;

    return container;
}

void FrameContainer::codeFrame()
{
    //    int ret =
    //    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
    //        return;
    //    }
    //    else if(ret < 0) {
    //        fprintf(stderr, "Error during encoding\n");
    //        exit(1);
    //    }
}

void FrameContainer::decodeFrame()
{
    qDebug() << "receive frame";
    int ret = avcodec_send_packet(m_codecContext, m_packet);
    if(ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }


    while(ret >= 0) {
        ret = avcodec_receive_frame(m_codecContext, m_frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if(ret < 0)
            return;
    }
}

QByteArray FrameContainer::toByteArray()
{
    return m_data;
}

QImage FrameContainer::toQImage()
{
    QImage img(m_width, m_height, QImage::Format_RGBA8888);
    memcpy(img.data_ptr(), m_frame->data, m_frame->pkt_size);
    return img;
}

void FrameContainer::tempEncoder()
{
    const AVCodec *codec;
    AVCodecContext *c = nullptr;
    int i = 0, ret = 0, x = 0, y = 0;
    AVFrame *frame;
    AVPacket *pkt;

    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!codec) {
        fprintf(stderr, "Codec not found\n");
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
    c->time_base = (AVRational) { 2, 60 };
    c->framerate = (AVRational) { 60, 2 };

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

        if(frame) {
            std::string msg;
            msg.append("Send frame %3").append(PRId64).append("\n");
            printf(msg.data(), frame->pts);

        }

        ret = avcodec_send_frame(c, frame);
        if(ret < 0) {
            fprintf(stderr, "Error sending a frame for encoding\n");
            exit(1);
        }

        while(ret >= 0) {
            ret = avcodec_receive_packet(c, pkt);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if(ret < 0) {
                fprintf(stderr, "Error during encoding\n");
                exit(1);
            }

            std::string msg;
            msg.append("Write packet %3").append(PRId64).append("(size=%5d)\n");
            printf(msg.data(), pkt->pts, pkt->size);
            av_packet_unref(pkt);
            qDebug() << "write new frame *********************************";
        }

        frame->pts = i;
    }

    ret = avcodec_send_frame(c, nullptr);
    if(ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while(ret >= 0) {
        ret = avcodec_receive_packet(c, pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if(ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        std::string msg;
        msg.append("Write packet %3").append(PRId64).append("(size=%5d)\n");
        printf(msg.data(), pkt->pts, pkt->size);
        av_packet_unref(pkt);
        qDebug() << "write new frame *********************************";
    }


    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

}

void FrameContainer::tempDecoder()
{
    //#define INBUF_SIZE 4096

    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c = nullptr;
    AVFrame *frame;
    uint8_t *data;
    size_t data_size;
    int ret;
    AVPacket *pkt;

    pkt = av_packet_alloc();
    if(!pkt)
        exit(1);

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

    if(avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    frame = av_frame_alloc();
    if(!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    //            data_size = fread(inbuf, 1, INBUF_SIZE, f);
    //            if(!data_size)
    //                break;

    //            /* use the parser to split the data into frames */
    //            data = inbuf;
    //            while(data_size > 0) {
    //                ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
    //                                       data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    //                if(ret < 0) {
    //                    fprintf(stderr, "Error while parsing\n");
    //                    exit(1);
    //                }
    //                data      += ret;
    //                data_size -= ret;

    //                if(pkt->size)
    //                    decode(c, frame, pkt, outfilename);
    //            }

    //        decode(c, frame, NULL, outfilename);

    //        static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt, const char *filename)
    //        {
    //            char buf[1024];
    //            int ret;

    //            ret = avcodec_send_packet(dec_ctx, pkt);
    //            if(ret < 0) {
    //                fprintf(stderr, "Error sending a packet for decoding\n");
    //                exit(1);
    //            }

    //            while(ret >= 0) {
    //                ret = avcodec_receive_frame(dec_ctx, frame);
    //                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    //                    return;
    //                else if(ret < 0) {
    //                    fprintf(stderr, "Error during decoding\n");
    //                    exit(1);
    //                }

    //                printf("saving frame %3d\n", dec_ctx->frame_number);
    //                fflush(stdout);

    //                snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
    //                pgm_save(frame->data[0], frame->linesize[0],
    //                         frame->width, frame->height, buf);
    //            }
    //        }

}
