#ifndef BILIBILIGETCODEDIALOG_H
#define BILIBILIGETCODEDIALOG_H

#include <QDialog>
#include <QPoint>
#include <string>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class BilibiliGetCodeDialog;
}
QT_END_NAMESPACE

class BilibiliGetCodeDialog : public QDialog
{
    Q_OBJECT

public:
    BilibiliGetCodeDialog(QWidget *parent = nullptr);
    ~BilibiliGetCodeDialog();

private:
    Ui::BilibiliGetCodeDialog *ui;
    bool m_bDragging;         // 是否正在拖动
    QPoint m_poStartPosition; // 拖动开始前的鼠标位置
    QPoint m_poFramePosition; // 窗体的原始位置

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

public:
    bool isSaveCode();
    std::string getCodeText();
};
#endif // BILIBILIGETCODEDIALOG_H
