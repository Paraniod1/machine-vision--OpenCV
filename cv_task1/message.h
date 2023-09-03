#ifndef MESSAGE_H
#define MESSAGE_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <mutex>

enum MessageType {
    MESSAGE_TYPE_SUCCESS = 0x20,
    MESSAGE_TYPE_ERROR,
    MESSAGE_TYPE_WARNING,
    MESSAGE_TYPE_INFORMATION
};

class MessageItem;

class Message : public QObject
{
    Q_OBJECT
public:
    explicit Message(QObject *parent = nullptr);
    ~Message() override;

    /**
     * @brief Push 推入消息
     * @param type 消息类型
     * @param content 消息内容
     */
    void Push(MessageType type, QString content);

    /**
     * @brief SetDuration 设置消息显示的时间
     * @param nDuration 显示时间，必须大于等于0，若等于0则不消失
     */
    void SetDuration(int nDuration);

private:
    std::vector<MessageItem*> m_vecMessage;
    std::mutex m_qMtx;
    int m_nWidth;
    int m_nDuration;

private slots:
    void adjustItemPos(MessageItem* pItem);
    void removeItem(MessageItem* pItem);
};

class MessageItem : public QWidget
{
    Q_OBJECT
public:
    explicit MessageItem(QWidget* parent = nullptr,
                          MessageType type = MessageType::MESSAGE_TYPE_INFORMATION,
                          QString content = "");
    ~MessageItem() override;
    void Show();
    void Close();
    void SetDuration(int nDuration);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void AppearAnimation();
    void DisappearAnimation();

private:
    const int nIconMargin = 12;
    const int nLeftMargin = 64;
    const int nTopMargin = 10;
    const int nMinWidth = 400;
    const int nMinHeight = 70;
    QLabel* m_pLabelIcon;
    QLabel* m_pLabelContent;
    QTimer m_lifeTimer;
    int m_nWidth;
    int m_nHeight;
    int m_nDuration;

signals:
    void itemReadyRemoved(MessageItem* pItem);
    void itemRemoved(MessageItem* pItem);
};

#endif // MESSAGE_H
