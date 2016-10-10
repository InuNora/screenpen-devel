#include "screenwidget.h"
#include "ui_screenwidget.h"

#include <QtGui/QPainter>
#include <QDesktopWidget>
#include <QMouseEvent>

#include <QDebug>
#include <X11/Xlib.h>

ScreenWidget::ScreenWidget(QWidget *parent) :
		QGLWidget(parent),
		ui(new Ui::screenwidget)
{
    GetTopWindow();
	ui->setupUi(this);
	setMouseTracking(true);

	_state = STATE_MOVING;

	_desktopPixmapPos = QPoint(0, 0);
	_desktopPixmapSize = QApplication::desktop()->size();
	_desktopPixmapScale = 1.0f;

	_shiftMultiplier = 2;
	_scaleSensivity = 0.1f;

  _drawMode = DRAWMODE_ARROW;

	_activePen.setColor(QColor(255, 0, 0));
	_activePen.setWidth(4);
}

ScreenWidget::~ScreenWidget()
{
	delete ui;
}

void ScreenWidget::paintEvent(QPaintEvent *event)
{
	QPainter p;

	p.begin(this);

	// Draw desktop pixmap.
	p.drawPixmap(_desktopPixmapPos.x(), _desktopPixmapPos.y(),
		     _desktopPixmapSize.width(), _desktopPixmapSize.height(),
		     _desktopPixmap);

	// Draw user objects.
	int x, y, w, h;
	for (int i = 0; i < _userRects.size(); ++i) {
		p.setPen(_userRects.at(i).pen);
		getRealUserObjectPos(_userRects.at(i), &x, &y, &w, &h);
		p.drawRect(x, y, w, h);
	}

	// Draw user lines.
	for (int i = 0; i < _userLines.size(); ++i) {
		p.setPen(_userLines.at(i).pen);
		getRealUserObjectPos(_userLines.at(i), &x, &y, &w, &h);
		p.drawLine(x, y, x+w, y+h);
	}

    // Draw user arrow.
    for (int i = 0; i < _userArrow.size(); ++i) {
        p.setPen(_userArrow.at(i).pen);
        getRealUserObjectPos(_userArrow.at(i), &x, &y, &w, &h);
        p.drawLine(x, y, x+w, y+h);
    }

	// Draw active user object.
	if (_state == STATE_DRAWING) {
		p.setPen(_activePen);

		int x = _desktopPixmapPos.x() + _startDrawPoint.x()*_desktopPixmapScale;
		int y = _desktopPixmapPos.y() + _startDrawPoint.y()*_desktopPixmapScale;
		int width = (_endDrawPoint.x() - _startDrawPoint.x())*_desktopPixmapScale;
		int height = (_endDrawPoint.y() - _startDrawPoint.y())*_desktopPixmapScale;

		if (_drawMode == DRAWMODE_RECT) {
			p.drawRect(x, y, width, height);
		} else if (_drawMode == DRAWMODE_LINE) {
			p.drawLine(x, y, width + x, height + y);
        } else if (_drawMode == DRAWMODE_ARROW) {
            p.drawPoint(x, y);
        }
	}

	p.end();
}

void ScreenWidget::mousePressEvent(QMouseEvent *event)
{
	_lastMousePos = event->pos();

	_state = STATE_DRAWING;

	_startDrawPoint = (event->pos() - _desktopPixmapPos)/_desktopPixmapScale;
	_endDrawPoint = _startDrawPoint;
}

void ScreenWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (_state == STATE_DRAWING) {
		_endDrawPoint = (event->pos() - _desktopPixmapPos)/_desktopPixmapScale;

		UserObjectData data;
		data.pen = _activePen;
		data.startPoint = _startDrawPoint;
		data.endPoint = _endDrawPoint;
		if (_drawMode == DRAWMODE_LINE) {
			_userLines.append(data);
		} else if (_drawMode == DRAWMODE_RECT) {
			_userRects.append(data);
        } else if (_drawMode == DRAWMODE_ARROW) {
            _userArrow.append(data);
        }

		_state = STATE_MOVING;
		update();
	}
}

void ScreenWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (_state == STATE_MOVING) {
		QPoint delta = event->pos() - _lastMousePos;

		shiftPixmap(delta);
		checkPixmapPos();
	} else if (_state == STATE_DRAWING) {
		_endDrawPoint = (event->pos() - _desktopPixmapPos)/_desktopPixmapScale;
        if (_drawMode == DRAWMODE_ARROW) {
            UserObjectData dataArrow;
            dataArrow.pen = _activePen;
            dataArrow.startPoint = _startDrawPoint;
            dataArrow.endPoint = _endDrawPoint;

            _userArrow.append(dataArrow);
            _startDrawPoint = _endDrawPoint;
        }
	}

	_lastMousePos = event->pos();
	update();
}

void ScreenWidget::wheelEvent(QWheelEvent *event)
{
	if (_state == STATE_MOVING) {
		int sign = event->delta() / abs(event->delta());

		_desktopPixmapScale += sign * _scaleSensivity;
		if (_desktopPixmapScale < 1.0f) {
			_desktopPixmapScale = 1.0f;
		}

		scalePixmapAt(event->pos());
		checkPixmapPos();

		update();
	}
}

