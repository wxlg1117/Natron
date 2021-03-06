/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****
#include "TableItemAnim.h"

#include <map>

#include <QTreeWidgetItem>

#include "Engine/KnobItemsTable.h"

#include "Gui/AnimationModule.h"
#include "Gui/KnobItemsTableGui.h"


#include <map>
#include <QTreeWidgetItem>

#include "Engine/AppInstance.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Project.h"


#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobAnim.h"
#include "Gui/NodeAnim.h"
#include "Gui/KnobItemsTableGui.h"

NATRON_NAMESPACE_ENTER

struct ItemCurve
{
    QTreeWidgetItem* item;
    //CurveGuiPtr curve;
};
typedef std::map<ViewIdx, ItemCurve> PerViewItemMap;

class TableItemAnimPrivate
{
public:

    TableItemAnimPrivate(const AnimationModuleBasePtr& model)
    : model(model)
    , tableItem()
    , table()
    , parentNode()
    , nameItem(0)
    , animationItems()
    , knobs()
    , children()
    {

    }

    AnimationModuleBaseWPtr model;
    KnobTableItemWPtr tableItem;
    KnobItemsTableGuiWPtr table;
    NodeAnimWPtr parentNode;
    QTreeWidgetItem* nameItem;
    PerViewItemMap animationItems;
    std::vector<KnobAnimPtr> knobs;
    std::vector<TableItemAnimPtr> children;


};


TableItemAnim::TableItemAnim(const AnimationModuleBasePtr& model,
                             const KnobItemsTableGuiPtr& table,
                             const NodeAnimPtr &parentNode,
                             const KnobTableItemPtr& item)
: AnimItemBase(model)
, _imp(new TableItemAnimPrivate(model))
{
    _imp->table = table;
    _imp->parentNode = parentNode;
    _imp->tableItem = item;

    connect(item.get(), SIGNAL(labelChanged(QString,TableChangeReasonEnum)), this, SLOT(onInternalItemLabelChanged(QString,TableChangeReasonEnum)));
    connect(item.get(), SIGNAL(availableViewsChanged()), this, SLOT(onInternalItemAvailableViewsChanged()));

    connect(item->getApp()->getProject().get(), SIGNAL(projectViewsChanged()), this, SLOT(onProjectViewsChanged()));
}

TableItemAnim::~TableItemAnim()
{
    destroyItems();
}

void
TableItemAnim::destroyItems()
{

    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        _imp->knobs[i]->destroyItems();
    }
    for (std::size_t i = 0; i < _imp->children.size(); ++i) {
        _imp->children[i]->destroyItems();
    }
    _imp->knobs.clear();
    _imp->children.clear();
    
    delete _imp->nameItem;
    _imp->nameItem = 0;
    

}


void
TableItemAnim::insertChild(int index, const TableItemAnimPtr& child)
{
    if (index < 0 || index >= (int)_imp->children.size()) {
        _imp->children.push_back(child);
    } else {
        std::vector<TableItemAnimPtr>::iterator it = _imp->children.begin();
        std::advance(it, index);
        _imp->children.insert(it, child);
    }

}


QTreeWidgetItem*
TableItemAnim::getRootItem() const
{
    return _imp->nameItem;
}

QTreeWidgetItem *
TableItemAnim::getTreeItem(DimSpec /*dimension*/, ViewSetSpec view) const
{
    if (view.isAll()) {
        return 0;
    }
    PerViewItemMap::const_iterator foundView = _imp->animationItems.find(ViewIdx(view));
    if (foundView == _imp->animationItems.end()) {
        return 0;
    }
    return foundView->second.item;
}

CurvePtr
TableItemAnim::getCurve(DimIdx dimension, ViewIdx view) const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        return CurvePtr();
    }
    return item->getAnimationCurve(view, dimension);
}

CurveGuiPtr
TableItemAnim::getCurveGui(DimIdx /*dimension*/, ViewIdx /*view*/) const
{
    return CurveGuiPtr();
#if 0
    PerViewItemMap::const_iterator foundView = _imp->animationItems.find(view);
    if (foundView == _imp->animationItems.end()) {
        return CurveGuiPtr();
    }
    return foundView->second.curve;
#endif
}

QString
TableItemAnim::getViewDimensionLabel(DimIdx dimension, ViewIdx view) const
{
    QString ret = _imp->nameItem->text(0);

    QTreeWidgetItem* item = getTreeItem(dimension, view);
    if (item) {
        ret += QLatin1Char(' ');
        ret += item->text(0);
    }
    return ret;
}

std::list<ViewIdx>
TableItemAnim::getViewsList() const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        return std::list<ViewIdx>();
    }
    return item->getViewsList();
}

int
TableItemAnim::getNDimensions() const
{
    return 1;
}


KnobTableItemPtr
TableItemAnim::getInternalItem() const
{
    return _imp->tableItem.lock();
}

