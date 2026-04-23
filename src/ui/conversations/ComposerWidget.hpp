#ifndef VOX_UI_CONVERSATIONS_COMPOSERWIDGET_HPP
#define VOX_UI_CONVERSATIONS_COMPOSERWIDGET_HPP

#include <QPlainTextEdit>
#include <QPushButton>
#include <QWidget>

namespace vox::ui::conversations {

class ComposerWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ComposerWidget(QWidget *parent = nullptr);

signals:
    void sendRequested(const QString &text);

private:
    QPlainTextEdit *m_edit{nullptr};
    QPushButton *m_sendButton{nullptr};
};

} // namespace vox::ui::conversations

#endif // VOX_UI_CONVERSATIONS_COMPOSERWIDGET_HPP
