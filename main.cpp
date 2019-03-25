#include <iostream>
#include <functional>
#include <iomanip>
#include <set>
#include <utility>
#include <sstream>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSettings>
#include <QCheckBox>
#include <QScrollBar>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QObject>
#include <QPainter>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

class CVImageWidget : public QWidget {
	Q_OBJECT
public:
	explicit CVImageWidget(QWidget *parent = 0) : QWidget(parent) {}
	QSize sizeHint() const { return _qimage.size(); }
	QSize minimumSizeHint() const { return _qimage.size(); }
	void setClickHandlers(function<void(int, int)>d, function<void(int, int)>u, function<void(int, int)>mv) {
		upClick = u;
		downClick = d;
		moveClick = mv;
	}
	public slots :
	void showImage(const cv::Mat& image) {
		switch (image.type()) {
		case CV_8UC1:
			cvtColor(image, _tmp, CV_GRAY2RGB);
			break;
		case CV_8UC3:
			cvtColor(image, _tmp, CV_BGR2RGB);
			break;
		}
		assert(_tmp.isContinuous());
		_qimage = QImage(_tmp.data, _tmp.cols, _tmp.rows, _tmp.cols * 3, QImage::Format_RGB888);
		this->setFixedSize(image.cols, image.rows);
		repaint();
	}
	bool eventFilter(QObject *object, QEvent *event) {
		if (event->type() == QEvent::MouseButtonRelease) {
			mouseReleaseEvent((QMouseEvent*)event);
		}
		return false;
	}
protected:
	void mousePressEvent(QMouseEvent *event) {
		if (event->buttons() & Qt::LeftButton) {
			QPoint pt = event->pos();
			downClick(pt.x(), pt.y());
			mouseDown = true;
		}
	}
	void mouseMoveEvent(QMouseEvent* event) {
		QPoint pt = event->pos();
		if (mouseDown)
			moveClick(pt.x(), pt.y());
	}
	void mouseReleaseEvent(QMouseEvent *event) {
		QPoint pt = event->pos();
		upClick(pt.x(), pt.y());
		mouseDown = false;
	}
	void paintEvent(QPaintEvent*) {
		QPainter painter(this);
		painter.drawImage(QPoint(0, 0), _qimage);
		painter.end();
	}
	function<void(int, int)> downClick, upClick, moveClick;
	bool mouseDown = false;
	QImage _qimage;
	cv::Mat _tmp;
};
class CMyWidget : public QWidget {
	Q_OBJECT
private:
	function<void()>cb;
public:
	explicit CMyWidget(QWidget *parent = 0) : QWidget(parent) {}
	void setCallback(function<void()>cb_) {
		this->cb = cb_;
	}
	bool eventFilter(QObject *object, QEvent *event) {
		return false;
	}
	public slots:
	void onTimer() {
		cb();
	}
};

Mat calcLBP(Mat img, int D, int tresh) {
	Mat ret(img.rows, img.cols, CV_8UC1);
	
	for (int y = 0; y < img.rows;y++) {
		vector<int> intencies(256,0);
		//fill(intencies.begin(), intencies.end(), 0);
		int cnt = 0;		
		for (int x = -D; x < img.cols; x++) {
			for (int d = -D; d <= D; d++) {
				int x1 = x + D;
				int y1 = y - d;
				if (x1 >= 0 && y1 >= 0 && x1 < img.cols && y1 < img.rows) {
					intencies[img.at<uchar>(y1, x1)]++;
					cnt++;
				}
				int x2 = x - D;
				int y2 = y - d;
				if (x2 >= 0 && y2 >= 0 && x2 < img.cols && y2 < img.rows) {
					intencies[img.at<uchar>(y2, x2)]--;
					cnt--;
				}					
			}
			if (x >= 0) {
				int s = 0,s1=0;
				auto m = 0;				
				for (int i = 0; i < 255; i++) {
					s += intencies[i];
					if (s > cnt / 2) {
						m = i;
						break;
					}
				}
				auto val = img.at<uchar>(y, x);
				auto r = 128;
				if (val <m-tresh) r = 0;
				if (val > m+tresh) r = 255;				
				ret.at<uchar>(y, x) = r;
			}
		}
		//cout << "y=" << y << endl;
	}
	//cout << "calc" << endl;
	return ret;
}
int main(int argc, char *argv[]) {
/*	vector<uchar> arr;
	arr.push_back(2); arr.push_back(2); arr.push_back(2); arr.push_back(2);
	arr.push_back(3); arr.push_back(3);
	arr.push_back(5);
	arr.push_back(7); arr.push_back(7);
	remove_sorted(arr, (unsigned char)7);
	exit(0);*/
	QApplication app(argc, argv);
	VideoCapture cap;
	if (argc == 1) {
		if (!cap.open(0)) {
			cout << "can't access camera..." << endl;
			return 0;
		}
	} else {
		if (!argv[1] || !cap.open(argv[1])) {
			cout << "can't open file " << argv[1] << endl;
			return 0;
		}
	}
	QHBoxLayout *upTreshold = new QHBoxLayout;
	QHBoxLayout *threadSetup = new QHBoxLayout;
	QHBoxLayout *sharpLayout = new QHBoxLayout;
	QScrollBar* upScroll = new QScrollBar(Qt::Horizontal);
	QScrollBar* downScroll = new QScrollBar(Qt::Horizontal);
//	Rect r;
	int watchMode = 0;
	auto plus = new QPushButton("+");
	auto minus = new QPushButton("-");
	auto watch = new QPushButton("WATCH");
	plus->setMaximumWidth(50);
	minus->setMaximumWidth(50);
	threadSetup->addWidget(upScroll);
	threadSetup->addWidget(downScroll);
	QObject::connect(plus, &QPushButton::clicked, [&]() {
	});
	QObject::connect(watch, &QPushButton::clicked, [&]() {
	});
	QObject::connect(minus, &QPushButton::clicked, [&]() {
	});
	int sz = 10;
	int tresh = 30;
	QObject::connect(upScroll, &QScrollBar::valueChanged, [&]() {
		sz= upScroll->value();
	});
	QObject::connect(downScroll, &QScrollBar::valueChanged, [&]() {
		tresh = downScroll->value();
		cout << tresh << endl;
	});
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(upTreshold);
	mainLayout->addLayout(threadSetup);
	CVImageWidget* imageWidget = new CVImageWidget();
	mainLayout->addWidget(imageWidget);
	auto *w = new CMyWidget();
	w->installEventFilter(w);
	imageWidget->installEventFilter(imageWidget);
	w->setLayout(mainLayout);
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	w->setWindowTitle("thread detector v1.0");
	w->show();
	auto qTimer = new QTimer(w);
	qTimer->setInterval(1000 / 20);
	imageWidget->setClickHandlers([&](int x, int y) {
	}, [&](int x, int y) {
	}, [&](int x, int y) {
	});
	imageWidget->setMouseTracking(true);
	w->setCallback([&] {
		Mat img,bw;
		cap >> img;
		cvtColor(img, bw, CV_RGB2GRAY);
		imageWidget->showImage(calcLBP(bw,sz,tresh));
	});
	w->connect(qTimer, SIGNAL(timeout()), w, SLOT(onTimer()));
	qTimer->start();
	return app.exec();
}
#include "main.moc"