AnimatingObjectIPtr
TableItemAnim::getInternalAnimItem() const
{
    return _imp->tableItem.lock();
}

NodeAnimPtr
TableItemAnim::getNode() const
{
    return _imp->parentNode.lock();
}

const std::vector<TableItemAnimPtr>&
TableItemAnim::getChildren() const
{
    return _imp->children;
}

const std::vector<KnobAnimPtr>&
TableItemAnim::getKnobs() const
{
    return _imp->knobs;
}

void
TableItemAnim::onInternalItemLabelChanged(const QString& label, TableChangeReasonEnum)
{
    if (_imp->nameItem) {
        _imp->nameItem->setText(0, label);
    }
}

bool
TableItemAnim::isRangeDrawingEnabled() const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        return false;
    }
#pragma message WARN("ToDo")
    return false;
}

RangeD
TableItemAnim::getFrameRange() const
{
#pragma message WARN("ToDo")
    RangeD ret = {0,0};
    return ret;
}

TableItemAnimPtr
TableItemAnim::findTableItem(const KnobTableItemPtr& item) const
{
    for (std::size_t i = 0; i < _imp->children.size(); ++i) {
        if (_imp->children[i]->getInternalItem() == item) {
            return _imp->children[i];
        } else {
            TableItemAnimPtr foundChild = _imp->children[i]->findTableItem(item);
            if (foundChild) {
                return foundChild;
            }
        }
    }
    return TableItemAnimPtr();
}

TableItemAnimPtr
TableItemAnim::removeItem(const KnobTableItemPtr& item)
{
    TableItemAnimPtr ret;
    for (std::vector<TableItemAnimPtr>::iterator it = _imp->children.begin(); it!=_imp->children.end(); ++it) {
        if ((*it)->getInternalItem() == item) {
            ret = *it;
            _imp->children.erase(it);
            break;
        } else {
            ret = (*it)->removeItem(item);
            if (ret) {
                break;
            }
        }
    }
    if (ret) {
        _imp->model.lock()->getSelectionModel()->removeAnyReferenceFromSelection(ret);
    }
    return ret;
}


void
TableItemAnim::initialize(QTreeWidgetItem* parentItem)
{
    KnobTableItemPtr item = getInternalItem();
    QString itemLabel = QString::fromUtf8( item->getLabel().c_str() );

    AnimItemBasePtr thisShared = shared_from_this();
    AnimationModuleBasePtr model = getModel();

    _imp->nameItem = new QTreeWidgetItem;
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_ITEM_POINTER, qVariantFromValue((void*)thisShared.get()));
    _imp->nameItem->setText(0, itemLabel);
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_TYPE, eAnimatedItemTypeTableItemRoot);
    _imp->nameItem->setExpanded(true);
    int nCols = getModel()->getTreeColumnsCount();
    if (nCols > ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE) {
        _imp->nameItem->setData(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QT_ROLE_CONTEXT_ITEM_VISIBLE, QVariant(true));
        _imp->nameItem->setIcon(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, model->getItemVisibilityIcon(true));
    }
    if (nCols > ANIMATION_MODULE_TREE_VIEW_COL_PLUGIN_ICON) {
        QString iconFilePath = QString::fromUtf8(item->getIconLabelFilePath().c_str());
        AnimationModuleTreeView::setItemIcon(iconFilePath, _imp->nameItem);
    }
    assert(parentItem);
    parentItem->addChild(_imp->nameItem);


    if (item->getCanAnimateUserKeyframes()) {
        createViewItems();
    }

    KnobItemsTableGuiPtr table = _imp->table.lock();
    NodeAnimPtr parentNode = _imp->parentNode.lock();
    std::vector<KnobTableItemPtr> children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        TableItemAnimPtr child(TableItemAnim::create(model, table, parentNode, children[i], _imp->nameItem));
        _imp->children.push_back(child);
    }

    initializeKnobsAnim();
}

void
TableItemAnim::createViewItems()
{
    KnobTableItemPtr item = getInternalItem();
    AnimationModuleBasePtr model = getModel();

    //AnimationModuleView* curveWidget = model->getView();
    AnimItemBasePtr thisShared = shared_from_this();

    std::list<ViewIdx> views = item->getViewsList();
    const std::vector<std::string>& projectViews = item->getApp()->getProject()->getProjectViewNames();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        QString viewName;
        if (*it >= 0 && *it < (int)projectViews.size()) {
            viewName = QString::fromUtf8(projectViews[*it].c_str());
        }
        QTreeWidgetItem* animationItem = new QTreeWidgetItem;
        QString itemLabel;
        if (views.size() > 1) {
            itemLabel += viewName + QString::fromUtf8(" ");
        }
        itemLabel += tr("Animation");
        animationItem->setData(0, QT_ROLE_CONTEXT_ITEM_POINTER, qVariantFromValue((void*)thisShared.get()));
        animationItem->setText(0, itemLabel);
        animationItem->setData(0, QT_ROLE_CONTEXT_TYPE, eAnimatedItemTypeTableItemAnimation);
        animationItem->setExpanded(true);
        int nCols = getModel()->getTreeColumnsCount();
        if (nCols > ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE) {
            animationItem->setData(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QT_ROLE_CONTEXT_ITEM_VISIBLE, QVariant(true));
            animationItem->setIcon(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, model->getItemVisibilityIcon(true));
        }



        ItemCurve& ic = _imp->animationItems[*it];
        ic.item = animationItem;
        /*if (curveWidget) {
            ic.curve.reset(new CurveGui(curveWidget, thisShared, DimIdx(0), *it));
        }*/

        _imp->nameItem->addChild(animationItem);
    }

}

