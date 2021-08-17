#ifndef FRAMECONTAINER_H
#define FRAMECONTAINER_H

#include <QImage>

struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVCodecParserContext;

class FrameContainer
{
public:

    FrameContainer();
    ~FrameContainer();

    static FrameContainer *fromImage(QImage img);
    static FrameContainer *fromByteArray(QByteArray ba, int imgWidth, int imgHeight);

    void codeFrame();
    void decodeFrame();

    QByteArray toByteArray();
    QImage toQImage();

private:
    int m_width;
    int m_height;

    AVCodec *m_codec;
    AVCodecContext *m_codecContext;
    AVFrame *m_frame;
    AVPacket *m_packet;
    AVCodecParserContext *m_parser;

    QByteArray m_data;

    static int timestap;

    void tempEncoder();
    void tempDecoder();
};


#endif // FRAMECONTAINER_H
