/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtDebugLeaksView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkQtDebugLeaksView.h"
#include "svtkObjectBase.h"
#include "svtkQtDebugLeaksModel.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
class svtkQtDebugLeaksView::qInternal
{
public:
  svtkQtDebugLeaksModel* Model;
  QSortFilterProxyModel* ProxyModel;
  QTableView* TableView;
  QTableView* ReferenceTableView;
  QCheckBox* FilterCheckBox;
  QLineEdit* FilterLineEdit;
};

//----------------------------------------------------------------------------
svtkQtDebugLeaksView::svtkQtDebugLeaksView(QWidget* p)
  : QWidget(p)
{
  this->Internal = new qInternal;

  this->Internal->Model = new svtkQtDebugLeaksModel(this);
  this->Internal->ProxyModel = new QSortFilterProxyModel(this->Internal->Model);
  this->Internal->ProxyModel->setSourceModel(this->Internal->Model);
  this->Internal->ProxyModel->setDynamicSortFilter(true);
  this->Internal->ProxyModel->setFilterKeyColumn(0);

  this->Internal->TableView = new QTableView;
  this->Internal->TableView->setSortingEnabled(true);
  this->Internal->TableView->setModel(this->Internal->ProxyModel);
  this->Internal->TableView->setObjectName("ClassTable");
  this->Internal->ReferenceTableView = new QTableView;
  this->Internal->ReferenceTableView->setObjectName("ReferenceTable");

  this->Internal->FilterCheckBox = new QCheckBox("Filter RegExp");
  this->Internal->FilterCheckBox->setChecked(true);
  this->Internal->FilterLineEdit = new QLineEdit();

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  QSplitter* splitter = new QSplitter();
  QPushButton* filterHelpButton = new QPushButton("RegExp Help");
  QHBoxLayout* filterLayout = new QHBoxLayout();

  filterLayout->addWidget(this->Internal->FilterCheckBox);
  filterLayout->addWidget(this->Internal->FilterLineEdit);
  filterLayout->addWidget(filterHelpButton);
  mainLayout->addLayout(filterLayout);
  mainLayout->addWidget(splitter);

  splitter->setOrientation(Qt::Vertical);
  splitter->addWidget(this->Internal->TableView);
  splitter->addWidget(this->Internal->ReferenceTableView);
  QList<int> sizes;
  sizes << 1 << 0;
  splitter->setSizes(sizes);

  this->connect(this->Internal->FilterLineEdit, SIGNAL(textChanged(const QString&)),
    SLOT(onFilterTextChanged(const QString&)));

  this->connect(this->Internal->FilterCheckBox, SIGNAL(stateChanged(int)), SLOT(onFilterToggled()));

  this->connect(filterHelpButton, SIGNAL(clicked()), SLOT(onFilterHelp()));

  this->connect(this->Internal->TableView->selectionModel(),
    SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(onCurrentRowChanged(const QModelIndex&)));

  this->connect(this->Internal->TableView, SIGNAL(doubleClicked(const QModelIndex&)), this,
    SLOT(onRowDoubleClicked(const QModelIndex&)));

  this->connect(this->Internal->ReferenceTableView, SIGNAL(doubleClicked(const QModelIndex&)), this,
    SLOT(onRowDoubleClicked(const QModelIndex&)));

  this->resize(400, 600);
  this->setWindowTitle("SVTK Debug Leaks View");
  this->Internal->TableView->setColumnWidth(0, 200);
  this->Internal->TableView->horizontalHeader()->setStretchLastSection(true);
  this->Internal->TableView->verticalHeader()->setVisible(false);
  this->Internal->TableView->setSelectionMode(QAbstractItemView::SingleSelection);
  this->Internal->TableView->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->Internal->ReferenceTableView->setSelectionMode(QAbstractItemView::SingleSelection);
  this->Internal->ReferenceTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->setAttribute(Qt::WA_QuitOnClose, false);
}

//-----------------------------------------------------------------------------
svtkQtDebugLeaksView::~svtkQtDebugLeaksView()
{
  this->Internal->ReferenceTableView->setModel(nullptr);
  this->Internal->TableView->setModel(nullptr);
  delete this->Internal->Model;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
svtkQtDebugLeaksModel* svtkQtDebugLeaksView::model()
{
  return this->Internal->Model;
}

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksView::onFilterHelp()
{
  QDesktopServices::openUrl(QUrl("http://doc.trolltech.com/4.6/qregexp.html#introduction"));
}

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksView::onCurrentRowChanged(const QModelIndex& current)
{
  QStandardItemModel* newModel = nullptr;
  QAbstractItemModel* previousModel = this->Internal->ReferenceTableView->model();

  QModelIndex index = this->Internal->ProxyModel->mapToSource(current);
  if (index.isValid())
  {
    QModelIndex classNameIndex = this->Internal->Model->index(index.row(), 0);
    QString className = this->Internal->Model->data(classNameIndex).toString();
    newModel = this->Internal->Model->referenceCountModel(className);
  }

  if (newModel != previousModel)
  {
    this->Internal->ReferenceTableView->setModel(newModel);
    this->Internal->ReferenceTableView->resizeColumnsToContents();
    this->Internal->ReferenceTableView->horizontalHeader()->setStretchLastSection(true);
    delete previousModel;
  }
}

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksView::onFilterTextChanged(const QString& text)
{
  if (this->filterEnabled())
  {
    this->Internal->ProxyModel->setFilterRegExp(text);
  }
}

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksView::onFilterToggled()
{
  QString text = this->filterText();
  if (!this->filterEnabled())
  {
    text = "";
  }

  this->Internal->ProxyModel->setFilterRegExp(text);
}

//-----------------------------------------------------------------------------
bool svtkQtDebugLeaksView::filterEnabled() const
{
  return this->Internal->FilterCheckBox->isChecked();
}

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksView::setFilterEnabled(bool value)
{
  this->Internal->FilterCheckBox->setChecked(value);
}

//-----------------------------------------------------------------------------
QString svtkQtDebugLeaksView::filterText() const
{
  return this->Internal->FilterLineEdit->text();
}

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksView::setFilterText(const QString& text)
{
  this->Internal->FilterLineEdit->setText(text);
}

//-----------------------------------------------------------------------------
Q_DECLARE_METATYPE(svtkObjectBase*);

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksView::onRowDoubleClicked(const QModelIndex& index)
{
  if (index.model() == this->Internal->ReferenceTableView->model())
  {
    QModelIndex objectIndex = this->Internal->ReferenceTableView->model()->index(index.row(), 0);
    QVariant objectVariant =
      this->Internal->ReferenceTableView->model()->data(objectIndex, Qt::UserRole);
    svtkObjectBase* object = objectVariant.value<svtkObjectBase*>();
    this->onObjectDoubleClicked(object);
  }
  else
  {
    QModelIndex sourceIndex = this->Internal->ProxyModel->mapToSource(index);
    if (sourceIndex.isValid())
    {
      QString className =
        this->Internal->Model->data(this->Internal->Model->index(sourceIndex.row(), 0)).toString();
      this->onClassNameDoubleClicked(className);
    }
  }
}

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksView::onObjectDoubleClicked(svtkObjectBase* object)
{
  Q_UNUSED(object);
}

//-----------------------------------------------------------------------------
void svtkQtDebugLeaksView::onClassNameDoubleClicked(const QString& className)
{
  Q_UNUSED(className);
}
