#include "framecontainer.h"

const int FrameContainer::BLOCK_SIZE = 8;
const int FrameContainer::REF_BLOCK_LEN = 8;

FrameContainer::FrameContainer()
    : m_blocks()
    , m_width(-1)
    , m_height(-1)
{

}

FrameContainer FrameContainer::fromImage(QImage img)
{
    if(img.isNull())
        return FrameContainer();

    FrameContainer container;
    container.m_height = img.height();
    container.m_width = img.width();


    quint16 r = 0;
    quint16 g = 0;
    quint16 b = 0;

    quint8 Cb = 0;
    quint8 Cr = 0;

    for(int y = 0; y * BLOCK_SIZE < container.m_height; ++y) {
        container.m_blocks.append(QVector<Block>());
        for(int x = 0; x * BLOCK_SIZE < container.m_width; ++x) {
            container.m_blocks[y].append(Block());
            Block &block = container.m_blocks[y][x];
            for(int by = 0; by < BLOCK_SIZE; ++by) {
                const QRgb *imgLine = reinterpret_cast<QRgb *>(img.scanLine(y * BLOCK_SIZE + by));
                for(int bx = 0; bx < BLOCK_SIZE; ++bx) {
                    const QRgb &imgPix = imgLine[x * BLOCK_SIZE + bx];
                    Pixel &pix = block.pixels[by][bx];

                    r = qRed(imgPix);
                    g = qGreen(imgPix);
                    b = qBlue(imgPix);

                    //Y
                    pix.unsignedPix[0] = 0.299 * r + 0.587 * g + 0.114 * b;
                    Cb = 128 - 0.168736 * r - 0.331264 * g + 0.5 * b;
                    Cr = 128 + 0.5 * r - 0.418688 * g - 0.081312 * b;

                    //Cb
                    pix.unsignedPix[1] = Cb;

                    //Cr
                    pix.unsignedPix[2] = Cr;
                }
            }

        }
    }

    return container;
}

FrameContainer FrameContainer::fromByteArray(QByteArray arr, int imgWidth, int imgHeight)
{
    QDataStream stream(&arr, QIODevice::ReadOnly);

    FrameContainer container;
    container.m_width = imgWidth;
    container.m_height = imgHeight;

    quint8 Cb = 0;
    quint8 Cr = 0;

    for(int y = 0; y * 8 < container.m_height; ++y) {
        container.m_blocks.append(QVector<Block>());
        for(int x = 0; x * 8 < container.m_width; ++x) {
            container.m_blocks[y].append(Block());
            Block &block = container.m_blocks[y][x];
            for(int by = 0; by < BLOCK_SIZE; ++by) {
                for(int bx = 0; bx < BLOCK_SIZE; ++bx) {
                    Pixel &pix = block.pixels[by][bx];

                    stream >> pix.unsignedPix[0];
                    stream >> Cb >> Cr;

                    pix.unsignedPix[1] = Cb;
                    pix.unsignedPix[2] = Cr;
                }
            }
        }
    }
    return container;
}