void
TableItemAnim::initializeKnobsAnim()
{
    KnobTableItemPtr item = _imp->tableItem.lock();
    const KnobsVec& knobs = item->getKnobs();
    TableItemAnimPtr thisShared = toTableItemAnim(shared_from_this());
    AnimationModuleBasePtr model = getModel();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {


        // If the knob is not animted, don't create an item
        if ( !(*it)->canAnimate() || !(*it)->isAnimationEnabled() ) {
            continue;
        }
        KnobAnimPtr knobAnimObject(KnobAnim::create(model, thisShared, _imp->nameItem, *it));
        _imp->knobs.push_back(knobAnimObject);

    } // for all knobs
} // initializeKnobsAnim


void
TableItemAnim::refreshVisibilityConditional(bool refreshHolder)
{
    NodeAnimPtr parentNode = _imp->parentNode.lock();
    if (refreshHolder && parentNode) {
        // Refresh parent which will refresh this item
        parentNode->refreshVisibility();
        return;
    }


    AnimationModulePtr animModule = toAnimationModule(getModel());
    bool onlyShowIfAnimated = false;
    if (animModule) {
        onlyShowIfAnimated = animModule->getEditor()->isOnlyAnimatedItemsVisibleButtonChecked();
    }

    // Refresh knobs
    bool showItem = false;
    const std::vector<KnobAnimPtr>& knobRows = getKnobs();
    for (std::vector<KnobAnimPtr>::const_iterator it = knobRows.begin(); it != knobRows.end(); ++it) {
        (*it)->refreshVisibilityConditional(false /*refreshHolder*/);
        if (!(*it)->getRootItem()->isHidden()) {
            showItem = true;
        }
    }


    // Refresh children
    for (std::vector<TableItemAnimPtr>::const_iterator it = _imp->children.begin(); it!=_imp->children.end(); ++it) {
        (*it)->refreshVisibilityConditional(false);
        if (!(*it)->getRootItem()->isHidden()) {
            showItem = true;
        }
    }
    if (!showItem) {
        for (PerViewItemMap::const_iterator it = _imp->animationItems.begin(); it!=_imp->animationItems.end(); ++it) {

            CurvePtr c = getInternalAnimItem()->getAnimationCurve(it->first, DimIdx(0));
            if (!c) {
                continue;
            }
            if (c->isAnimated() || !onlyShowIfAnimated) {
                it->second.item->setHidden(false);
                showItem = true;
            } else {
                it->second.item->setHidden(true);
            }
        }
    }
    _imp->nameItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, showItem);
    _imp->nameItem->setHidden(!showItem);
}

void
TableItemAnim::refreshVisibility()
{
    refreshVisibilityConditional(true);
}

void
TableItemAnim::onProjectViewsChanged()
{
    const std::vector<std::string>& projectViewNames = getInternalItem()->getApp()->getProject()->getProjectViewNames();
    for (PerViewItemMap::const_iterator it = _imp->animationItems.begin(); it != _imp->animationItems.end(); ++it) {
        QString viewName;
        if (it->first >= 0 && it->first < (int)projectViewNames.size()) {
            viewName = QString::fromUtf8(projectViewNames[it->first].c_str());
        } else {
            viewName = it->second.item->text(0);
            QString notFoundSuffix = QString::fromUtf8("(*)");
            if (!viewName.endsWith(notFoundSuffix)) {
                viewName += QLatin1Char(' ');
                viewName += notFoundSuffix;
            }
        }
        it->second.item->setText(0, viewName);
    }
}

void
TableItemAnim::onInternalItemAvailableViewsChanged()
{
    for (PerViewItemMap::const_iterator it = _imp->animationItems.begin(); it != _imp->animationItems.end(); ++it) {
        delete it->second.item;
    }
    _imp->animationItems.clear();
    createViewItems();

    refreshVisibility();

    getModel()->refreshSelectionBboxAndUpdateView();
}

bool
TableItemAnim::getAllDimensionsVisible(ViewIdx /*view*/) const
{
    return true;
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_TableItemAnim.cpp"
