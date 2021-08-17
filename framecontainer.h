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
    static const int BLOCK_SIZE;
    static const int REF_BLOCK_LEN;

    //! Y - unsingedPix[0]
    //! Cb - unsingedPix[1]
    //! Cr - pix.unsingedPix[2]
    union Pixel {
        quint8 unsignedPix[3];
    };
    struct Block {
        Pixel pixels[8][8];
        bool coded = false;
    };

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

    static int timestap;
};


#endif // FRAMECONTAINER_H
