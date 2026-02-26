# Validity Wallet: Dark Theme + Dashboard Modernization

**Date:** 2026-02-25
**Status:** Approved
**Constraint:** No consensus/fork changes — UI/UX only

## Theme System

ThemeManager class loads QSS stylesheets at runtime. Two themes: Light (preserves current) and Dark (new). Preference persisted via QSettings. Toggle in Options dialog.

### Dark Palette (Validity-branded)
- Background: #1e1e2e
- Card/Surface: #2a2a3d
- Card Border: #3a3a4d
- Primary Accent: #43b581 (existing Validity green)
- Text Primary: #e0e0e0
- Text Secondary: #a0a0b0
- Negative: #e05555
- Warning: #f0b060

## Dashboard Cards (Overview Page)

Replace flat grid with 4 cards:
1. **Balance Card** (top-left) — Available, Pending, Immature, Stake, Total
2. **Staking Rewards Card** (top-right) — 30-day bar chart + summary stats
3. **Network Stats Card** (bottom-left) — Supply, network weight, your weight %, daily reward
4. **Recent Transactions Card** (bottom-right) — Last 5 transactions

## Staking Chart Widget

Custom QPainter widget: 30 vertical bars (one per day), x-axis dates, y-axis amounts. Uses PrepareRangeForStakeReport() data. Validity green bars, hover tooltips.

## New Files
- src/qt/res/themes/dark.qss
- src/qt/res/themes/light.qss
- src/qt/thememanager.h/.cpp
- src/qt/stakingchartwidget.h/.cpp

## Modified Files
- src/qt/forms/overviewpage.ui
- src/qt/overviewpage.cpp/.h
- src/qt/bitcoingui.cpp/.h
- src/qt/optionsdialog.cpp/.ui
- src/qt/guiconstants.h
- src/qt/bitcoin.qrc
- src/qt/Makefile.qt.include
