#include <QDir>
#include <QList>
#include <QTextStream>
#include <QDateTime>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#define ATLAS_WIDTH 1024
#define ATLAS_HEIGHT 1024

struct region
{
    int x;
    int y;
    int w;
    int h;
    int used;
};


MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, ui(new Ui::MainWindow)
{
    ui->setupUi(this);

//    qsrand((uint)QDateTime::currentMSecsSinceEpoch());

    QStringList args = qApp->arguments();

    if (args.size() == 1)
    {
        files.clear();
        imageList.clear();

        ProcessAssets(args[0], ""); // No language
        RenderImage("", "G_");
    }
    else
    for (int i = 1; i < args.size(); ++i)
    {
        files.clear();
        imageList.clear();

        if (args[i].compare("-") == 0)
        {
            ProcessAssets(args[0], "");
            RenderImage("", "G_");
        }
        else
        {
//            ProcessAssets(args[0], "");
            ProcessAssets(args[0], args[i]);
            RenderImage(args[i], "L_");
        }
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}


int MainWindow::ProcessAssets(QString dir, QString language)
{
    QDir currentDir = QDir();
    QStringList p = currentDir.absolutePath().split("/");
    QString asset = p.last();

//    if (language.length() == 2)
//        target = language+"_"+asset;
//    else
        target = asset;

    ui->statusList->addItem(target);

    // Read all the images to place onto atlas
    QStringList extra;

    // Add language files?
    if (language.length() == 2)
    {
        if (!currentDir.cd(language))
        {
            ui->statusList->addItem(language+" not found.");
            return 1;
        }
        currentDir.cd("images");
        extra = currentDir.entryList(QStringList("*.png"), QDir::Files | QDir::NoSymLinks);
    }
    else
    {
        currentDir.cd("images");
        extra = currentDir.entryList(QStringList("*.png"), QDir::Files | QDir::NoSymLinks);
    }

    files.append(extra);

    for (int i = 0; i < extra.size(); ++i)
    {
        QPixmap *image = new QPixmap;

        image->load(currentDir.absolutePath()+"/"+extra[i]);
        imageList.append(*image);

        QString result;
        QTextStream(&result) << extra[i].toUtf8() << " (" << image->width() << "x" << image->height() << ")";
        ui->imageList->addItem(result);
    }

    if (language.length() == 2)
    {
        currentDir.cdUp();
        currentDir.cdUp();
    }
    else
    {
        currentDir.cdUp();
    }

    return 0;
}

int MainWindow::RenderImage(QString language, QString prepend)
{
    QList<int> sortedImageList;

    int targetSize = imageList.size();
    do
    {
        int largestIndex = -1;
        int largestPixels = -1;

        for (int i = 0; i < imageList.size(); ++i)
        {
            if (!sortedImageList.contains(i))
            {
                if (largestIndex == -1 ||
                    /*imageList.at(i).width() > largestPixels ||*/
                    imageList.at(i).height() > largestPixels /*||
                    imageList.at(i).width()*imageList.at(i).height() > largestPixels*/)
                {
                    largestIndex = i;
                    /*if (imageList.at(i).width() > largestPixels)
                        largestPixels = imageList.at(i).width();
                    else
                    if (imageList.at(i).height() > largestPixels)*/
                        largestPixels = imageList.at(i).height();
                    /*else
                        largestPixels = imageList.at(i).width()*imageList.at(i).height();*/
                }
            }
        }

        sortedImageList.append(largestIndex);
    }
    while (sortedImageList.size() < targetSize);

    QList<struct region> regionList;
    struct region all = {0, 0, ATLAS_WIDTH, ATLAS_HEIGHT, -1}; // Pixel spacing
    regionList.append(all);

    int imageBottom = 0;

    for (int i = 0; i < imageList.size(); ++i)
    {
        int index = sortedImageList.at(i);

        for (QList<struct region>::iterator rl_it = regionList.begin(); rl_it != regionList.end(); ++rl_it)
        {
            struct region r = *rl_it;

            // Used?
            if (r.used == -1)
            {
                int rw = r.w;
                int rh = r.h;

                int spX = imageList.at(index).width() == ATLAS_WIDTH?0:1;
                int spY = imageList.at(index).height() == ATLAS_HEIGHT?0:1;

                int xl = rw-(imageList.at(index).width()+spX); // Pixel spacing
                int yl = rh-(imageList.at(index).height()+spY); // Pixel spacing

                if (xl < 0 || yl < 0)
                    continue;

                // Exact size found
                if (xl == 0 && yl == 0)
                {
//                    ui->statusList->insertItem(0, "Exact size found");
                    r.used = index;
                    *rl_it = r;

                    if (r.y+r.h > imageBottom)
                        imageBottom = r.y+r.h;

                    break;
                }
                else
                {
                    r.used = index;
                    r.w = imageList.at(index).width()+spX; // Pixel spacing
                    r.h = imageList.at(index).height()+spY; // Pixel spacing
                    *rl_it = r;

                    if (r.y+r.h > imageBottom)
                        imageBottom = r.y+r.h;

                    ++rl_it;

                    if (xl > 0 && yl > 0)
                    {
                        // Split the region 3 ways
//                        ui->statusList->insertItem(0, "Split the region 3 ways");
                        struct region horizontal = {r.x+rw-xl, r.y, xl, r.h, -1};
                        struct region vertical = {r.x, r.y+rh-yl, rw, yl, -1};
                        rl_it = regionList.insert(rl_it, horizontal);
                        ++rl_it;
                        rl_it = regionList.insert(rl_it, vertical);
                    }
                    else
                    if (xl > 0)
                    {
                        // Split the region 2 ways
//                        ui->statusList->insertItem(0, "Split the region 2 ways, horizontal");
                        struct region horizontal = {r.x+rw-xl, r.y, xl, r.h, -1};
                        rl_it = regionList.insert(rl_it, horizontal);
                    }
                    else
                    if (yl > 0)
                    {
                        // Split the region 2 ways
//                        ui->statusList->insertItem(0, "Split the region 2 ways, vertical");
                        struct region vertical = {r.x, r.y+rh-yl, r.w, yl, -1};
                        rl_it = regionList.insert(rl_it, vertical);
                    }

                    break;
                }
            }
        }
    }

    // Create transparent atlas image
    QPixmap atlas(ATLAS_WIDTH, imageBottom); // Could make the final image smaller
    atlas.fill(Qt::transparent);

    // Paint onto atlas
    QPainter painter(&atlas);

    QString enumStr;
    QTextStream outEnum(&enumStr);

    outEnum << "enum " << target.toUpper() << endl;
    outEnum << "{ " << endl;

    QString posStr;
    QTextStream outPos(&posStr);

    outPos << "const CIwSVec2 " << target << "TopLeft[] = {" << endl;

    QString sizeStr;
    QTextStream outSize(&sizeStr);

    outSize << "const CIwSVec2 " << target << "Size[] = {" << endl;

    int counter = 0;
    int processed = 0;
    for (QList<QPixmap>::iterator itIL = imageList.begin(); itIL != imageList.end(); ++itIL)
    {
        QPixmap image = *itIL;
        bool found = false;

        for (QList<struct region>::iterator itRL = regionList.begin(); itRL != regionList.end(); ++itRL)
        {
            struct region region = *itRL;

            if (region.used == counter)
            {
                QString name = files.at(counter).left(files.at(counter).length()-4).toUpper();

//                out << "#define " << name.toUtf8() << "_TOPLEFT CIwSVec2(" << region.x << "," << region.y << ")" << endl;
//                out << "#define " << name.toUtf8() << "_SIZE CIwSVec2(" << image.width() << "," << image.height() << ")" << endl;
                if (counter)
                    outEnum << "," << endl;
                outEnum << "\t" << prepend << name.toUtf8();

                if (counter)
                    outPos << "," << endl;
                outPos << "\t" << "CIwSVec2(" << region.x << "," << region.y << ")";

                if (counter)
                    outSize << "," << endl;
                outSize << "\t" << "CIwSVec2(" << image.width() << "," << image.height() << ")";
//                painter.drawRect(QRectF(region.x, region.y, region.w, region.h));

                painter.drawPixmap(region.x, region.y, image);
                ++processed;
                found = true;
                break;
            }
        }

        if (!found)
        {
            ui->statusList->addItem("Not processed: "+files.at(counter));
//            painter.drawRect(QRectF(r.x, r.y, r.w, r.h));
        }

        ++counter;
    }
    outEnum << endl;
    outEnum << "};" << endl;
    outEnum << endl;
    outEnum << "extern const CIwSVec2 " << target << "TopLeft[];" << endl;
    outEnum << "extern const CIwSVec2 " << target << "Size[];" << endl;

    outPos << endl << "};" << endl;
    outSize << endl << "};" << endl;

    ui->statusList->addItem(QString("Number of images: ")+QString::number(processed)+" ("+QString::number(imageList.size())+")");
    ui->propertyList->setText(enumStr);
    ui->propertyList->append(posStr);
    ui->propertyList->append(sizeStr);

    // Save the atlas image
    if (language.length() == 2)
        atlas.save(language+"_"+target+".png", "PNG", -1);
    else
        atlas.save(target+".png", "PNG", -1);

//    if (language.length() == 2)
        target = prepend+target;

    QFile h_inc;
    h_inc.setFileName(target+"_h.inc");
    if (h_inc.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&h_inc);
        out << enumStr << endl;
        h_inc.close();
    }

    QFile c_inc;
    c_inc.setFileName(target+"_c.inc");
    if (c_inc.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&c_inc);
        out << posStr << endl;
        out << sizeStr << endl;
        c_inc.close();
    }

    return 0;
}
