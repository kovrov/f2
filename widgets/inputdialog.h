#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H



#include <QDialog>
#include <QPointer>


class QLabel;
class QVBoxLayout;
class QDialogButtonBox;
class QComboBox;

namespace widgets {



class InputDialog : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY (InputDialog)
    Q_PROPERTY (QString labelText READ labelText WRITE setLabelText)

public:
    explicit InputDialog(QWidget *parent = 0, Qt::WindowFlags flags = 0);

    void setLabelText(const QString &text);
    QString labelText() const;

    void setInputWidget(QWidget *widget);
    QWidget * inputWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void setVisible(bool visible);

signals:

private slots:

private:  // really
    mutable QLabel *m_label;
    mutable QDialogButtonBox *m_buttonBox;
//    mutable QComboBox *m_comboBox;
    mutable QVBoxLayout *m_mainLayout;
    QPointer<QWidget> m_inputWidget;
};



}  // namespace widgets

#endif // INPUTDIALOG_H
