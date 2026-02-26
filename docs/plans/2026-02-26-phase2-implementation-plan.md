# Phase 2: Full Visual Overhaul + Remaining Features

**Date:** 2026-02-26
**Constraint:** NO consensus/fork changes. UI/client-side ONLY.
**Goal:** Match validitytech.com aesthetic, implement all remaining recommendations, security audit.

## Priority 1: Visual Overhaul (Match validitytech.com)

### 1A. Updated Dark Theme (dark.qss rewrite)
Redesign the dark theme to match the website's premium, cosmic aesthetic:
- **Background gradient:** Deep navy #0a0a1a to dark purple #1a1030
- **Card surfaces:** Semi-transparent dark with subtle border glow
- **Accent color:** Keep #43b581 (Validity green) — matches website exactly
- **Typography:** Larger headings, more letter-spacing, bolder weights
- **Buttons:** Pill-shaped with green accent, hover glow effects
- **Progress bars:** Green gradient with glow
- **Inputs:** Dark with subtle border, green focus ring
- **Overall vibe:** Premium, futuristic, cryptocurrency-native

### 1B. Custom Sidebar Navigation
Replace the default toolbar/tab bar with a modern vertical sidebar:
- Validity logo at top
- Icon + text navigation items (Overview, Send, Receive, Transactions, Addresses)
- Active state with green accent bar
- Collapsible to icon-only mode
- Staking status indicator in sidebar footer

### 1C. Updated Splash Screen
- Dark background with Validity logo
- Animated loading progress
- Step descriptions ("Loading block index...", "Verifying blocks...")

### 1D. Improved Window Chrome
- Custom title bar styling (within QSS limits)
- Status bar with staking indicator, connection count, block height
- Unified dark appearance

## Priority 2: Security Audit

### 2A. Client-Side Code Review
- RPC command input validation
- Wallet encryption implementation review
- Key storage and memory handling
- Network message parsing safety
- Serialization buffer overflow checks
- User input sanitization (addresses, amounts, labels)
- QSS/theme loading safety
- File permission handling
- Memory wiping for sensitive data
- Review `#include "rpc/blockchain.cpp"` pattern

## Priority 3: UI Page Improvements

### 3A. Send Page Modernization
- Modern form layout with card styling
- Amount input with unit selector
- Fee estimation display
- Transaction preview before send
- Success/failure feedback

### 3B. Receive Page Modernization
- QR code display with dark theme support
- Copy address button with visual feedback
- Request amount field
- Generated URI display

### 3C. Transaction History
- Date range filter
- Amount filter
- Type filter (sent, received, staked)
- Search by address/label
- Better transaction detail panel
- Export to CSV button

### 3D. Address Book
- Search functionality
- Category/label system
- Quick copy with feedback
- Contact card layout

## Priority 4: Medium Features

### 4A. Options Dialog Modernization
- Categorized settings with sidebar
- Theme picker section
- Network settings with visual status
- Wallet settings (backup path, etc.)

### 4B. System Tray Improvements
- Balance in tooltip
- Staking status icon
- Quick actions menu

### 4C. Notification System
- Desktop notifications for staking rewards
- Transaction received alerts
- Notification preferences in settings

## Implementation Order

1. dark.qss rewrite (visual overhaul foundation)
2. Sidebar navigation widget
3. Send/Receive page styling
4. Transaction history improvements
5. Address book improvements
6. Options dialog
7. Splash screen
8. Security audit report
9. System tray + notifications
