// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2025-2026 The Validity developers
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
#include "stakingchartwidget.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include <main.h>
#include <util.h>
#include "wallet/wallet.h"
#include "walletframe.h"

#include "carddragdrop.h"

#include <QDebug>

#include "rpc/server.h"

#include <QAbstractItemDelegate>
#include <QDateTime>
#include <QPainter>
#include <QPointer>
#include <QSplitter>
#include <QSettings>

#define DECORATION_SIZE 54
#define NUM_ITEMS 7

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
    txdelegate(new TxViewDelegate(platformStyle, this)),
    stakingChart(0),
    gridManager(0),
    lastStaking(false),
    initialStatsLoaded(false),
    cachedWalletTxCount(0),
    cachedBlockHeight(0),
    cachedSupply(0),
    cachedNetworkWeight(0)
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
    ui->listTransactions->setUniformItemSizes(true);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    connect(ui->labelWalletStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
    connect(ui->labelTransactionsStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));

    // Initially hide staking-specific widgets
    ui->progressBar_AnnualGeneration->setVisible(false);
    ui->labelAnualGenerationText->setVisible(false);
    ui->progressBar_MyWeight->setVisible(false);
    ui->labelMyWeightText->setVisible(false);
    ui->labelExpectedStakingStats->setVisible(false);
    ui->labelExpectedStakingStatsText->setVisible(false);

    // Progress bar styling is handled by the theme QSS now,
    // but set alignment and max values
    ui->progressBar_MyWeight->setAlignment(Qt::AlignCenter);
    ui->progressBar_Supply->setAlignment(Qt::AlignCenter);
    ui->progressBar_TotalStaking->setAlignment(Qt::AlignCenter);
    ui->progressBar_AnnualGeneration->setAlignment(Qt::AlignCenter);

    ui->progressBar_Supply->setMaximum(100);
    ui->progressBar_TotalStaking->setMaximum(100);
    ui->progressBar_MyWeight->setMaximum(100);
    ui->progressBar_AnnualGeneration->setMaximum(100);

    ui->labelSupplyText->setVisible(true);
    ui->progressBar_Supply->setVisible(true);
    ui->labelTotalStakingText->setVisible(true);
    ui->progressBar_TotalStaking->setVisible(true);

    // Create and insert the staking chart widget
    stakingChart = new StakingChartWidget(this);
    ui->chartPlaceholder->addWidget(stakingChart);
    stakingChart->setMinimumHeight(140);

    // Rearrange cards into a 2-column drag-and-drop grid
    QVBoxLayout *topLayout = qobject_cast<QVBoxLayout*>(layout());
    if (topLayout) {
        topLayout->removeWidget(ui->frame);
        topLayout->removeWidget(ui->frame_2);
        topLayout->removeWidget(ui->transactionsCard);
        topLayout->removeWidget(ui->networkCard);

        // Card order: balance, staking, network on left; transactions (tall) on right
        QList<QWidget*> cards;
        cards << ui->frame << ui->frame_2 << ui->networkCard << ui->transactionsCard;

        gridManager = new DashboardGridManager(this);
        QSplitter *grid = gridManager->setupGrid(cards);
        topLayout->addWidget(grid);
    }
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
        // Show balances immediately (lightweight)
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getStake(),
                           model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(), model->getWatchStake());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));

        // Defer the heavy transaction list setup — it triggers full wallet scan
        QTimer::singleShot(500, this, SLOT(setupTransactionList()));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::setupTransactionList()
{
    if (!walletModel || !walletModel->getOptionsModel())
        return;

    qDebug() << "OverviewPage::setupTransactionList START";
    int64_t nStart = GetTimeMicros();

    filter.reset(new TransactionFilterProxy());
    filter->setSourceModel(walletModel->getTransactionTableModel());
    filter->setLimit(NUM_ITEMS);
    filter->setDynamicSortFilter(true);
    filter->setSortRole(Qt::EditRole);
    filter->setShowInactive(false);
    filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

    ui->listTransactions->setModel(filter.get());
    ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

    qDebug() << "OverviewPage::setupTransactionList DONE in" << (GetTimeMicros() - nStart) / 1000 << "ms";
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
struct StakePeriodRange_T {
    int64_t Start;
    int64_t End;
    int64_t Total;
    int Count;
    std::string Name;
};

typedef std::vector<StakePeriodRange_T> vStakePeriodRange_T;

extern vStakePeriodRange_T PrepareRangeForStakeReport();
extern int GetsStakeSubTotal(vStakePeriodRange_T& aRange);
extern double GetSupply();

static double roundTo2(double value){
    return (double)((int64_t)(value * 100)) / 100.0;
}


void OverviewPage::BlockCountChanged(int count, const QDateTime& blockDate, double nVerificationProgress, bool header){

    //if last update time was less than 5 seconds ago, do nothing
     if ((GetTime() - nLastReportUpdate) < 5)
        return;

    // if initial block download, do nothing
    if(IsInitialBlockDownload())
        return;
    // if walletmodel is not available, do nothing
    if (!walletModel || !walletModel->getOptionsModel())
        return;

    if (!pwalletMain)
        return;

    bool staking;
    {
        LOCK(pwalletMain->cs_wallet);
        staking = pwalletMain->IsStaking();
    }

    // if staking status has changed, force update
    if(lastStaking != staking)
        nLastReportUpdate = 0;
    lastStaking = staking;

    // Defer the very first stats load so the UI renders immediately
    if (!initialStatsLoaded) {
        initialStatsLoaded = true;
        QPointer<OverviewPage> guard(this);
        QTimer::singleShot(2000, [guard]() {
            if (guard) guard->deferredStatsLoad();
        });
        return;
    }

    if ((GetTime() - nLastReportUpdate) > 300) {
        int64_t nMyWeight;
        {
            LOCK(pwalletMain->cs_wallet);
            nMyWeight = pwalletMain->GetStakeWeight();
        }
        int64_t nNetworkWeight;
        int64_t nCoinSupply;

        {
            LOCK(cs_main);
            nNetworkWeight = GetPoSKernelPS();

            // Cache validation: rescan UTXO if block height changed,
            // network weight drifted significantly, or cache is empty
            bool cacheValid = (cachedSupply > 0) &&
                              (count == cachedBlockHeight) &&
                              (cachedNetworkWeight > 0) &&
                              (qAbs(nNetworkWeight - cachedNetworkWeight) <
                               cachedNetworkWeight / 5); // <20% drift

            if (!cacheValid) {
                nCoinSupply = GetSupply();
                cachedSupply = nCoinSupply;
                cachedNetworkWeight = nNetworkWeight;
                cachedBlockHeight = count;
            } else {
                nCoinSupply = cachedSupply;
            }
        }

       int unit = walletModel->getOptionsModel()->getDisplayUnit();

       // Cache validation: rescan wallet txs if count changed or
       // staking status changed (new stakes may have matured)
       size_t currentTxCount;
       {
           LOCK(pwalletMain->cs_wallet);
           currentTxCount = pwalletMain->mapWallet.size();
       }
       if (currentTxCount != cachedWalletTxCount) {
           UpdateHistoricalStakingStats(unit);
           cachedWalletTxCount = currentTxCount;
       }

       UpdateNetworkStats(nCoinSupply, nNetworkWeight, unit);
       UpdateCurrentStakingStats(staking, nMyWeight, nNetworkWeight, unit, count);

       // Save the last update
       nLastReportUpdate = GetTime();
    }
}

void OverviewPage::deferredStatsLoad()
{
    if (!walletModel || !walletModel->getOptionsModel() || !pwalletMain)
        return;

    qDebug() << "OverviewPage::deferredStatsLoad START";
    int64_t nStart = GetTimeMicros();

    bool staking;
    int64_t nMyWeight;
    {
        LOCK(pwalletMain->cs_wallet);
        staking = pwalletMain->IsStaking();
        nMyWeight = pwalletMain->GetStakeWeight();
    }

    int64_t nNetworkWeight;
    int64_t nCoinSupply;

    {
        LOCK(cs_main);
        nNetworkWeight = GetPoSKernelPS();
        nCoinSupply = GetSupply();
        cachedSupply = nCoinSupply;
        cachedNetworkWeight = nNetworkWeight;
        cachedBlockHeight = chainActive.Height();
    }

    int unit = walletModel->getOptionsModel()->getDisplayUnit();

    UpdateHistoricalStakingStats(unit);
    {
        LOCK(pwalletMain->cs_wallet);
        cachedWalletTxCount = pwalletMain->mapWallet.size();
    }

    UpdateNetworkStats(nCoinSupply, nNetworkWeight, unit);
    UpdateCurrentStakingStats(staking, nMyWeight, nNetworkWeight, unit, chainActive.Height());

    nLastReportUpdate = GetTime();

    qDebug() << "OverviewPage::deferredStatsLoad DONE in" << (GetTimeMicros() - nStart) / 1000 << "ms";
}

void OverviewPage::UpdateHistoricalStakingStats(int unit){
    // Get data for staking report
    vStakePeriodRange_T aRange = PrepareRangeForStakeReport();
    GetsStakeSubTotal(aRange);

    if (aRange.size() < 35)
        return;

    // Prepare the subtotals (indices 30-34 are the summary periods)
    CAmount amount24h = aRange[30].Total;
    CAmount amount7d  = aRange[31].Total;
    CAmount amount30d = aRange[32].Total;
    CAmount amount1y  = aRange[33].Total;
    CAmount amountAll = aRange[34].Total;

    // Display staking history
    ui->label24hStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amount24h, false, BitcoinUnits::separatorAlways, 2));
    ui->label7dStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amount7d, false, BitcoinUnits::separatorAlways, 2));
    ui->label30dStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amount30d, false, BitcoinUnits::separatorAlways, 2));
    ui->label1yStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amount1y, false, BitcoinUnits::separatorAlways, 2));
    ui->labelallStakingStats->setText(BitcoinUnits::formatWithUnit(unit, amountAll, false, BitcoinUnits::separatorAlways, 2));

    uiInterface.SetStaked(amountAll, amount24h, amount7d);

    // Update the staking chart with daily data (first 30 entries are individual days)
    QVector<StakeDayData> chartData;
    for (int i = 0; i < 30 && i < (int)aRange.size(); i++) {
        StakeDayData day;
        // Convert timestamp to short date label
        QDateTime dt;
#if QT_VERSION >= 0x050800
        dt = QDateTime::fromSecsSinceEpoch(aRange[i].Start);
#else
        dt = QDateTime::fromTime_t(aRange[i].Start);
#endif
        day.label = dt.toString("MMM d");
        day.amount = aRange[i].Total;
        chartData.append(day);
    }
    stakingChart->setUnit(unit);
    stakingChart->setData(chartData);
}

