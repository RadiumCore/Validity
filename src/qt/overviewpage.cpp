// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include <main.h>
#include <util.h>
#include "wallet/wallet.h"
#include "walletframe.h"

#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 54
#define NUM_ITEMS 5

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(BitcoinUnits::BTC),
        platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon = platformStyle->SingleColorIcon(icon);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentStake(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    currentWatchOnlyStake(-1),
    txdelegate(new TxViewDelegate(platformStyle, this))
{
    ui->setupUi(this);

    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/warning");
    icon.addPixmap(icon.pixmap(QSize(64,64), QIcon::Normal), QIcon::Disabled); // also set the disabled icon because we are using a disabled QPushButton to work around missing HiDPI support of QLabel (https://bugreports.qt.io/browse/QTBUG-42503)
    ui->labelTransactionsStatus->setIcon(icon);
    ui->labelWalletStatus->setIcon(icon);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    NewBlock(false);
    connect(ui->labelWalletStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
    connect(ui->labelTransactionsStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));

    ui->progressBar_AnnualGeneration->setVisible(false);
    ui->labelAnualGenerationText->setVisible(false); 
    ui->progressBar_MyWeight->setVisible(false);
    ui->labelMyWeightText->setVisible(false);
    ui->labelExpectedStakingStats->setVisible(false);
    ui->labelExpectedStakingStatsText->setVisible(false);

    ui->labelSupplyText->setVisible(false);
    ui->progressBar_Supply->setVisible(false);
    ui->progressBar_Supply->setVisible(false);
	


    ui->labelTotalStakingText->setVisible(false);
    ui->progressBar_TotalStaking->setVisible(false);
    ui->progressBar_AnnualGeneration->setStyleSheet("QProgressBar { background-color: white; border: 0px solid grey; border-radius: 0px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #43b581, stop: 1 #43b581); border-radius: 0px; margin: 0px; }"); 
    ui->progressBar_MyWeight->setStyleSheet("QProgressBar { background-color: white; border: 0px solid grey; border-radius: 0px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #43b581, stop: 1 #43b581); border-radius: 0px; margin: 0px; }");
    ui->progressBar_Supply->setStyleSheet("QProgressBar { background-color: white; border: 0px solid grey; border-radius: 0px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #43b581, stop: 1 #43b581); border-radius: 0px; margin: 0px; }"); 
    ui->progressBar_TotalStaking->setStyleSheet("QProgressBar { background-color: white; border: 0px solid grey; border-radius: 0px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #43b581, stop: 1 #43b581); border-radius: 0px; margin: 0px; }"); 

    ui->labelSupplyText->setVisible(true);
     ui->progressBar_Supply->setVisible(true); 
     ui->labelTotalStakingText->setVisible(true);
     ui->progressBar_TotalStaking->setVisible(true);

    ui->progressBar_MyWeight->setAlignment(Qt::AlignCenter);
    ui->progressBar_Supply->setAlignment(Qt::AlignCenter);
    ui->progressBar_TotalStaking->setAlignment(Qt::AlignCenter);
    ui->progressBar_AnnualGeneration->setAlignment(Qt::AlignCenter);

    ui->progressBar_Supply->setMaximum(100);
    ui->progressBar_TotalStaking->setMaximum(100);
    ui->progressBar_MyWeight->setMaximum(100);
    ui->progressBar_AnnualGeneration->setMaximum(100);


}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}




