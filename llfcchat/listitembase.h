#ifndef LISTITEMBASE_H
#define LISTITEMBASE_H

#include <QWidget>
#include "global.h"

class ListItemBase : public QWidget // 列表项基类
{
    Q_OBJECT
public:
    explicit ListItemBase(QWidget *parent = nullptr);
    void SetItemType(ListItemType itemType);

    ListItemType GetItemType();

protected:
    virtual void paintEvent(QPaintEvent *event) override; // 重写paintEvent
private:
    ListItemType _itemType;

public slots:

signals:

};

#endif // LISTITEMBASE_H
