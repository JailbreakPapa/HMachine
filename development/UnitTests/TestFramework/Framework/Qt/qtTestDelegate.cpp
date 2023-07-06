#include <TestFramework/TestFrameworkPCH.h>

#ifdef WD_USE_QT

#  include <QApplication>
#  include <QPainter>
#  include <TestFramework/Framework/Qt/qtTestDelegate.h>
#  include <TestFramework/Framework/Qt/qtTestModel.h>

////////////////////////////////////////////////////////////////////////
// wdQtTestDelegate public functions
////////////////////////////////////////////////////////////////////////

wdQtTestDelegate::wdQtTestDelegate(QObject* pParent)
  : QStyledItemDelegate(pParent)
{
}

wdQtTestDelegate::~wdQtTestDelegate() {}

void wdQtTestDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (index.column() == wdQtTestModel::Columns::Duration)
  {
    // We need to draw the alternate background color here because setting it via the model would
    // overwrite our duration bar.
    pPainter->save();
    pPainter->setPen(Qt::NoPen);
    pPainter->setBrush(option.palette.alternateBase());
    pPainter->drawRect(option.rect);
    pPainter->restore();

    bool bSuccess = false;
    float fProgress = index.data(wdQtTestModel::UserRoles::Duration).toFloat(&bSuccess);

    // If we got a valid float from the model we can draw a small duration bar on top of the background.
    if (bSuccess)
    {
      QColor DurationColor = index.data(wdQtTestModel::UserRoles::DurationColor).value<QColor>();
      QStyleOptionViewItem option2 = option;
      option2.palette.setBrush(QPalette::Base, QBrush(DurationColor));
      option2.rect.setWidth((int)((float)option2.rect.width() * fProgress));
      QApplication::style()->drawControl(QStyle::CE_ProgressBarGroove, &option2, pPainter);
    }
  }

  QStyledItemDelegate::paint(pPainter, option, index);
}

#endif

WD_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestDelegate);