void FrameContainer::codeFrame()
{
    Pixel avaragePix;
    avaragePix.unsignedPix[0] = 127;
    avaragePix.unsignedPix[1] = 127;
    avaragePix.unsignedPix[2] = 127;

    FrameContainer uncodedFrame = *this;


    for(int y = 0; y * BLOCK_SIZE < m_height; ++y) {
        for(int x = 0; x * BLOCK_SIZE < m_width; ++x) {

            Pixel xPixels[8];
            Pixel yPixels[9];

            bool canCodeYBlock = false;
            bool canConeXBlock = false;

            // Получить верхний левый референсный пиксел
            Pixel &yPix = yPixels[0];
            if(x == 0 || y == 0)
                yPix = avaragePix;
            else
                yPix = uncodedFrame.m_blocks[y - 1][x - 1].pixels[BLOCK_SIZE - 1][BLOCK_SIZE - 1];

            // Получаем референсные пикселы слева и сверху от текущего блока
            for(int r_1 = 0; r_1 < BLOCK_SIZE; ++r_1) {
                canCodeYBlock = x != 0;
                canConeXBlock = y != 0;

                Pixel &yPix = yPixels[r_1 + 1];
                Pixel &xPix = xPixels[r_1];

                if(canCodeYBlock)
                    yPix = uncodedFrame.m_blocks[y][x - 1].pixels[r_1][BLOCK_SIZE - 1];
                else {
                    yPix = xPixels[0];
                }

                if(canConeXBlock)
                    xPix = uncodedFrame.m_blocks[y - 1][x].pixels[BLOCK_SIZE - 1][r_1];
                else {
                    xPix = yPixels[0];
                }
            }

            // Получаем референсные пикселы ниже и левее текущего блока
            //            for(int r_2 = BLOCK_SIZE; r_2 < REF_BLOCK_LEN; ++r_2) {
            //                canCodeYBlock = x != 0 && y + 1 != m_blocks.size() && m_blocks[y + 1][x - 1].coded;
            //                canConeXBlock = y != 0 && x + 1 != m_blocks[y].size() && m_blocks[y - 1][x + 1].coded;

            //                yPix = refBlock.yPixels[r_2 + 1];
            //                Pixel &xPix = refBlock.xPixels[r_2];

            //                if(canCodeYBlock)
            //                    yPix = m_blocks[y][x - 1].pixels[r_2][BLOCK_SIZE - 1];
            //                else
            //                    yPix = avaragePix;

            //                if(canConeXBlock)
            //                    xPix = m_blocks[y - 1][x].pixels[BLOCK_SIZE - 1][r_2];
            //                else
            //                    xPix = avaragePix;
            //            }
            if(!(m_blocks.size() > y) || !(m_blocks[y].size() > x))
                continue;

            Block &block = m_blocks[y][x];

            for(int by = 0; by < BLOCK_SIZE; ++by) {
                for(int bx = 0; bx < BLOCK_SIZE; ++bx) {
                    Pixel delta;
                    Pixel &pix = block.pixels[by][bx];

                    delta.unsignedPix[0] = ((BLOCK_SIZE - 1 - bx) * (yPixels[by + 1].unsignedPix[0])
                            + (bx + 1) * (xPixels[BLOCK_SIZE - 1].unsignedPix[0])
                            + (BLOCK_SIZE - 1 - by) * (xPixels[bx].unsignedPix[0])
                            + (by + 1) * (yPixels[BLOCK_SIZE].unsignedPix[0]) + (BLOCK_SIZE)) / (2 * BLOCK_SIZE);

                    delta.unsignedPix[1] = ((BLOCK_SIZE - 1 - bx) * (yPixels[by + 1].unsignedPix[1])
                            + (bx + 1) * (xPixels[BLOCK_SIZE - 1].unsignedPix[1])
                            + (BLOCK_SIZE - 1 - by) * (xPixels[bx].unsignedPix[1])
                            + (by + 1) * (yPixels[BLOCK_SIZE].unsignedPix[1]) + (BLOCK_SIZE)) / (2 * BLOCK_SIZE);

                    delta.unsignedPix[2] = ((BLOCK_SIZE - 1 - bx) * (yPixels[by + 1].unsignedPix[2])
                            + (bx + 1) * (xPixels[BLOCK_SIZE - 1].unsignedPix[2])
                            + (BLOCK_SIZE - 1 - by) * (xPixels[bx].unsignedPix[2])
                            + (by + 1) * (yPixels[BLOCK_SIZE].unsignedPix[2]) + (BLOCK_SIZE)) / (2 * BLOCK_SIZE);

                    pix.unsignedPix[0] -= delta.unsignedPix[0];
                    pix.unsignedPix[1] -= delta.unsignedPix[1];
                    pix.unsignedPix[2] -= delta.unsignedPix[2];
                }
            }
            block.coded = true;
        }
    }
}