void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& stake, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance, const CAmount& watchOnlyStake)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentStake = stake;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    currentWatchOnlyStake = watchOnlyStake;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, stake, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance + stake, false, BitcoinUnits::separatorAlways));
    ui->labelWatchAvailable->setText(BitcoinUnits::formatWithUnit(unit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::formatWithUnit(unit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::formatWithUnit(unit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchStake->setText(BitcoinUnits::formatWithUnit(unit, watchOnlyStake, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::formatWithUnit(unit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance + watchOnlyStake, false, BitcoinUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showStake = stake != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;
    bool showWatchOnlyStake = watchOnlyStake != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance
    ui->labelStake->setVisible(showStake || showWatchOnlyStake);
    ui->labelStakeText->setVisible(showStake || showWatchOnlyStake);
    ui->labelWatchStake->setVisible(showWatchOnlyStake); // show watch-only stake balance

}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly)
    {
        ui->labelWatchImmature->hide();
        ui->labelWatchStake->hide();
    }
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
      
        connect(model, SIGNAL(numBlocksChanged(int, QDateTime, double, bool)), this, SLOT(BlockCountChanged(int, QDateTime, double, bool)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getStake(),
                           model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(), model->getWatchStake());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
        	 setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentStake,
        	 currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance, currentWatchOnlyStake);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}
using namespace boost;


using namespace std;


struct StakePeriodRange_T {
    int64_t Start;
    int64_t End;
    int64_t Total;
    int Count;
    string Name;
};

typedef vector<StakePeriodRange_T> vStakePeriodRange_T;

extern vStakePeriodRange_T PrepareRangeForStakeReport();
extern int GetsStakeSubTotal(vStakePeriodRange_T& aRange);
double GetPoSKernelPS();
double GetSupply();

double round(double value){
     int64_t pre_round = value * 100;;
    return ((double)pre_round) / 100;
}


void OverviewPage::BlockCountChanged(int count, const QDateTime& blockDate, double nVerificationProgress, bool header){
	NewBlock(false);
}

void OverviewPage::NewBlock(bool fImmediate)
{
    
    
    if ((GetTime() - nLastReportUpdate) < 5)
        return;
    
    if(IsInitialBlockDownload())
        return;
     ui->labelSupplyText->setVisible(true);

     ui->progressBar_Supply->setVisible(true);
     ui->progressBar_Supply->setMaximum(100);

     ui->labelTotalStakingText->setVisible(true);
     
     ui->progressBar_TotalStaking->setMaximum(100);
     ui->progressBar_TotalStaking->setVisible(true);


   LOCK(cs_main);
    if (!walletModel || !walletModel->getOptionsModel())
        return;

    // Get data for staking report
    static vStakePeriodRange_T aRange;
   


    if (fImmediate)
        nLastReportUpdate = 0;

    int64_t nMyWeight = pwalletMain ? pwalletMain->GetStakeWeight() : 0;
    bool staking = nLastCoinStakeSearchInterval && nMyWeight;

// if staking status has changed, force update
    if(lastStaking != staking)
        nLastReportUpdate = 0;


    // Skip report recalc if not immediate or before 5 minutes from last
    if ((GetTime() - nLastReportUpdate) > 300) {
        // Load the range
        aRange = PrepareRangeForStakeReport();

        // Get the subtotals
        GetsStakeSubTotal(aRange);

        // Save the last update
        nLastReportUpdate = GetTime();
    }


    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    // Prepair the subtotals
    CAmount amount24h = round(aRange[30].Total);
    CAmount amount7d = round(aRange[31].Total);
    CAmount amount30d = round(aRange[32].Total);
    CAmount amount1y = round(aRange[33].Total);
    CAmount amountAll = round(aRange[34].Total);

    // Display staking history
    ui->label24hStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amount24h, false, BitcoinUnits::separatorAlways, 2));
    ui->label7dStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amount7d, false, BitcoinUnits::separatorAlways, 2));
    ui->label30dStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amount30d, false, BitcoinUnits::separatorAlways, 2));
    ui->label1yStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amount1y, false, BitcoinUnits::separatorAlways, 2));
    ui->labelallStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amountAll, false, BitcoinUnits::separatorAlways, 2));
   

    // Get network staking stats
    CCoinsViewCache view(pcoinsTip);

  

   
    int64_t nNetworkWeight = GetPoSKernelPS();    
    int64_t nCoinSupply = GetSupply(); 

    double pStakingCoins = ((double)nNetworkWeight/(double)nCoinSupply) *100;
    double pCoinSupply = (((double)nCoinSupply/100000000)/(double)9000000) *100  ;

   
  
  



    //update total staking coins bar
    ui->progressBar_TotalStaking->setValue(pStakingCoins);
    ui->progressBar_TotalStaking->setFormat(tr("%1% (%2)").arg(round(pStakingCoins)).arg(BitcoinUnits::format(unit, nNetworkWeight, false, BitcoinUnits::separatorNever, 0)));
    ui->progressBar_TotalStaking->setAlignment(Qt::AlignCenter);
   
    // update coin supply bar
    ui->progressBar_Supply->setValue(pCoinSupply);
    ui->progressBar_Supply->setFormat(tr("%1 / %2").arg(BitcoinUnits::format(unit, nCoinSupply, false, BitcoinUnits::separatorNever, 0)).arg(9000000));
    ui->progressBar_Supply->setAlignment(Qt::AlignCenter);




   
    if (staking){


        //activate progress bars
        ui->labelMyWeightText->setVisible(true);
        ui->progressBar_MyWeight->setMaximum(100);
        ui->progressBar_MyWeight->setVisible(true);
        ui->labelAnualGenerationText->setVisible(true);
        ui->progressBar_AnnualGeneration->setMaximum(100);
        ui->progressBar_AnnualGeneration->setVisible(true);
       
    
        //Set user stake weight progress bar
        double pMyWeight = ((double)nMyWeight/(double)nNetworkWeight);
        ui->progressBar_MyWeight->setValue(pMyWeight*100);
        ui->progressBar_MyWeight->setFormat(tr("%1%").arg(round(pMyWeight*100)));
        ui->progressBar_MyWeight->setAlignment(Qt::AlignCenter);

        
        double nStakeSubsidy = GetProofOfStakeSubsidy(chainActive.Tip(), 0);
        double nAnnualCoins = ((nStakeSubsidy * 60 * 25 * 365) * pMyWeight); 
        double nTotalBalance = currentBalance + currentUnconfirmedBalance + currentImmatureBalance + currentStake;
        double pAnualPercent = round(nAnnualCoins/nTotalBalance);
       
   

         //set stake generation bar
         ui->progressBar_AnnualGeneration->setValue(pAnualPercent*100);
         ui->progressBar_AnnualGeneration->setFormat(tr("%1% ").arg(round(pAnualPercent*100)));
         ui->progressBar_AnnualGeneration->setAlignment(Qt::AlignCenter);



         
          CAmount nExpectedDailyReward = (1440 * nStakeSubsidy) * pMyWeight ;
          ui->labelExpectedStakingStats->setVisible(true);
          ui->labelExpectedStakingStatsText->setVisible(true);
          ui->labelExpectedStakingStats->setText(BitcoinUnits::formatWithUnit(unit, nExpectedDailyReward, false, BitcoinUnits::separatorAlways, 2));
       
        
    }
    else{
         ui->labelExpectedStakingStats->setText("Wallet not staking");
         
         ui->progressBar_AnnualGeneration->setVisible(false);
          ui->labelAnualGenerationText->setVisible(false);

         //ui->progressBar_AnnualGeneration->setMaximum(0);
         //ui->progressBar_AnnualGeneration->setFormat(tr("Wallet not staking"));

         ui->progressBar_MyWeight->setVisible(false);
         ui->labelMyWeightText->setVisible(false);
         //ui->progressBar_MyWeight->setMaximum(0);
         //ui->progressBar_MyWeight->setFormat(tr("Wallet not staking"));

         
          ui->labelExpectedStakingStats->setVisible(false);
          ui->labelExpectedStakingStatsText->setVisible(false);
          
          
       
          
         }
       


    uiInterface.SetStaked(amountAll, amount24h, amount7d);







   
   
   

}





