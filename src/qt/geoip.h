// Copyright (c) 2026 The Validity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GEOIP_H
#define BITCOIN_QT_GEOIP_H

#include <QString>
#include <QPair>

// Lightweight IP geolocation - maps IP addresses to approximate lat/lon
// Uses hardcoded IP range heuristics (no external dependencies)
namespace GeoIP {
    // Returns (latitude, longitude) for an IP address
    // Returns (0, 0) if lookup fails
    QPair<double, double> lookup(const QString &ipAddress);

    // Returns region name for display
    QString regionName(const QString &ipAddress);
}

#endif // BITCOIN_QT_GEOIP_H
