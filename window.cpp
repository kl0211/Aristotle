/*Aristotle - An educational-oriented whiteboard application
  Copyright (C) 2013 Robert Werfelmann

  Aristotle is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Aristotle is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Aristotle.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtGui>
#include "window.h"
#include "drawingarea.h"

//Private Functions
void MainWindow::createActs(){
  quitAct = new QAction(tr("&Quit"), this);
  quitAct->setShortcut(tr("CTRL+Q"));
  connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

  markerColorAct = new QAction(tr("&Marker Color"), this);
  markerColorAct->setShortcut(tr("F3"));
  connect(markerColorAct, SIGNAL(triggered()), this, SLOT(markerColor()));

  markerSizeAct = new QAction(tr("&Marker Size"), this);
  markerSizeAct->setShortcut(tr("F4"));
  connect(markerSizeAct, SIGNAL(triggered()), this, SLOT(markerSize()));

  clearScreenAct = new QAction(tr("&Clear Screen"), this);
  clearScreenAct->setShortcut(tr("CTRL+A"));
  connect(clearScreenAct, SIGNAL(triggered()), drawingArea, SLOT(clearImage()));

  aboutAct = new QAction(tr("&About"), this);
  aboutAct->setShortcut(tr("F1"));
  connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createMenus(){
  fileMenu = new QMenu(tr("&File"), this);
  fileMenu->addAction(quitAct);

  optionMenu = new QMenu(tr("&Options"), this);
  optionMenu->addAction(markerColorAct);
  optionMenu->addAction(markerSizeAct);
  optionMenu->addAction(clearScreenAct);

  helpMenu = new QMenu(tr("&Help"), this);
  helpMenu->addAction(aboutAct);

  menuBar()->addMenu(fileMenu);
  menuBar()->addMenu(optionMenu);
  menuBar()->addMenu(helpMenu);
}

//Private Slots Functions
void MainWindow::markerColor(){
  QColor newColor = QColorDialog::getColor(drawingArea->markerColor());
  if (newColor.isValid())
    drawingArea->setMarkerColor(newColor);
}

void MainWindow::markerSize(){
  bool isGood;
  int newSize = QInputDialog::getInteger(this, tr("Aristotle"),
					 tr("Set Marker Size:"),
					 drawingArea->markerSize(),
					 1, 50, 1, &isGood);
  if (isGood)
    drawingArea->setMarkerSize(newSize);
}

void MainWindow::about(){
  QMessageBox::about(this, tr("About Aristotle"),
		     tr("This area needs a good about"));
}

//Public Functions
MainWindow::MainWindow(){
  drawingArea = new DrawingBoard;
  setCentralWidget(drawingArea);

  createActs();
  createMenus();

  setWindowTitle(tr("Aristotle"));
  resize(800, 600);
}
