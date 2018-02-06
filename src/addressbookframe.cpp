// Copyright (c) 2015-2017, The Bytecoin developers
//
// This file is part of Bytecoin.
//
// Bytecoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Bytecoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Bytecoin.  If not, see <http://www.gnu.org/licenses/>.

#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QMessageBox>

#include "addressbookframe.h"
#include "addressbookdelegate.h"
#include "addressbookmanager.h"
#include "QuestionDialog.h"
//#include "Common/RightAlignmentColumnDelegate.h"
//#include "DonationColumnDelegate.h"
#include "addressbookmodel.h"
#include "newaddressdialog.h"
#include "ui_addressbookframe.h"
#include "mainwindow.h"

namespace WalletGUI
{

AddressBookFrame::AddressBookFrame(QWidget* parent)
    : QFrame(parent)
    , m_ui(new Ui::AddressBookFrame)
//    , m_addressBookManager(nullptr)
    , mainWindow_(nullptr)
    , m_sortedAddressBookModel(nullptr)
    , m_helperLabel(new QLabel(this))
    , m_addressBookDelegate(new AddressBookDelegate(this))
{
    m_ui->setupUi(this);
    QPixmap helperPixmap(":images/add_address_helper");
    m_helperLabel->setGeometry(helperPixmap.rect());
    m_helperLabel->setPixmap(helperPixmap);
//    m_ui->m_addressBookView->setHoverIsVisible(true);
    m_ui->m_addressBookView->setItemDelegateForColumn(AddressBookModel::COLUMN_ACTION, m_addressBookDelegate);
    connect(m_addressBookDelegate, &AddressBookDelegate::sendToSignal, this, &AddressBookFrame::sendToSignal);
    connect(m_addressBookDelegate, &AddressBookDelegate::editSignal, this, &AddressBookFrame::editClicked);
    connect(m_addressBookDelegate, &AddressBookDelegate::deleteSignal, this, &AddressBookFrame::deleteClicked);
//    connect(this, &AddressBookFrame::sendToSignal, this, &AddressBookFrame::sendToClicked);
}

AddressBookFrame::~AddressBookFrame() {
}

void AddressBookFrame::setAddressBookManager(AddressBookManager* manager)
{
    m_addressBookManager = manager;
}

void AddressBookFrame::setMainWindow(MainWindow* _mainWindow)
{
  mainWindow_ = _mainWindow;
  connect(this, &AddressBookFrame::sendToSignal, mainWindow_, &MainWindow::addRecipient);
}

void AddressBookFrame::setSortedAddressBookModel(QAbstractItemModel* _model) {
  m_sortedAddressBookModel = _model;
  m_ui->m_addressBookView->setModel(m_sortedAddressBookModel);
//  m_ui->m_addressBookView->setItemDelegateForColumn(AddressBookModel::COLUMN_ADDRESS, new RightAlignmentColumnDelegate(false, this));
  m_ui->m_addressBookView->header()->setSectionResizeMode(AddressBookModel::COLUMN_LABEL, QHeaderView::Fixed);
  m_ui->m_addressBookView->header()->setSectionResizeMode(AddressBookModel::COLUMN_ADDRESS, QHeaderView::Stretch);
//  m_ui->m_addressBookView->header()->setSectionResizeMode(AddressBookModel::COLUMN_DONATION, QHeaderView::Fixed);
  m_ui->m_addressBookView->header()->setSectionResizeMode(AddressBookModel::COLUMN_ACTION, QHeaderView::Fixed);
  m_ui->m_addressBookView->header()->setResizeContentsPrecision(-1);
  m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_LABEL, 250);
//  m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_DONATION, 90);
  m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_ACTION, 40);
  connect(m_sortedAddressBookModel, &QAbstractItemModel::rowsInserted, this, &AddressBookFrame::rowsInserted);
  connect(m_sortedAddressBookModel, &QAbstractItemModel::rowsRemoved, this, &AddressBookFrame::rowsRemoved);

  for (int i = 0; i < m_sortedAddressBookModel->rowCount(); ++i) {
    QPersistentModelIndex index = m_sortedAddressBookModel->index(i, AddressBookModel::COLUMN_ACTION);
    m_ui->m_addressBookView->openPersistentEditor(index);
  }

  if (m_sortedAddressBookModel->rowCount() > 0) {
    m_helperLabel->hide();
  }
}

