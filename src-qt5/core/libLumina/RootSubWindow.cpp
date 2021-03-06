//===========================================
//  Lumina Desktop source code
//  Copyright (c) 2017, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "RootSubWindow.h"
#include <QDebug>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

#define WIN_BORDER 5

// === PUBLIC ===
RootSubWindow::RootSubWindow(QWidget *root, NativeWindow *win) : QFrame(root){
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setMouseTracking(true);
  //Create the QWindow and QWidget containers for the window
  WIN = win;
  closing = false;
  //WinWidget = QWidget::createWindowContainer( WIN->window(), this);
  initWindowFrame();
  //Hookup the signals/slots
  connect(WIN, SIGNAL(PropertiesChanged(QList<NativeWindow::Property>, QList<QVariant>)), this, SLOT(propertiesChanged(QList<NativeWindow::Property>, QList<QVariant>)));
  WIN->addFrameWinID(WinWidget->winId());
  WIN->emit RequestReparent(WIN->id(), WinWidget->winId(), QPoint(0,0));
  LoadAllProperties();
  //QTimer::singleShot(20, this, SLOT(LoadAllProperties()) );
}

RootSubWindow::~RootSubWindow(){

}

WId RootSubWindow::id(){
  return WIN->id();
}

// === PRIVATE ===
RootSubWindow::ModState RootSubWindow::getStateAtPoint(QPoint pt, bool setoffset){
  //Note: pt should be in widget-relative coordinates, not global
  if(!WinWidget->geometry().contains(pt) && !titleBar->geometry().contains(pt)){
    //above the frame itself - need to figure out which quadrant it is in (8-directions)
    if(pt.y() < WIN_BORDER){
      //One of the top options
      if(pt.x() < WIN_BORDER){
	if(setoffset){ offset.setX(pt.x()); offset.setY(pt.y()); } //difference from top-left corner
	return ResizeTopLeft;
      }else if(pt.x() > (this->width()-WIN_BORDER)){
	if(setoffset){ offset.setX(this->width()-pt.x()); offset.setY(pt.y()); } //difference from top-right corner
	return ResizeTopRight;
      }else{
	if(setoffset){ offset.setX(0); offset.setY(pt.y()); } //difference from top edge (X does not matter)
	return ResizeTop;
      }
    }else if(pt.y() > (this->height()-WIN_BORDER) ){
      //One of the bottom options
      if(pt.x() < WIN_BORDER){
	if(setoffset){ offset.setX(pt.x()); offset.setY(this->height()-pt.y()); } //difference from bottom-left corner
	return ResizeBottomLeft;
      }else if(pt.x() > (this->width()-WIN_BORDER)){
	if(setoffset){ offset.setX(this->width()-pt.x()); offset.setY(this->height()-pt.y()); } //difference from bottom-right corner
	return ResizeBottomRight;
      }else{
	if(setoffset){ offset.setX(0); offset.setY(this->height() - pt.y()); } //difference from bottom edge (X does not matter)
	return ResizeBottom;
      }
    }else{
      //One of the side options
      if(pt.x() < WIN_BORDER){
	if(setoffset){ offset.setX(pt.x()); offset.setY(0); } //difference from left edge (Y does not matter)
	return ResizeLeft;
      }else if(pt.x() > (this->width()-WIN_BORDER) ){
	if(setoffset){ offset.setX(this->width()-pt.x()); offset.setY(0); } //difference from right edge (Y does not matter)
	return ResizeRight;
      }else{
	return Normal;
      }
    }
  }
  return Normal;
}

