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

#ifndef DRAWINGAREA_H
#define DRAWINGAREA_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QWidget>

class DrawingBoard : public QWidget{
Q_OBJECT

   void drawLineTo(const QPoint &), erase(const QPoint &),
        resizeImage(QImage *, const QSize &);
   bool drawing;
   int myMarkerSize;
   QColor myMarkerColor;
   QImage image;
   QPoint lastPoint;

 protected:
   void mousePressEvent(QMouseEvent *), mouseMoveEvent(QMouseEvent *),
   mouseReleaseEvent(QMouseEvent *), paintEvent(QPaintEvent *),
   resizeEvent(QResizeEvent *);

 public:
   DrawingBoard(QWidget * parent = 0);
   
   void setMarkerColor(const QColor &), setMarkerSize(int);
   QColor markerColor() const {return myMarkerColor;}
   int markerSize() const {return myMarkerSize;}

 public slots:
   void clearImage();

};

#endif