void AddressBookFrame::resizeEvent(QResizeEvent* /*_event*/) {
  QRect addButtonRect = m_ui->m_addAddressButton->geometry();
  QPoint addButtonCenter = addButtonRect.center();
  QPoint helperBottomRight(addButtonCenter.x(), addButtonRect.y() - 20);
  QRect helperRect = m_helperLabel->geometry();
  helperRect.moveBottomRight(helperBottomRight);
  m_helperLabel->setGeometry(helperRect);
  m_helperLabel->updateGeometry();
  m_helperLabel->raise();
}

void AddressBookFrame::rowsInserted(const QModelIndex& /*_parent*/, int _first, int _last) {
  for (int i = _first; i <= _last; ++i) {
    QPersistentModelIndex index = m_sortedAddressBookModel->index(i, AddressBookModel::COLUMN_ACTION);
    m_ui->m_addressBookView->openPersistentEditor(index);
  }

  m_helperLabel->hide();
}

void AddressBookFrame::rowsRemoved(const QModelIndex& /*_parent*/, int /*_first*/, int /*_last*/) {
  if (m_sortedAddressBookModel->rowCount() == 0) {
    m_helperLabel->show();
  }
}

//void AddressBookFrame::sendToClicked(const QString& _address, const QString& label)
//{
//    mainWindow_->addRecipient(address, label)
//}

void AddressBookFrame::addClicked() {
  NewAddressDialog dlg(m_addressBookManager, mainWindow_);
  if (dlg.exec() == QDialog::Accepted) {
    QString label = dlg.getLabel();
    QString address = dlg.getAddress();
    m_addressBookManager->addAddress(label, address);
  }
}

void AddressBookFrame::editClicked(const QPersistentModelIndex& _index) {
  NewAddressDialog dlg(m_addressBookManager, _index, mainWindow_);
  if (dlg.exec() == QDialog::Accepted) {
    QString label = dlg.getLabel();
    QString address = dlg.getAddress();
    m_addressBookManager->editAddress(_index.data(AddressBookModel::ROLE_ROW).toInt(), label, address);
  }
}

void AddressBookFrame::deleteClicked(const QPersistentModelIndex& _index) {
  QString text(tr("Are you sure you would like to delete this address?"));

  QuestionDialog dlg(tr("Delete addresss"), text, mainWindow_);
  if (dlg.exec() == QDialog::Accepted) {
    m_addressBookManager->removeAddress(_index.data(AddressBookModel::ROLE_ROW).toInt());
  }
}

void AddressBookFrame::contextMenu(const QPoint& _pos) {
  QPersistentModelIndex index = m_ui->m_addressBookView->indexAt(_pos);
  if (!index.isValid()) {
    return;
  }

  QMenu menu;
  menu.setObjectName("m_addressBookMenu");
  QAction* sendAction = new QAction(tr("Send"), &menu);
  QAction* editAction = new QAction(tr("Edit"), &menu);
  QAction* copyAction = new QAction(tr("Copy to clipboard"), &menu);
  QAction* delAction = new QAction(tr("Delete"), &menu);
  menu.addAction(sendAction);
  menu.addAction(editAction);
  menu.addAction(copyAction);
  menu.addAction(delAction);

  connect(sendAction, &QAction::triggered, [this, index]() {
      Q_EMIT sendToSignal(
                  index.data(AddressBookModel::ROLE_ADDRESS).toString(),
                  index.data(AddressBookModel::ROLE_LABEL).toString());
    });
  connect(editAction, &QAction::triggered, [this, index]() {
      Q_EMIT this->editClicked(index);
    });
  connect(copyAction, &QAction::triggered, [index]() {
      QApplication::clipboard()->setText(index.data(AddressBookModel::ROLE_ADDRESS).toString());
    });
  connect(delAction, &QAction::triggered, [this, index]() {
      this->deleteClicked(index);
    });

  menu.exec(m_ui->m_addressBookView->mapToGlobal(_pos) + QPoint(-10, 20));
}

}