void RootSubWindow::setMouseCursor(ModState state, bool override){
  if(currentCursor==state && !override){ return; } //nothing to change
  Qt::CursorShape shape;
  switch(state){
    case Normal:
      shape = Qt::ArrowCursor;
      break;
    case Move:
      shape = Qt::SizeAllCursor;
      break;
    case ResizeTop:
      shape = Qt::SizeVerCursor;
      break;
    case ResizeTopRight:
      shape = Qt::SizeBDiagCursor;
      break;
    case ResizeRight:
      shape = Qt::SizeHorCursor;
      break;
    case ResizeBottomRight:
      shape = Qt::SizeFDiagCursor;
      break;
    case ResizeBottom:
      shape = Qt::SizeVerCursor;
      break;
    case ResizeBottomLeft:
      shape = Qt::SizeBDiagCursor;
      break;
    case ResizeLeft:
      shape = Qt::SizeHorCursor;
      break;
    case ResizeTopLeft:
      shape = Qt::SizeFDiagCursor;
      break;
  }
  if(override){
    QApplication::setOverrideCursor(QCursor(shape));
  }else{
    currentCursor = state;
    this->setCursor(shape);
  }
}

void RootSubWindow::initWindowFrame(){
  //qDebug() << "Create RootSubWindow Frame";
  mainLayout = new QVBoxLayout(this);
  titleBar = new QHBoxLayout();
 closeB = new QToolButton(this);
  maxB = new QToolButton(this);
  minB = new QToolButton(this);
  otherB = new QToolButton(this);
  titleLabel = new QLabel(this);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  otherM = new QMenu(this); //menu of other actions
    otherB->setMenu(otherM);
    otherB->setPopupMode(QToolButton::InstantPopup);
    otherB->setAutoRaise(true);
  WinWidget = new QWidget(this);
  connect(closeB, SIGNAL(clicked()), this, SLOT(triggerClose()) );
  connect(maxB, SIGNAL(clicked()), this, SLOT(toggleMaximize()) );
  connect(minB, SIGNAL(clicked()), this, SLOT(toggleMinimize()) );
  //Now assemble the frame layout based on the current settings
    titleBar->addWidget(otherB);
    titleBar->addWidget(titleLabel);
    titleBar->addWidget(minB);
    titleBar->addWidget(maxB);
    titleBar->addWidget(closeB);
  WinWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  mainLayout->addLayout(titleBar);
  mainLayout->addWidget(WinWidget);
  //Now all the stylesheet options
  this->setObjectName("WindowFrame");
    closeB->setObjectName("Button_Close");
    minB->setObjectName("Button_Minimize");
    maxB->setObjectName("Button_Maximize");
    otherM->setObjectName("Menu_Actions");
    titleLabel->setObjectName("Label_Title");
  this->setStyleSheet("QWidget#WindowFrame{background-color: rgba(0,0,0,125);} QWidget#Label_Title{background-color: transparent; color: white; }");
  //And adjust the margins
  mainLayout->setContentsMargins(WIN_BORDER,WIN_BORDER,WIN_BORDER,WIN_BORDER);
  mainLayout->setSpacing(0);
  titleBar->setSpacing(1);
  titleBar->setContentsMargins(0,0,0,0);
  //this->setLayout(mainLayout);
  //qDebug() << " - Done";
}

void RootSubWindow::LoadProperties( QList< NativeWindow::Property> list){
  QList<QVariant> vals;
  for(int i=0; i<list.length(); i++){
    if(list[i] == NativeWindow::Visible){ list.removeAt(i); i--; continue; }
    vals << WIN->property(list[i]);
    qDebug() << "Property:" << list[i] << vals[i];
  }
  propertiesChanged(list, vals);
  WIN->requestProperty(NativeWindow::Visible, true);
}

// === PUBLIC SLOTS ===
void RootSubWindow::clientClosed(){
  qDebug() << "Client Closed";
  closing = true;
  this->close();
}

void RootSubWindow::LoadAllProperties(){
  QList< NativeWindow::Property> list = WIN->allProperties();
   /*list.removeAll(NativeWindow::Visible);
   list << NativeWindow::Visible;*/
  LoadProperties(list);
}

//Button Actions - public so they can be tied to key shortcuts and stuff as well
void RootSubWindow::toggleMinimize(){

}

void RootSubWindow::toggleMaximize(){

}

void RootSubWindow::triggerClose(){
  WIN->requestClose();
}

void RootSubWindow::toggleSticky(){

}

void RootSubWindow::activate(){
  WIN->requestProperty(NativeWindow::Active, true);
}

