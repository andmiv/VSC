#ifndef FRAMECONTAINER_H
#define FRAMECONTAINER_H

#include <QImage>

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

    static FrameContainer fromImage(QImage img);
    static FrameContainer fromByteArray(QByteArray ba, int imgWidth, int imgHeight);

    void codeFrame();
    void decodeFrame();

    QByteArray toByteArray();
    QImage toQImage();

private:
    QVector<QVector<Block>> m_blocks;
    int m_width;
    int m_height;
};


#endif // FRAMECONTAINER_H
