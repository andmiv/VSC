#include "framecontainer.h"

#include <QDebug>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/error.h"
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

const int FrameContainer::BLOCK_SIZE = 8;
const int FrameContainer::REF_BLOCK_LEN = 8;

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
    container->m_packet = av_packet_alloc();
    Q_ASSERT(container->m_packet);
    container->m_codecContext->bit_rate = 4000000;
    container->m_codecContext->width = container->m_width;
    container->m_codecContext->height = container->m_height;
    container->m_codecContext->time_base = (AVRational) { 2, 60 };
    container->m_codecContext->framerate = (AVRational) { 60, 2 };
    container->m_codecContext->gop_size = 10;
    container->m_codecContext->max_b_frames = 1;
    container->m_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    int ret = avcodec_open2(container->m_codecContext, container->m_codec, nullptr);
    Q_ASSERT(ret >= 0);

    container->m_frame = av_frame_alloc();
    Q_ASSERT(container->m_frame);

    container->m_frame->format = AV_PIX_FMT_RGBA;
    container->m_frame->width = container->m_width;
    container->m_frame->height = container->m_height;

    ret = av_frame_get_buffer(container->m_frame, 0);
    Q_ASSERT(ret >= 0);

    ret = av_frame_make_writable(container->m_frame);
    Q_ASSERT(ret >= 0);

    fflush(stdout);
    // Очень спорная функция
    for(int i = 0; i < container->m_height; ++i) {
            memcpy(container->m_frame->data[i], img.scanLine(i), img.width());
    }

    container->m_frame->pts = timestap;
    ++timestap;

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
    if(!container->m_codec) {
        delete container;
        return nullptr;
    }

    container->m_parser = av_parser_init(container->m_codec->id);
    if(!container->m_parser) {
        delete container;
        return nullptr;
    }

    container->m_codecContext = avcodec_alloc_context3(container->m_codec);
    if(!container->m_codecContext) {
        delete container;
        return nullptr;
    }

    if(avcodec_open2(container->m_codecContext, container->m_codec, nullptr) < 0) {
        delete container;
        return nullptr;
    }

    container->m_frame = av_frame_alloc();
    if(!container->m_frame) {
        delete container;
        return nullptr;
    }

    av_parser_parse2(container->m_parser, container->m_codecContext,
                     &container->m_packet->data, &container->m_packet->size,
                     (uint8_t *)arr.data(), arr.size(), AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

    return container;
}

void FrameContainer::codeFrame()
{
    if(m_frame)
        qDebug() << "Send frame" << m_frame->pts;

    int ret = avcodec_send_frame(m_codecContext, m_frame);
    Q_ASSERT(ret >= 0);

    while(ret >= 0) {
        ret = avcodec_receive_packet(m_codecContext, m_packet);
        Q_ASSERT(ret >= 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF);
        av_packet_unref(m_packet);
    }
}

void FrameContainer::decodeFrame()
{
    int ret = avcodec_send_packet(m_codecContext, m_packet) < 0;
    if(ret < 0)
        return;

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
    QByteArray ba;
    ba.resize(m_packet->size);
    memcpy(ba.data_ptr(), m_packet->data, m_packet->size);
    return ba;
}

QImage FrameContainer::toQImage()
{
    QImage img(m_width, m_height, QImage::Format_RGBA8888);
    memcpy(img.data_ptr(), m_frame->data, m_frame->pkt_size);
    return img;
}
