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
#include "drawingarea.h"

void DrawingBoard::drawLineTo(const QPoint & endPoint){
  QPainter painter(&image);
  painter.setPen(QPen(myMarkerColor, myMarkerSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.drawLine(lastPoint, endPoint);
  modified = true;

  int rad = (myMarkerSize / 2) + 2;
  update(QRect(lastPoint, endPoint).normalized().adjusted(-rad, -rad, +rad, +rad));
  lastPoint = endPoint;
}

void DrawingBoard::resizeImage(QImage * image, const QSize & newSize){
  if (image->size() == newSize) return;

  QImage newImage(newSize, QImage::Format_RGB32);
  newImage.fill(qRgb(0,0,0));
  QPainter painter(&newImage);
  painter.drawImage(QPoint(0,0), *image);
  *image = newImage;
}

void DrawingBoard::mousePressEvent(QMouseEvent * event){
  if (event->button() == Qt::LeftButton){
    lastPoint = event->pos();
    drawing = true;
  }
}

void DrawingBoard::mouseMoveEvent(QMouseEvent * event){
  if ((event->buttons() & Qt::LeftButton) && drawing)
    drawLineTo(event->pos());
}

void DrawingBoard::mouseReleaseEvent(QMouseEvent * event){
  if (event->button() == Qt::LeftButton && drawing){
    drawLineTo(event->pos());
    drawing = true;
  }
}

void DrawingBoard::paintEvent(QPaintEvent * event){
  QPainter painter(this);
  QRect dirtyRect = event->rect();
  painter.drawImage(dirtyRect, image, dirtyRect);
}

void DrawingBoard::resizeEvent(QResizeEvent * event){
  if (width() > image.width() || height() > image.height()){
    int newWidth = qMax(width() + 128, image.width());
    int newHeight = qMax(height() + 128, image.height());
    resizeImage(&image, QSize(newWidth, newHeight));
    update();
  }
  QWidget::resizeEvent(event);
}

DrawingBoard::DrawingBoard(QWidget * parent)
  : QWidget(parent){
  setAttribute(Qt::WA_StaticContents);
  modified = false;
  drawing = false;
  myMarkerSize = 4;
  myMarkerColor = Qt::white;
}

void DrawingBoard::setMarkerColor(const QColor & newColor){
  myMarkerColor = newColor;
}

void DrawingBoard::setMarkerSize(int newSize){
  myMarkerSize = newSize;
}

void DrawingBoard::clearImage(){
  image.fill(qRgb(0,0,0));
  modified = true;
  update();
}