void OverviewPage::UpdateCurrentStakingStats(bool staking, int64_t nMyWeight, int64_t nNetworkWeight, int unit, int nHeight){

    // set visability
    ui->labelMyWeightText->setVisible(staking);
    ui->progressBar_MyWeight->setVisible(staking);
    ui->labelAnualGenerationText->setVisible(staking);
    ui->progressBar_AnnualGeneration->setVisible(staking);
    ui->labelExpectedStakingStats->setVisible(staking);
    ui->labelExpectedStakingStatsText->setVisible(staking);

    if(!staking)
        return;

    //Set user stake weight progress bar
    double pMyWeight = (nNetworkWeight > 0) ? ((double)nMyWeight/(double)nNetworkWeight) : 0;
    ui->progressBar_MyWeight->setValue(pMyWeight*100);
    ui->progressBar_MyWeight->setFormat(tr("%1%").arg(roundTo2(pMyWeight*100)));

    double nStakeSubsidy = (double)getFixedStakeSubsidy(nHeight);
    double nAnnualCoins = (nStakeSubsidy * 60.0 * 25.0 * 365.0) * pMyWeight;
    double nTotalBalance = (double)(currentBalance + currentUnconfirmedBalance + currentImmatureBalance + currentStake);
    double pAnualPercent = (nTotalBalance > 0) ? roundTo2(nAnnualCoins / nTotalBalance) : 0;

    //set stake generation bar
    ui->progressBar_AnnualGeneration->setValue(pAnualPercent*100);
    ui->progressBar_AnnualGeneration->setFormat(tr("%1% ").arg(roundTo2(pAnualPercent*100)));

    double rawDailyReward = (1440.0 * nStakeSubsidy) * pMyWeight;
    CAmount nExpectedDailyReward = (rawDailyReward > 0 && rawDailyReward < (double)MAX_MONEY)
        ? (CAmount)rawDailyReward : 0;
    ui->labelExpectedStakingStats->setText(BitcoinUnits::formatWithUnit(unit, nExpectedDailyReward, false, BitcoinUnits::separatorAlways, 2));
}

void OverviewPage::UpdateNetworkStats(int64_t nCoinSupply, int64_t nNetworkWeight, int unit){

    double pCoinSupply = (((double)nCoinSupply/100000000)/(double)9000000) *100  ;
    double pStakingCoins = (nCoinSupply > 0) ? (((double)nNetworkWeight/(double)nCoinSupply) *100) : 0;
    //update total staking coins bar
    ui->progressBar_TotalStaking->setValue(pStakingCoins);
    ui->progressBar_TotalStaking->setFormat(tr("%1% (%2)").arg(roundTo2(pStakingCoins)).arg(BitcoinUnits::format(unit, nNetworkWeight, false, BitcoinUnits::separatorNever, 0)));
    ui->progressBar_TotalStaking->setAlignment(Qt::AlignCenter);

    // update coin supply bar
    ui->progressBar_Supply->setValue(pCoinSupply);
    ui->progressBar_Supply->setFormat(tr("%1 / %2").arg(BitcoinUnits::format(unit, nCoinSupply, false, BitcoinUnits::separatorNever, 0)).arg(9000000));
    ui->progressBar_Supply->setAlignment(Qt::AlignCenter);
}



void OverviewPage::NewBlock(bool fImmediate, int nHeight)
{
}