//Mouse Interactivity
void RootSubWindow::startMoving(){
  //If the cursor is not over this window, move it to the center of the titlebar
  QPoint curpt = QCursor::pos(); //global coords
  if(!this->geometry().contains(curpt)){
    curpt = this->mapToGlobal(titleBar->geometry().center());
    QCursor::setPos(curpt);
  }
  //Calculate the offset
  activeState = Move;
  offset = this->mapFromGlobal(curpt);
  setMouseCursor(activeState, true); //this one is an override cursor
  //Also need to capture the mouse
  this->grabMouse();
}

void RootSubWindow::startResizing(){

}



// === PRIVATE SLOTS ===
void RootSubWindow::propertiesChanged(QList<NativeWindow::Property> props, QList<QVariant> vals){
  for(int i=0; i<props.length() && i<vals.length(); i++){
    if(vals[i].isNull()){ continue; } //not the same as a default/empty value - the property has just not been set yet
    //qDebug() << "RootSubWindow: Property Changed:" << props[i] << vals[i];
    switch(props[i]){
	case NativeWindow::Visible:
		qDebug() << "Got Visibility Change:" << vals[i];
		if(vals[i].toBool()){ WinWidget->setVisible(true); this->show(); }
		else{ this->hide(); }
		break;
	case NativeWindow::Title:
		titleLabel->setText(vals[i].toString());
		break;
	case NativeWindow::Icon:
		//qDebug() << "Got Icon Change:" << vals[i];
		otherB->setIcon(vals[i].value< QIcon>());
		break;
	case NativeWindow::GlobalPos:
		//qDebug() << "Got Global Pos:" << vals[i].toPoint();
		this->move( vals[i].toPoint() );
		break;
	case NativeWindow::Size:
		//qDebug() << "Got Widget Size:" << vals[i].toSize();
		//WinWidget->setSizeHint( vals[i].toSize() );
		//WinWidget->resize(vals[i].toSize() );
		this->resize( vals[i].toSize()+QSize( this->width()-WinWidget->width(), this->height()-WinWidget->height() ) );
		//qDebug() << " - Size after change:" << WinWidget->size() << this->size();
		break;
	case NativeWindow::MinSize:
		WinWidget->setMinimumSize(vals[i].toSize());
		break;
	case NativeWindow::MaxSize:
		WinWidget->setMaximumSize(vals[i].toSize());
		break;
	case NativeWindow::Active:
		//if(vals[i].toBool()){ WinWidget->setFocus(); }
		break;
	/*case NativeWindow::WindowFlags:
		this->setWindowFlags( val.value< Qt::WindowFlags >() );
		break;*/
	default:
		qDebug() << "Window Property Unused:" << props[i] << vals[i];
    }
  }
}

// === PROTECTED ===
void RootSubWindow::mousePressEvent(QMouseEvent *ev){
  //qDebug() << "Frame Mouse Press Event";
  offset.setX(0); offset.setY(0);
  if(activeState != Normal){ return; } // do nothing - already in a state of grabbed mouse
  this->activate();
  if(this->childAt(ev->pos())!=0){
    //Check for any non-left-click event and skip it
    if(ev->button()!=Qt::LeftButton){ return; }
    activeState = Move;
    offset.setX(ev->pos().x()); offset.setY(ev->pos().y());
  }else{
    //Clicked on the frame somewhere
    activeState = getStateAtPoint(ev->pos(), true); //also have it set the offset variable
  }
  setMouseCursor(activeState, true); //this one is an override cursor

}

