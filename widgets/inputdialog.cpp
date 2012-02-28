#include <QLabel>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

#include <QComboBox>

#include "inputdialog.h"

using namespace widgets;



InputDialog::InputDialog(QWidget *parent, Qt::WindowFlags flags)
  : QDialog(parent, flags),
    m_label(0),
    m_buttonBox(0),
    m_mainLayout(0)
{
}


void InputDialog::setLabelText(const QString &text)
{
    if (!m_label)
        m_label = new QLabel(text, this);
    else
        m_label->setText(text);
}


QString InputDialog::labelText() const
{
    return m_label->text();
}


void InputDialog::setInputWidget(QWidget *widget)
{
    if (m_inputWidget == widget)
        return;

    if (m_mainLayout && m_inputWidget)
        m_mainLayout->removeWidget(m_inputWidget);

    if (m_inputWidget && m_inputWidget->parent() == this)
        delete m_inputWidget;

    m_inputWidget = widget;

    if (m_mainLayout) {
        m_mainLayout->insertWidget(1, m_inputWidget);
        m_label->setBuddy(m_inputWidget);
        return;
    }

    if (!m_label)
        m_label = new QLabel(InputDialog::tr("Enter a value:"), this);
    m_label->setBuddy(m_inputWidget);
    m_label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    QObject::connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_mainLayout = new QVBoxLayout(this);
    //we want to let the input dialog grow to available size on Symbian.
    m_mainLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_mainLayout->addWidget(m_label);
    m_mainLayout->addWidget(m_inputWidget);
    m_mainLayout->addWidget(m_buttonBox);
    m_inputWidget->show();
}


QWidget * InputDialog::inputWidget()
{
    return m_inputWidget;
}


QSize InputDialog::minimumSizeHint() const
{
    Q_ASSERT (m_mainLayout);
    return QDialog::minimumSizeHint();
}


QSize InputDialog::sizeHint() const
{
    Q_ASSERT (m_mainLayout);
    return QDialog::sizeHint();
}


void InputDialog::setVisible(bool visible)
{
    if (visible) {
        Q_ASSERT (m_mainLayout);
        m_inputWidget->setFocus();
    }
    QDialog::setVisible(visible);
}