void ScreenWidget::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();

	if (key == Qt::Key_Escape) {
		QApplication::quit();
        returnFocus();
	} else if ((key >= Qt::Key_1) && (key <= Qt::Key_9)) {
		_activePen.setWidth(key - Qt::Key_0);
	} else if (key == Qt::Key_R) {
		_activePen.setColor(QColor(255, 0, 0));
	} else if (key == Qt::Key_G) {
		_activePen.setColor(QColor(0, 255, 0));
	} else if (key == Qt::Key_B) {
		_activePen.setColor(QColor(0, 0, 255));
	} else if (key == Qt::Key_C) {
		_activePen.setColor(QColor(0, 255, 255));
	} else if (key == Qt::Key_M) {
		_activePen.setColor(QColor(255, 0, 255));
	} else if (key == Qt::Key_Y) {
		_activePen.setColor(QColor(255, 255, 0));
	} else if (key == Qt::Key_W) {
		_activePen.setColor(QColor(255, 255, 255));
	} else if (key == Qt::Key_Q) {
		_userRects.clear();
		_userLines.clear();
        _userArrow.clear();
		_state = STATE_MOVING;
	} else if (key == Qt::Key_Z) {
		_drawMode = DRAWMODE_LINE;
	} else if (key == Qt::Key_X) {
		_drawMode = DRAWMODE_RECT;
    } else if (key == Qt::Key_A) {
        _drawMode = DRAWMODE_ARROW;
    }

	update();
}

void ScreenWidget::keyReleaseEvent(QKeyEvent *event)
{
}

void ScreenWidget::grabDesktop()
{
    _desktopPixmap = QPixmap::grabWindow(QApplication::desktop()->winId());
}

void ScreenWidget::shiftPixmap(const QPoint delta)
{
	_desktopPixmapPos -= delta * _shiftMultiplier;
}

void ScreenWidget::scalePixmapAt(const QPoint pos)
{
	int old_w = _desktopPixmapSize.width();
	int old_h = _desktopPixmapSize.height();

	int new_w = _desktopPixmap.width() * _desktopPixmapScale;
	int new_h = _desktopPixmap.height() * _desktopPixmapScale;
	_desktopPixmapSize = QSize(new_w, new_h);

	int dw = new_w - old_w;
	int dh = new_h - old_h;

	int cur_x = pos.x() + abs(_desktopPixmapPos.x());
	int cur_y = pos.y() + abs(_desktopPixmapPos.y());

	float cur_px = -((float)cur_x / old_w);
	float cur_py = -((float)cur_y / old_h);

	_desktopPixmapPos.setX(_desktopPixmapPos.x() + dw*cur_px);
	_desktopPixmapPos.setY(_desktopPixmapPos.y() + dh*cur_py);
}

void ScreenWidget::checkPixmapPos()
{
	if (_desktopPixmapPos.x() > 0) {
		_desktopPixmapPos.setX(0);
	} else if ((_desktopPixmapSize.width() + _desktopPixmapPos.x()) < width()) {
		_desktopPixmapPos.setX(width() - _desktopPixmapSize.width());
	}

	if (_desktopPixmapPos.y() > 0) {
		_desktopPixmapPos.setY(0);
	} else if ((_desktopPixmapSize.height() + _desktopPixmapPos.y()) < height()) {
		_desktopPixmapPos.setY(height() - _desktopPixmapSize.height());
	}
}

void ScreenWidget::getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h)
{
	*x = _desktopPixmapPos.x() + userObj.startPoint.x()*_desktopPixmapScale;
	*y = _desktopPixmapPos.y() + userObj.startPoint.y()*_desktopPixmapScale;
	*w = (userObj.endPoint.x() - userObj.startPoint.x())*_desktopPixmapScale;
	*h = (userObj.endPoint.y() - userObj.startPoint.y())*_desktopPixmapScale;
}

void ScreenWidget::returnFocus()
{
    Display *dpy;
    Window _winf;
    _winf = id_win;
    dpy  = XOpenDisplay (0);
    if(dpy == NULL)
    {
        qDebug() << "Не удалось соединится с X-Server" << endl;
        return;
    }

    XSetInputFocus(dpy, _winf,  RevertToParent, CurrentTime);

    printf ("%i\n", id_win);
    XCloseDisplay(dpy);

}

void ScreenWidget::GetTopWindow()
{
    Window _winf;
    Display *dpy;
    XWindowAttributes attr;
    dpy  = XOpenDisplay (0);
    int ret;
    char *win_name;
    if(dpy == NULL)
    {
        qDebug() << "Не удалось соединится с X-Server" << endl;
        return;
    }
    XGetInputFocus(dpy, &_winf, &ret);
    printf ("0x%lx\n", _winf);
    id_win = _winf;
    printf ("%i\n", id_win);
    XFetchName(dpy, _winf, &win_name);
    printf("%s",win_name);
    printf("ok\n");
    XCloseDisplay(dpy);
}