void FrameContainer::decodeFrame()
{
    Pixel avaragePix;
    avaragePix.unsignedPix[0] = 127;
    avaragePix.unsignedPix[1] = 127;
    avaragePix.unsignedPix[2] = 127;

    for(int y = 0; y * BLOCK_SIZE < m_height; ++y) {
        for(int x = 0; x * BLOCK_SIZE < m_width; ++x) {

            Pixel xPixels[8];
            Pixel yPixels[9];

            bool canCodeYBlock = false;
            bool canConeXBlock = false;

            // Получить верхний левый референсный пиксел
            Pixel &yPix = yPixels[0];
            if(x == 0 || y == 0)
                yPix = avaragePix;
            else
                yPix = m_blocks[y - 1][x - 1].pixels[BLOCK_SIZE - 1][BLOCK_SIZE - 1];

            // Получаем референсные пикселы слева и сверху от текущего блока
            for(int r_1 = 0; r_1 < BLOCK_SIZE; ++r_1) {
                canCodeYBlock = x != 0;
                canConeXBlock = y != 0;

                Pixel &yPix = yPixels[r_1 + 1];
                Pixel &xPix = xPixels[r_1];

                if(canCodeYBlock)
                    yPix = m_blocks[y][x - 1].pixels[r_1][BLOCK_SIZE - 1];
                else {
                    yPix = xPixels[0];
                }

                if(canConeXBlock)
                    xPix = m_blocks[y - 1][x].pixels[BLOCK_SIZE - 1][r_1];
                else {
                    xPix = yPixels[0];
                }
            }

            // Получаем референсные пикселы ниже и левее текущего блока
            //            for(int r_2 = BLOCK_SIZE; r_2 < REF_BLOCK_LEN; ++r_2) {
            //                canCodeYBlock = x != 0 && y + 1 != m_blocks.size() && m_blocks[y + 1][x - 1].coded;
            //                canConeXBlock = y != 0 && x + 1 != m_blocks[y].size() && m_blocks[y - 1][x + 1].coded;

            //                yPix = refBlock.yPixels[r_2 + 1];
            //                Pixel &xPix = refBlock.xPixels[r_2];

            //                if(canCodeYBlock)
            //                    yPix = m_blocks[y][x - 1].pixels[r_2][BLOCK_SIZE - 1];
            //                else
            //                    yPix = avaragePix;

            //                if(canConeXBlock)
            //                    xPix = m_blocks[y - 1][x].pixels[BLOCK_SIZE - 1][r_2];
            //                else
            //                    xPix = avaragePix;
            //            }
            Block &block = m_blocks[y][x];

            for(int by = 0; by < BLOCK_SIZE; ++by) {
                for(int bx = 0; bx < BLOCK_SIZE; ++bx) {
                    Pixel delta;
                    Pixel &pix = block.pixels[by][bx];

                    delta.unsignedPix[0] = ((BLOCK_SIZE - 1 - bx) * (yPixels[by + 1].unsignedPix[0])
                            + (bx + 1) * (xPixels[BLOCK_SIZE - 1].unsignedPix[0])
                            + (BLOCK_SIZE - 1 - by) * (xPixels[bx].unsignedPix[0])
                            + (by + 1) * (yPixels[BLOCK_SIZE].unsignedPix[0]) + (BLOCK_SIZE)) / (2 * BLOCK_SIZE);

                    delta.unsignedPix[1] = ((BLOCK_SIZE - 1 - bx) * (yPixels[by + 1].unsignedPix[1])
                            + (bx + 1) * (xPixels[BLOCK_SIZE - 1].unsignedPix[1])
                            + (BLOCK_SIZE - 1 - by) * (xPixels[bx].unsignedPix[1])
                            + (by + 1) * (yPixels[BLOCK_SIZE].unsignedPix[1]) + (BLOCK_SIZE)) / (2 * BLOCK_SIZE);

                    delta.unsignedPix[2] = ((BLOCK_SIZE - 1 - bx) * (yPixels[by + 1].unsignedPix[2])
                            + (bx + 1) * (xPixels[BLOCK_SIZE - 1].unsignedPix[2])
                            + (BLOCK_SIZE - 1 - by) * (xPixels[bx].unsignedPix[2])
                            + (by + 1) * (yPixels[BLOCK_SIZE].unsignedPix[2]) + (BLOCK_SIZE)) / (2 * BLOCK_SIZE);

                    pix.unsignedPix[0] += delta.unsignedPix[0];
                    pix.unsignedPix[1] += delta.unsignedPix[1];
                    pix.unsignedPix[2] += delta.unsignedPix[2];
                }
            }
            block.coded = true;
        }
    }
}

QByteArray FrameContainer::toByteArray()
{
    if(m_blocks.isEmpty())
        return QByteArray();

    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);

    for(int y = 0; y < m_blocks.size(); ++y) {
        for(int x = 0; x < m_blocks[y].size(); ++x) {
            const Block &block = m_blocks[y][x];
            for(int by = 0; by < BLOCK_SIZE; ++by) {
                for(int bx = 0; bx < BLOCK_SIZE; ++bx) {
                    const Pixel &pix = block.pixels[by][bx];
                    stream << pix.unsignedPix[0];
                    stream << pix.unsignedPix[1]
                            << pix.unsignedPix[2];
                }
            }
        }
    }
    return ba;
}

QImage FrameContainer::toQImage()
{
    if(!(m_width > 0) || !(m_height > 0))
        return QImage();

    QImage img(m_width, m_height, QImage::Format_RGB32);

    int r = 0;
    int g = 0;
    int b = 0;

    for(int y = 0; y < m_blocks.size(); ++y) {
        for(int x = 0; x < m_blocks[y].size(); ++x) {
            const Block &block = m_blocks[y][x];
            for(int by = 0; by < BLOCK_SIZE; ++by) {
                QRgb *imgLine = reinterpret_cast<QRgb *>(img.scanLine(y * BLOCK_SIZE + by));
                for(int bx = 0; bx < BLOCK_SIZE; ++bx) {
                    QRgb &imgPix = imgLine[x * BLOCK_SIZE + bx];
                    const Pixel &pix = block.pixels[by][bx];

                    r = pix.unsignedPix[0] + 1.402 * (pix.unsignedPix[2] - 128);
                    g = pix.unsignedPix[0] - 0.34414 * (pix.unsignedPix[1] - 128) - 0.71414 * (pix.unsignedPix[2] - 128);
                    b = pix.unsignedPix[0] + 1.772 * (pix.unsignedPix[1] - 128);

                    if(r > 255) r = 255;
                    else if(r < 0) r = 0;
                    if(g > 255) g = 255;
                    else if(g < 0) g = 0;
                    if(b > 255) b = 255;
                    else if(b < 0) b = 0;

                    imgPix = qRgb(r, g, b);
                }
            }
        }
    }

    return img;
}
