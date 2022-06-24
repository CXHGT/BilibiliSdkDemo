#include "BilibiliGetCodeDialog.h"
#include "./ui_BilibiliGetCodeDialog.h"
#include <QMouseEvent>
#include <QPushButton>
#include <qdesktopservices.h>

BilibiliGetCodeDialog::BilibiliGetCodeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BilibiliGetCodeDialog)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    connect(ui->closeButton,&QPushButton::clicked,this,&QDialog::reject);
    connect(ui->startButton, &QPushButton::clicked,this, &QDialog::accept);

    connect(ui->getCodeButton,&QPushButton::clicked,[]{
        QString URL = "https://link.bilibili.com/p/center/index/user-center/my-info/operation#/my-room/start-live";
        QDesktopServices::openUrl(QUrl(URL.toLatin1()));
    });
    connect(ui->clearButton,&QPushButton::clicked, [&]{
        ui->codeLineEdit->setText("");
    });

}

bool BilibiliGetCodeDialog::isSaveCode()
{
    return ui->radioButton->isChecked();
}

std::string BilibiliGetCodeDialog::getCodeText()
{
    return ui->codeLineEdit->text().toStdString();
}

void BilibiliGetCodeDialog::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		QRect rect = this->rect();  //rect是鼠标实现可拖动的区域
		rect.setBottom(rect.top() + this->height());
		if (rect.contains(event->pos()))
		{
			m_bDragging = true;
			m_poStartPosition = event->globalPos();
			m_poFramePosition = frameGeometry().topLeft();
		}	
	}
	QWidget::mousePressEvent(event);
}

void BilibiliGetCodeDialog::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton)	//只响应鼠标所见
	{
		if (m_bDragging)
		{
			// delta 相对偏移量, 
			QPoint delta = event->globalPos() - m_poStartPosition;
			// 新位置：窗体原始位置  + 偏移量
			move(m_poFramePosition + delta);
		}
	}
	QWidget::mouseMoveEvent(event);
}

void BilibiliGetCodeDialog::mouseReleaseEvent(QMouseEvent* event)
{
	m_bDragging = false;
	QWidget::mouseReleaseEvent(event);
}

BilibiliGetCodeDialog::~BilibiliGetCodeDialog()
{
    delete ui;
}

