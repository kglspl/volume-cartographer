//----------------------------------------------------------------------------------------------------------------------------------------
// TextureViewer.h file for TextureViewer Class
// Purpose: Create header file for TextureViewer Class
// Developer: Michael Royal - mgro224@g.uky.edu
// October 12, 2015 - Spring Semester 2016
// Last Updated 10/16/2015 by: Michael Royal
// Code Reference:
// http://doc.qt.io/qt-5/qtwidgets-widgets-imageviewer-example.html ---(I
// edited/formatted the code to suit our purposes)

// Copyright 2015 (Brent Seales: Volume Cartography Research)
// University of Kentucky VisCenter
//----------------------------------------------------------------------------------------------------------------------------------------

#pragma once

#include <QAction>
#include <QAction>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <qmainwindow.h>
#include <qnumeric.h>
#include "GlobalValues.h"

class TextureViewer : QObject
{
    Q_OBJECT

public:
    TextureViewer(GlobalValues* globals);
    QVBoxLayout* getLayout();
    void clearGUI();
    void setImage();
    void clearImageLabel();
    void progressActive(bool value);
    void setEnabled(bool value);
    void clearLabel();

private slots:
    void open();
    void zoom_In();
    void zoom_Out();
    void reset_Size();
    void quitThread();

private:
    void create_Actions();
    void scale_Texture(double factor);
    void adjustScrollBar(QScrollBar* scrollBar, double factor);

    GlobalValues* _globals;

    QPushButton* zoomIn;
    QPushButton* zoomOut;
    QPushButton* refresh;
    QPushButton* cancel;
    QLabel* spacer;
    QLabel* viewer;
    QLabel* imageLabel;
    QScrollArea* scrollArea;
    QHBoxLayout* zoom;
    QVBoxLayout* image_Management;
    QProgressBar* progressBar;
    QPixmap pix;
    QAction* zoomInAction;
    QAction* zoomOutAction;
    QAction* resetSizeAction;
    QAction* _cancel;

    double scaleFactor;
};