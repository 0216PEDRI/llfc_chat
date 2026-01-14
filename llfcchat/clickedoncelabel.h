#ifndef CLICKEDONCELABEL_H
#define CLICKEDONCELABEL_H

#include <QLabel>
#include <QMouseEvent>

class ClickedOnceLabel: public QLabel // 仅需点击触发时，用 ClickedOnceLabel; 需要状态切换、悬停效果、选中状态时，用 ClickedLabel
{
    Q_OBJECT
public:
    ClickedOnceLabel(QWidget *parent=nullptr);
    virtual void mouseReleaseEvent(QMouseEvent *ev) override; // 只重写 mouseReleaseEvent
signals:
    void clicked(QString);
};

#endif // CLICKEDONCELABEL_H
