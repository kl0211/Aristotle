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

#ifndef WINDOW_H
#define WINDOW_H

#include <QList>
#include <QMainWindow>

class DrawingBoard;

class MainWindow : public QMainWindow{
  Q_OBJECT

  void createActs(), createMenus();
  DrawingBoard * drawingArea;
  QMenu * fileMenu, * optionMenu, * helpMenu;
  QAction * quitAct, * markerColorAct, * markerSizeAct,
          * clearScreenAct, * aboutAct;

 private slots:
  void markerColor();
  void markerSize();
  void about();

 public:
  MainWindow();
};

#endif