void RootSubWindow::mouseMoveEvent(QMouseEvent *ev){
  ev->accept();
  if(activeState == Normal){
    setMouseCursor( getStateAtPoint(ev->pos()) ); //just update the mouse cursor
  }else{
    //Currently in a modification state
    QRect geom = this->geometry();
    QSize minsize(WinWidget->minimumSize().width() + (2*WIN_BORDER), WinWidget->minimumSize().height()+(2*WIN_BORDER)+titleBar->geometry().size().height());
    switch(activeState){
      case Move:
        geom.moveTopLeft(ev->globalPos()-offset); //will not change size
        break;
      case ResizeTop:
        geom.setTop(ev->globalPos().y()-offset.y());
        if(geom.size().height() < minsize.height()){
	  geom.setTop(geom.y() - (minsize.height()-geom.size().height())); //reset back to min height
        }
        break;
      case ResizeTopRight:
        geom.setTopRight(ev->globalPos()-offset);
        if(geom.size().height() < minsize.height()){
	  geom.setTop(geom.y() - (minsize.height()-geom.size().height())); //reset back to min height
        }
        if(geom.size().width() < minsize.width()){
	  geom.setRight(geom.x() + minsize.width()); //reset back to min width
        }
        break;
      case ResizeRight:
        geom.setRight(ev->globalPos().x()-offset.x());
        if(geom.size().width() < minsize.width()){
	  geom.setRight(geom.x() + minsize.width()); //reset back to min width
        }
        break;
      case ResizeBottomRight:
        geom.setBottomRight(ev->globalPos()-offset);
        if(geom.size().height() < minsize.height()){
	  geom.setBottom(geom.y() + minsize.height()); //reset back to min height
        }
        if(geom.size().width() < minsize.width()){
	  geom.setRight(geom.x() + minsize.width()); //reset back to min width
        }
        break;
      case ResizeBottom:
        geom.setBottom(ev->globalPos().y()-offset.y());
        if(geom.size().height() < minsize.height()){
	  geom.setBottom(geom.y() + minsize.height()); //reset back to min height
        }
        break;
      case ResizeBottomLeft:
        geom.setBottomLeft(ev->globalPos()-offset);
        if(geom.size().height() < minsize.height()){
	  geom.setBottom(geom.y() + minsize.height()); //reset back to min height
        }
        if(geom.size().width() < minsize.width()){
	  geom.setLeft(geom.x() - (minsize.width()-geom.size().width())); //reset back to min width
        }
        break;
      case ResizeLeft:
        geom.setLeft(ev->globalPos().x()-offset.x());
        if(geom.size().width() < minsize.width()){
	  geom.setLeft(geom.x() - (minsize.width()-geom.size().width())); //reset back to min width
        }
        break;
      case ResizeTopLeft:
        geom.setTopLeft(ev->globalPos()-offset);
        if(geom.size().height() < minsize.height()){
	  geom.setTop(geom.y() - (minsize.height()-geom.size().height())); //reset back to min height
        }
        if(geom.size().width() < minsize.width()){
	  geom.setLeft(geom.x() - (minsize.width()-geom.size().width())); //reset back to min width
        }
        break;
      default:
	break;
    }

    this->setGeometry(geom);
  }
}

void RootSubWindow::mouseReleaseEvent(QMouseEvent *ev){
  //Check for a right-click event
  //qDebug() << "Frame Mouse Release Event";
  ev->accept();
  if( (activeState==Normal) && (titleBar->geometry().contains(ev->pos())) && (ev->button()==Qt::RightButton) ){
    otherM->popup(ev->globalPos());
    return;
  }
  activeState = Normal;
  QApplication::restoreOverrideCursor();
  setMouseCursor( getStateAtPoint(ev->pos()) );
  if(QWidget::mouseGrabber() == this){ this->releaseMouse(); }
}

void RootSubWindow::leaveEvent(QEvent *ev){
  QFrame::leaveEvent(ev);
  if(activeState == Normal){
    setMouseCursor(Normal);
  }
}

/*void RootSubWindow::hideEvent(QHideEvent *ev){
  WIN->requestProperty(NativeWindow::Visible, false);
  QFrame::hideEvent(ev);
}*/

void RootSubWindow::resizeEvent(QResizeEvent *ev){
  qDebug() << "Got Resize Event:" << ev->size();
  WIN->requestProperty(NativeWindow::Size, WinWidget->size());
  QFrame::resizeEvent(ev);
}
/*void RootSubWindow::showEvent(QShowEvent *ev){
  WIN->requestProperty(NativeWindow::Visible, true);
  QFrame::showEvent(ev);
}*/

