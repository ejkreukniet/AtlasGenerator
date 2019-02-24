#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
    int ProcessAssets(QString dir, QString language);
    int RenderImage(QString language, QString prepend);

private:
    Ui::MainWindow *ui;
    QString target;
    QStringList files;
    QList<QPixmap> imageList;
};

#endif // MAINWINDOW_H
