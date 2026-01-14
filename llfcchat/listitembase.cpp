#include "listitembase.h"
#include <QStyleOption>
#include <QPainter>

ListItemBase::ListItemBase(QWidget *parent) : QWidget(parent)
{

}

void ListItemBase::SetItemType(ListItemType itemType)
{
    _itemType = itemType;
}

ListItemType ListItemBase::GetItemType()
{
    return _itemType;
}

void ListItemBase::paintEvent(QPaintEvent *event)
{
    QStyleOption opt; // 创建一个样式选项对象，用于指定绘制控件时的样式参数
    opt.initFrom(this);  // 初始化样式选项对象，基于当前控件的状态设置相关属性
    QPainter p(this);  // 创建一个 QPainter 对象，用于在当前控件（this）上进行绘制操作

    // 使用当前控件的样式来绘制控件的基本外观（例如背景、边框等）
    // QStyle::PE_Widget 表示控件的基础外观部分
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
