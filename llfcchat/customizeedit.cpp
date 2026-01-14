#include "customizeedit.h"

CustomizeEdit::CustomizeEdit(QWidget *parent):QLineEdit (parent),_max_len(0)
{
    // 当用户在文本框中输入字符时，textChanged 信号会被触发，进而调用 limitTextLength 函数来限制输入文本的字节数。
    connect(this, &QLineEdit::textChanged, this, &CustomizeEdit::limitTextLength);
}

void CustomizeEdit::SetMaxLength(int maxLen)
{
    _max_len = maxLen;
}
